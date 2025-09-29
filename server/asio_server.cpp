#include <iomanip>
#include <iostream>
#include <sstream>

#include "asio_server.h"
#include "behavior_tree.h"
#include "monster_ai.h"
#include "player.h"
#include "player_manager.h"
#include "rest_api_server.h"
#include "simple_websocket_server.h"

namespace bt
{

    AsioServer::AsioServer(const AsioServerConfig& config)
        : config_(config), running_(false), acceptor_(io_context_), total_packets_sent_(0), total_packets_received_(0)
    {
        // Behavior Tree 엔진 초기화
        bt_engine_ = std::make_unique<BehaviorTreeEngine>();

        // 매니저들 초기화
        monster_manager_ = std::make_shared<MonsterManager>();
        player_manager_  = std::make_shared<PlayerManager>();

        // 웹 서버 초기화
        rest_api_server_ = std::make_shared<RestApiServer>(8081); // REST API 서버는 8081 포트 사용
        rest_api_server_->set_monster_manager(monster_manager_);
        rest_api_server_->set_player_manager(player_manager_);
        // bt_engine_을 shared_ptr로 변환 (원본은 유지)
        std::shared_ptr<BehaviorTreeEngine> shared_bt_engine(bt_engine_.get(),
                                                             [](BehaviorTreeEngine*)
                                                             {
                                                             });
        rest_api_server_->set_bt_engine(shared_bt_engine);

        // WebSocket 서버 초기화
        websocket_server_ = std::make_shared<SimpleWebSocketServer>(8082); // WebSocket 서버는 8082 포트 사용

        // RestApiServer에 WebSocket 서버 참조 설정
        rest_api_server_->set_websocket_server(websocket_server_);

        // MonsterManager에 WebSocket 서버 설정
        monster_manager_->set_websocket_server(websocket_server_);

        // MonsterManager에 PlayerManager 설정
        monster_manager_->set_player_manager(player_manager_);

        // MonsterManager에 BT 엔진 설정
        monster_manager_->set_bt_engine(shared_bt_engine);

        // PlayerManager에 WebSocket 서버 설정
        player_manager_->set_websocket_server(websocket_server_);

        log_message("AsioServer 인스턴스가 생성되었습니다.");
    }

    AsioServer::~AsioServer()
    {
        stop();
        log_message("AsioServer 인스턴스가 소멸되었습니다.");
    }

