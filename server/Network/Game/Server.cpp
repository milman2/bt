#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <cstring>

#include "../World/GameWorld.h"
#include "PacketHandler.h"
#include "Server.h"

namespace bt
{

    Server::Server(const ServerConfig& config) : config_(config), running_(false), server_socket_(-1)
    {
        // 게임 월드와 패킷 핸들러 초기화
        game_world_     = std::make_unique<GameWorld>();
        packet_handler_ = std::make_unique<PacketHandler>();

        LogMessage("서버 인스턴스가 생성되었습니다.");
    }

    Server::~Server()
    {
        stop();
        LogMessage("서버 인스턴스가 소멸되었습니다.");
    }

    bool Server::start()
    {
        if (running_.load())
        {
            LogMessage("서버가 이미 실행 중입니다.", true);
            return false;
        }

        LogMessage("서버 시작 중...");

        // 소켓 생성 및 바인딩
        if (!CreateSocket() || !BindSocket() || !ListenSocket())
        {
            LogMessage("소켓 설정 실패", true);
            return false;
        }

        // 게임 월드 시작
        game_world_->StartGameLoop();

        // 워커 스레드 시작
        for (int i = 0; i < config_.worker_threads; ++i)
        {
            worker_threads_.emplace_back(&Server::WorkerThreadFunction, this);
        }

        // 연결 수락 스레드 시작
        std::thread accept_thread(&Server::AcceptConnections, this);
        accept_thread.detach();

        running_.store(true);
        LogMessage("서버가 성공적으로 시작되었습니다. 포트: " + std::to_string(config_.port));

        return true;
    }

    void Server::stop()
    {
        if (!running_.load())
        {
            return;
        }

        LogMessage("서버 종료 중...");
        running_.store(false);

        // 소켓 닫기
        if (server_socket_ != -1)
        {
            close(server_socket_);
            server_socket_ = -1;
        }

        // 모든 클라이언트 연결 종료
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            for (auto& [socket_fd, client] : clients_)
            {
                close(socket_fd);
            }
            clients_.clear();
        }

        // 게임 월드 중지
        if (game_world_)
        {
            game_world_->StopGameLoop();
        }

        // 워커 스레드 종료
        task_queue_cv_.notify_all();
        for (auto& thread : worker_threads_)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        worker_threads_.clear();