    bool AsioServer::start()
    {
        if (running_.load())
        {
            log_message("서버가 이미 실행 중입니다.", true);
            return false;
        }

        log_message("서버 시작 중...");

        try
        {
            // 엔드포인트 설정
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(config_.host), config_.port);

            // 어셉터 설정
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen();

            // running_ 플래그를 먼저 설정
            running_.store(true);

            // 서버 시작 시간 기록
            server_start_time_ = std::chrono::steady_clock::now();

            // 연결 수락 시작 (워커 스레드 시작 전에)
            start_accept();

            // 워커 스레드 시작 (async_accept 후에)
            for (size_t i = 0; i < config_.worker_threads; ++i)
            {
                worker_threads_.create_thread(
                    [this, i]()
                    {
                        log_message("워커 스레드 " + std::to_string(i) + " 시작");
                        io_context_.run();
                        log_message("워커 스레드 " + std::to_string(i) + " 종료");
                    });
            }

            // REST API 서버 시작
            if (rest_api_server_)
            {
                rest_api_server_->start();
            }

            // WebSocket 서버 시작
            if (websocket_server_)
            {
                websocket_server_->start();
            }

            log_message("서버가 성공적으로 시작되었습니다. 포트: " + std::to_string(config_.port));
            log_message("REST API 서버: http://localhost:8081");
            log_message("WebSocket 실시간 연결: ws://localhost:8082");

            return true;
        }
        catch (const std::exception& e)
        {
            log_message("서버 시작 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    void AsioServer::stop()
    {
        if (!running_.load())
        {
            return;
        }

        log_message("서버 종료 중...");
        running_.store(false);

        try
        {
            // 어셉터 닫기
            acceptor_.close();

            // 모든 클라이언트 연결 종료
            {
                boost::lock_guard<boost::mutex> lock(clients_mutex_);
                for (auto& [client, info] : clients_)
                {
                    client->stop();
                }
                clients_.clear();
            }

            // IO 컨텍스트 중지
            io_context_.stop();

            // 워커 스레드 종료
            worker_threads_.join_all();

            // REST API 서버 중지
            if (rest_api_server_)
            {
                rest_api_server_->stop();
            }

            // WebSocket 서버 중지
            if (websocket_server_)
            {
                websocket_server_->stop();
            }

            log_message("서버가 종료되었습니다.");
        }
        catch (const std::exception& e)
        {
            log_message("서버 종료 중 오류: " + std::string(e.what()), true);
        }
    }

    void AsioServer::start_accept()
    {
        if (!running_.load())
        {
            log_message("서버가 실행 중이 아니므로 연결 수락을 시작하지 않습니다.", true);
            return;
        }

        auto client = boost::make_shared<AsioClient>(io_context_, this);

        log_message("새로운 클라이언트 연결을 기다리는 중...");
        log_message("async_accept 호출 중...");

        try
        {
            acceptor_.async_accept(
                client->socket(),
                [this, client](const boost::system::error_code& error)
                {
                    log_message("async_accept 콜백 호출됨, 에러: " + (error ? error.message() : "없음"));
                    handle_accept(client, error);
                });
            log_message("async_accept 호출 완료");
        }
        catch (const std::exception& e)
        {
            log_message("async_accept 호출 중 예외 발생: " + std::string(e.what()), true);
        }
    }

    void AsioServer::handle_accept(boost::shared_ptr<AsioClient> client, const boost::system::error_code& error)
    {
        log_message("handle_accept 호출됨, 에러: " + (error ? error.message() : "없음"));

        if (!error)
        {
            log_message("클라이언트 연결 성공, IP: " + client->get_ip_address() + ":" +
                        std::to_string(client->get_port()));

            // 클라이언트 수 제한 확인
            {
                boost::lock_guard<boost::mutex> lock(clients_mutex_);
                if (clients_.size() >= config_.max_clients)
                {
                    log_message("최대 클라이언트 수 초과. 연결 거부", true);
                    client->stop();
                    start_accept();
                    return;
                }
            }

            // 클라이언트 추가
            add_client(client);
            client->start();

            log_message("새 클라이언트 연결 완료: " + client->get_ip_address() + ":" +
                        std::to_string(client->get_port()));
        }
        else
        {
            if (running_.load())
            {
                log_message("연결 수락 오류: " + error.message(), true);
            }
            else
            {
                log_message("서버가 종료 중이므로 연결 수락 오류 무시: " + error.message());
            }
        }

        // 다음 연결 수락 시작
        if (running_.load())
        {
            log_message("다음 클라이언트 연결을 기다리는 중...");
            start_accept();
        }
        else
        {
            log_message("서버가 종료 중이므로 다음 연결 수락을 시작하지 않습니다.");
        }
    }

    void AsioServer::add_client(boost::shared_ptr<AsioClient> client)
    {
        AsioClientInfo info;
        info.client           = client;
        info.ip_address       = client->get_ip_address();
        info.port             = client->get_port();
        info.connect_time     = client->get_connect_time();
        info.is_authenticated = false;
        info.player_id        = 0;
        info.client_type      = "unknown";

        boost::lock_guard<boost::mutex> lock(clients_mutex_);
        clients_[client] = info;

        // 클라이언트 연결 시 플레이어 생성
        if (player_manager_)
        {
            // 클라이언트 ID 생성 (포인터 주소의 해시를 사용)
            uint32_t client_id = static_cast<uint32_t>(std::hash<void*>{}(client.get()));

            // 플레이어 이름 생성 (클라이언트 IP 기반)
            std::string player_name = "Player_" + info.ip_address.substr(info.ip_address.find_last_of('.') + 1);

            // 기본 위치 설정 (스폰 지점)
            MonsterPosition spawn_position = {0.0f, 0.0f, 0.0f, 0.0f};

            // 플레이어 생성
            auto player = player_manager_->create_player_for_client(client_id, player_name, spawn_position);
            if (player)
            {
                info.player_id          = player->get_id();
                std::string client_info = info.ip_address + ":" + std::to_string(info.port);
                log_message("클라이언트 연결: " + client_info + " -> 플레이어 생성: " + player_name +
                            " (ID: " + std::to_string(player->get_id()) + ")");
            }
        }
    }

    void AsioServer::remove_client(boost::shared_ptr<AsioClient> client)
    {
        std::string client_info;
        uint32_t    client_id = 0;

        {
            boost::lock_guard<boost::mutex> lock(clients_mutex_);
            auto                            it = clients_.find(client);
            if (it != clients_.end())
            {
                client_info = it->second.ip_address + ":" + std::to_string(it->second.port);
                client_id   = static_cast<uint32_t>(std::hash<void*>{}(client.get()));
                clients_.erase(it);
            }
        }

        // 클라이언트 연결 해제 시 플레이어 제거
        if (player_manager_ && client_id != 0)
        {
            player_manager_->remove_player_by_client_id(client_id);
        }

        if (!client_info.empty())
        {
            log_message("클라이언트 연결 종료: " + client_info);
        }
    }

    void AsioServer::broadcast_packet(const Packet& packet, boost::shared_ptr<AsioClient> exclude_client)
    {
        boost::lock_guard<boost::mutex> lock(clients_mutex_);

        for (const auto& [client, info] : clients_)
        {
            if (client != exclude_client)
            {
                send_packet(client, packet);
            }
        }
    }

    void AsioServer::send_packet(boost::shared_ptr<AsioClient> client, const Packet& packet)
    {
        if (client && client->is_connected())
        {
            client->send_packet(packet);
            total_packets_sent_.fetch_add(1);
        }
    }

    void AsioServer::process_packet(boost::shared_ptr<AsioClient> client, const Packet& packet)
    {
        total_packets_received_.fetch_add(1);

        // 패킷 타입에 따른 처리
        switch (static_cast<PacketType>(packet.type))
        {
            case PacketType::CONNECT_REQUEST:
                // 연결 요청 처리
                log_message("연결 요청 수신: " + client->get_ip_address());
                // 연결 응답 전송
                send_connect_response(client);
                break;

            case PacketType::MONSTER_SPAWN:
                // 몬스터 스폰 요청 처리
                log_message("몬스터 스폰 요청 수신");
                // 몬스터 스폰 응답 전송
                send_monster_spawn_response(client, true);
                break;

            case PacketType::MONSTER_UPDATE:
                // 몬스터 업데이트 처리
                log_message("몬스터 업데이트 요청 수신");
                // 몬스터 업데이트 응답 전송
                send_monster_update_response(client, true);
                break;

            case PacketType::BT_EXECUTE:
                // Behavior Tree 실행 요청 처리
                log_message("BT 실행 요청 수신");
                if (bt_engine_)
                {
                    // BT 실행 로직
                    send_bt_execute_response(client, true);
                }
                else
                {
                    send_bt_execute_response(client, false);
                }
                break;

            default:
                log_message("알 수 없는 패킷 타입: " + std::to_string(packet.type), true);
                // 에러 응답 전송
                send_error_response(client, "알 수 없는 패킷 타입");
                break;
        }
    }

    void AsioServer::send_connect_response(boost::shared_ptr<AsioClient> client)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success = 1; // 성공
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success, sizeof(uint32_t));

        client->send_packet(response);
        total_packets_sent_.fetch_add(1);
        log_message("연결 응답 전송 완료");
    }