        LogMessage("서버가 종료되었습니다.");
    }

    bool Server::CreateSocket()
    {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ == -1)
        {
            LogMessage("소켓 생성 실패: " + std::string(strerror(errno)), true);
            return false;
        }

        // 소켓 옵션 설정
        int opt = 1;
        if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            LogMessage("소켓 옵션 설정 실패: " + std::string(strerror(errno)), true);
            close(server_socket_);
            return false;
        }

        return true;
    }

    bool Server::BindSocket()
    {
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family      = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(config_.host.c_str());
        server_addr_.sin_port        = htons(config_.port);

        if (bind(server_socket_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) == -1)
        {
            LogMessage("소켓 바인딩 실패: " + std::string(strerror(errno)), true);
            close(server_socket_);
            return false;
        }

        return true;
    }

    bool Server::ListenSocket()
    {
        if (listen(server_socket_, config_.max_clients) == -1)
        {
            LogMessage("소켓 리스닝 실패: " + std::string(strerror(errno)), true);
            close(server_socket_);
            return false;
        }

        return true;
    }

    void Server::AcceptConnections()
    {
        LogMessage("연결 수락 스레드 시작");

        while (running_.load())
        {
            struct sockaddr_in client_addr;
            socklen_t          client_len = sizeof(client_addr);

            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);

            if (client_socket == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                if (running_.load())
                {
                    LogMessage("연결 수락 실패: " + std::string(strerror(errno)), true);
                }
                continue;
            }

            // 클라이언트 수 제한 확인
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                if (clients_.size() >= static_cast<size_t>(config_.max_clients))
                {
                    LogMessage("최대 클라이언트 수 초과. 연결 거부", true);
                    close(client_socket);
                    continue;
                }
            }

            // 클라이언트 추가
            std::string client_ip   = inet_ntoa(client_addr.sin_addr);
            uint16_t    client_port = ntohs(client_addr.sin_port);
            AddClient(client_socket, client_ip, client_port);
        }

        LogMessage("연결 수락 스레드 종료");
    }

    void Server::AddClient(int socket_fd, const std::string& ip, uint16_t port)
    {
        auto client              = std::make_unique<ClientInfo>();
        client->socket_fd        = socket_fd;
        client->ip_address       = ip;
        client->port             = port;
        client->connect_time     = std::chrono::steady_clock::now();
        client->is_authenticated = false;
        client->player_id        = 0;

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_[socket_fd] = std::move(client);
        }

        // 클라이언트 처리 스레드 시작
        std::thread client_thread(&Server::HandleClient, this, socket_fd);
        client_thread.detach();

        LogMessage("새 클라이언트 연결: " + ip + ":" + std::to_string(port) + " (소켓: " + std::to_string(socket_fd) +
                    ")");
    }

    void Server::RemoveClient(int socket_fd)
    {
        std::string client_info;

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            auto                        it = clients_.find(socket_fd);
            if (it != clients_.end())
            {
                client_info = it->second->ip_address + ":" + std::to_string(it->second->port);
                clients_.erase(it);
            }
        }

        close(socket_fd);

        if (!client_info.empty())
        {
            LogMessage("클라이언트 연결 종료: " + client_info);
        }
    }

    void Server::HandleClient(int socket_fd)
    {
        LogMessage("클라이언트 처리 시작: 소켓 " + std::to_string(socket_fd));

        // 소켓을 논블로킹으로 설정
        SetSocketNonBlocking(socket_fd);

        std::vector<uint8_t> buffer(4096);

        while (running_.load())
        {
            // 클라이언트가 여전히 연결되어 있는지 확인
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                if (clients_.find(socket_fd) == clients_.end())
                {
                    break;
                }
            }

            // 데이터 수신
            ssize_t bytes_received = recv(socket_fd, buffer.data(), buffer.size(), 0);

            if (bytes_received == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                LogMessage("데이터 수신 오류: " + std::string(strerror(errno)), true);
                break;
            }
            else if (bytes_received == 0)
            {
                LogMessage("클라이언트 연결 종료 감지");
                break;
            }

            // 패킷 처리 (간단한 구현)
            if (bytes_received >= sizeof(uint32_t))
            {
                uint32_t packet_size = *reinterpret_cast<uint32_t*>(buffer.data());
                if (packet_size <= bytes_received)
                {
                    Packet packet;
                    packet.size = packet_size;
                    if (packet_size > sizeof(uint32_t))
                    {
                        packet.type = *reinterpret_cast<uint16_t*>(buffer.data() + sizeof(uint32_t));
                        packet.data.assign(buffer.begin() + sizeof(uint32_t) + sizeof(uint16_t),
                                           buffer.begin() + packet_size);
                    }
                    ProcessPacket(socket_fd, packet);
                }
            }
        }

        RemoveClient(socket_fd);
        LogMessage("클라이언트 처리 종료: 소켓 " + std::to_string(socket_fd));
    }

    void Server::ProcessPacket(int socket_fd, const Packet& packet)
    {
        if (packet_handler_)
        {
            packet_handler_->HandlePacket(socket_fd, packet);
        }
    }

    void Server::WorkerThreadFunction()
    {
        LogMessage("워커 스레드 시작");

        while (running_.load())
        {
            std::unique_lock<std::mutex> lock(task_queue_mutex_);
            task_queue_cv_.wait(lock,
                                [this]
                                {
                                    return !task_queue_.empty() || !running_.load();
                                });

            if (!running_.load())
            {
                break;
            }

            if (!task_queue_.empty())
            {
                auto task = task_queue_.front();
                task_queue_.pop();
                lock.unlock();

                task();
            }
        }

        LogMessage("워커 스레드 종료");
    }

    void Server::BroadcastPacket(const Packet& packet, int exclude_socket)
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);

        for (const auto& [socket_fd, client] : clients_)
        {
            if (socket_fd != exclude_socket)
            {
                SendPacket(socket_fd, packet);
            }
        }
    }

    void Server::SendPacket(int socket_fd, const Packet& packet)
    {
        // 패킷 크기 + 타입 + 데이터 전송
        uint32_t             total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
        std::vector<uint8_t> send_buffer(total_size);

        // 패킷 크기
        *reinterpret_cast<uint32_t*>(send_buffer.data()) = total_size;
        // 패킷 타입
        *reinterpret_cast<uint16_t*>(send_buffer.data() + sizeof(uint32_t)) = packet.type;
        // 패킷 데이터
        std::copy(packet.data.begin(), packet.data.end(), send_buffer.begin() + sizeof(uint32_t) + sizeof(uint16_t));

        ssize_t bytes_sent = send(socket_fd, send_buffer.data(), total_size, 0);
        if (bytes_sent == -1)
        {
            LogMessage("패킷 전송 실패: " + std::string(strerror(errno)), true);
        }
    }

    bool Server::SetSocketNonBlocking(int socket_fd)
    {
        int flags = fcntl(socket_fd, F_GETFL, 0);
        if (flags == -1)
        {
            return false;
        }
        return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    void Server::LogMessage(const std::string& message, bool is_error)
    {
        std::lock_guard<std::mutex> lock(log_mutex_);

        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm     = *std::localtime(&time_t);

        std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        if (is_error)
        {
            std::cout << "[ERROR] ";
        }
        else
        {
            std::cout << "[INFO] ";
        }
        std::cout << message << std::endl;
    }

} // namespace bt