    void AsioServer::send_monster_spawn_response(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->send_packet(response);
        total_packets_sent_.fetch_add(1);
        log_message("몬스터 스폰 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::send_monster_update_response(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->send_packet(response);
        total_packets_sent_.fetch_add(1);
        log_message("몬스터 업데이트 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::send_bt_execute_response(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->send_packet(response);
        total_packets_sent_.fetch_add(1);
        log_message("BT 실행 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::send_error_response(boost::shared_ptr<AsioClient> client, const std::string& error_message)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::ERROR_MESSAGE);
        response.size = sizeof(uint32_t) + error_message.length(); // type + message
        response.data.resize(response.size);

        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), error_message.c_str(), error_message.length());

        client->send_packet(response);
        total_packets_sent_.fetch_add(1);
        log_message("에러 응답 전송 완료: " + error_message);
    }

    size_t AsioServer::get_connected_clients() const
    {
        boost::lock_guard<boost::mutex> lock(clients_mutex_);
        return clients_.size();
    }

    void AsioServer::log_message(const std::string& message, bool is_error)
    {
        boost::lock_guard<boost::mutex> lock(log_mutex_);

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

    // 서버 상태 모니터링을 위한 헬스체크 추가
    bool AsioServer::is_healthy() const
    {
        try
        {
            // 기본적인 서버 상태 확인
            if (!running_.load())
            {
                return false;
            }

            // IO 컨텍스트가 정상적으로 동작하는지 확인
            if (io_context_.stopped())
            {
                return false;
            }

            // 워커 스레드가 정상적으로 동작하는지 확인
            if (worker_threads_.size() == 0)
            {
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            // const 메서드에서는 로그 출력 불가, 단순히 false 반환
            return false;
        }
    }

    // 서버 통계 정보 반환
    ServerHealthInfo AsioServer::get_health_info() const
    {
        ServerHealthInfo info;
        info.is_healthy             = is_healthy();
        info.connected_clients      = get_connected_clients();
        info.total_packets_sent     = total_packets_sent_.load();
        info.total_packets_received = total_packets_received_.load();
        info.worker_threads         = config_.worker_threads;
        info.max_clients            = config_.max_clients;
        info.uptime_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - server_start_time_)
                .count();

        return info;
    }

    // AsioClient 구현
    AsioClient::AsioClient(boost::asio::io_context& io_context, AsioServer* server)
        : io_context_(io_context)
        , socket_(io_context)
        , server_(server)
        , connected_(false)
        , expected_packet_size_(0)
        , sending_(false)
    {
        connect_time_ = boost::chrono::steady_clock::now();
    }

    AsioClient::~AsioClient()
    {
        stop();
    }

    void AsioClient::start()
    {
        connected_.store(true);
        receive_packet();
    }

    void AsioClient::stop()
    {
        if (connected_.load())
        {
            connected_.store(false);

            boost::system::error_code ec;
            socket_.close(ec);

            if (server_)
            {
                server_->remove_client(shared_from_this());
            }
        }
    }

    void AsioClient::send_packet(const Packet& packet)
    {
        if (!connected_.load())
        {
            return;
        }

        boost::lock_guard<boost::mutex> lock(send_queue_mutex_);
        send_queue_.push(packet);

        if (!sending_)
        {
            sending_ = true;
            // 실제 전송은 비동기로 처리
            // 여기서는 큐에 추가만 함
        }
    }

    void AsioClient::receive_packet()
    {
        if (!connected_.load())
        {
            return;
        }

        // 패킷 크기 수신
        boost::asio::async_read(socket_,
                                boost::asio::buffer(&expected_packet_size_, sizeof(expected_packet_size_)),
                                [this](const boost::system::error_code& error, size_t bytes_transferred)
                                {
                                    handle_packet_size(error, bytes_transferred);
                                });
    }

    void AsioClient::handle_packet_size(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error && bytes_transferred == sizeof(expected_packet_size_))
        {
            // 패킷 데이터 수신
            packet_buffer_.resize(expected_packet_size_ - sizeof(uint32_t));

            boost::asio::async_read(socket_,
                                    boost::asio::buffer(packet_buffer_),
                                    [this](const boost::system::error_code& error, size_t bytes_transferred)
                                    {
                                        handle_packet_data(error, bytes_transferred);
                                    });
        }
        else
        {
            handle_disconnect();
        }
    }

    void AsioClient::handle_packet_data(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error && bytes_transferred == packet_buffer_.size())
        {
            // 패킷 파싱
            Packet packet;
            packet.size = expected_packet_size_;
            packet.type = *reinterpret_cast<uint16_t*>(packet_buffer_.data());
            packet.data.assign(packet_buffer_.begin() + sizeof(uint16_t), packet_buffer_.end());

            // 서버에 패킷 전달
            if (server_)
            {
                server_->process_packet(shared_from_this(), packet);
            }

            // 다음 패킷 수신
            receive_packet();
        }
        else
        {
            handle_disconnect();
        }
    }

    void AsioClient::handle_send(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            boost::lock_guard<boost::mutex> lock(send_queue_mutex_);
            send_queue_.pop();

            if (!send_queue_.empty())
            {
                // 다음 패킷 전송
                // 실제 구현에서는 여기서 비동기 전송 처리
            }
            else
            {
                sending_ = false;
            }
        }
        else
        {
            handle_disconnect();
        }
    }

    void AsioClient::handle_disconnect()
    {
        if (connected_.load())
        {
            connected_.store(false);

            if (server_)
            {
                server_->remove_client(shared_from_this());
            }
        }
    }

    std::string AsioClient::get_ip_address() const
    {
        try
        {
            return socket_.remote_endpoint().address().to_string();
        }
        catch (...)
        {
            return "unknown";
        }
    }

    uint16_t AsioClient::get_port() const
    {
        try
        {
            return socket_.remote_endpoint().port();
        }
        catch (...)
        {
            return 0;
        }
    }

} // namespace bt
