#include <algorithm>
#include <iostream>
#include <sstream>

#include <cstring>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include "simple_websocket_server.h"

namespace bt
{

    // 정적 멤버 초기화
    std::atomic<uint32_t> SimpleWebSocketSession::next_session_id_{1};

    // SimpleWebSocketSession 구현
    SimpleWebSocketSession::SimpleWebSocketSession(int socket_fd, uint32_t session_id)
        : socket_fd_(socket_fd), connected_(true), session_id_(session_id)
    {
    }

    SimpleWebSocketSession::~SimpleWebSocketSession()
    {
        if (connected_.load())
        {
            close(socket_fd_);
            connected_.store(false);
        }
    }

    bool SimpleWebSocketSession::send(const std::string& message)
    {
        if (!connected_.load())
        {
            return false;
        }

        // 간단한 WebSocket 프레임 생성
        std::string frame = create_websocket_frame(message);

        ssize_t bytes_sent = ::send(socket_fd_, frame.c_str(), frame.length(), 0);
        if (bytes_sent < 0)
        {
            connected_.store(false);
            return false;
        }

        return true;
    }

    std::string SimpleWebSocketSession::create_websocket_frame(const std::string& message)
    {
        std::string frame;

        // FIN=1, Opcode=1 (text frame)
        frame.push_back(0x81);

        // Payload length
        size_t payload_len = message.length();
        if (payload_len < 126)
        {
            frame.push_back(static_cast<char>(payload_len));
        }
        else if (payload_len < 65536)
        {
            frame.push_back(126);
            frame.push_back(static_cast<char>((payload_len >> 8) & 0xFF));
            frame.push_back(static_cast<char>(payload_len & 0xFF));
        }
        else
        {
            frame.push_back(127);
            for (int i = 7; i >= 0; i--)
            {
                frame.push_back(static_cast<char>((payload_len >> (i * 8)) & 0xFF));
            }
        }

        // Payload data
        frame += message;

        return frame;
    }

    // SimpleWebSocketServer 구현
    SimpleWebSocketServer::SimpleWebSocketServer(uint16_t port) : port_(port), running_(false), server_socket_(-1) {}

    SimpleWebSocketServer::~SimpleWebSocketServer()
    {
        stop();
    }

    bool SimpleWebSocketServer::start()
    {
        if (running_.load())
        {
            return true;
        }

        try
        {
            // 소켓 생성
            server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_ < 0)
            {
                std::cerr << "소켓 생성 실패" << std::endl;
                return false;
            }

            // 소켓 옵션 설정
            int opt = 1;
            if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
            {
                std::cerr << "소켓 옵션 설정 실패" << std::endl;
                close(server_socket_);
                return false;
            }

            // 주소 설정
            struct sockaddr_in address;
            address.sin_family      = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port        = htons(port_);

            // 바인딩
            if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0)
            {
                std::cerr << "바인딩 실패" << std::endl;
                close(server_socket_);
                return false;
            }

            // 리스닝
            if (listen(server_socket_, 10) < 0)
            {
                std::cerr << "리스닝 실패" << std::endl;
                close(server_socket_);
                return false;
            }

            running_.store(true);

            // 서버 스레드 시작
            server_thread_ = std::thread(&SimpleWebSocketServer::server_loop, this);

            std::cout << "WebSocket 서버가 포트 " << port_ << "에서 시작되었습니다." << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "WebSocket 서버 시작 실패: " << e.what() << std::endl;
            return false;
        }
    }

    void SimpleWebSocketServer::stop()
    {
        if (!running_.load())
        {
            return;
        }

        std::cout << "WebSocket 서버 종료 중..." << std::endl;
        running_.store(false);

        // 모든 세션 종료
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.clear(); // shared_ptr이 자동으로 소멸자를 호출함
        }

        // 서버 소켓 종료
        if (server_socket_ >= 0)
        {
            close(server_socket_);
            server_socket_ = -1;
        }

        // 서버 스레드 종료
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }

        std::cout << "WebSocket 서버가 종료되었습니다." << std::endl;
    }

    void SimpleWebSocketServer::server_loop()
    {
        std::cout << "WebSocket 서버가 포트 " << port_ << "에서 리스닝 중..." << std::endl;

        while (running_.load())
        {
            // 서버 소켓이 유효한지 확인
            if (server_socket_ < 0 || server_socket_ >= FD_SETSIZE)
            {
                std::cerr << "WebSocket 서버 소켓이 유효하지 않습니다: " << server_socket_ << std::endl;
                break;
            }

            struct sockaddr_in client_address;
            socklen_t          addrlen = sizeof(client_address);

            // 클라이언트 연결 대기 (논블로킹)
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_socket_, &readfds);

            struct timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0;

            int activity = select(server_socket_ + 1, &readfds, NULL, NULL, &timeout);

            if (activity < 0)
            {
                if (running_.load())
                {
                    std::cerr << "select 오류" << std::endl;
                }
                break; // 오류 시 루프 종료
            }

            if (activity == 0)
            {
                // 타임아웃 - running 상태 확인 후 계속
                if (!running_.load())
                {
                    break;
                }
                continue;
            }

            if (FD_ISSET(server_socket_, &readfds))
            {
                int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &addrlen);
                if (client_socket < 0)
                {
                    continue;
                }

                // 클라이언트 처리 스레드 시작
                std::thread client_thread(&SimpleWebSocketServer::handle_client, this, client_socket);
                client_thread.detach();
            }
        }
    }

    void SimpleWebSocketServer::handle_client(int client_socket)
    {
        char buffer[4096] = {0};
        int  bytes_read   = recv(client_socket, buffer, 4096, 0);

        if (bytes_read > 0)
        {
            std::string request(buffer, bytes_read);

            // WebSocket 핸드셰이크 수행
            if (perform_handshake(client_socket, request))
            {
                // 세션 생성 및 추가
                auto session = std::make_shared<SimpleWebSocketSession>(client_socket,
                                                                        SimpleWebSocketSession::get_next_session_id());
                {
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    sessions_.push_back(session);
                }

                std::cout << "WebSocket 클라이언트 연결됨 (세션 ID: " << session->get_id() << ")" << std::endl;

                // 환영 메시지 전송
                std::string welcome_msg =
                    R"({"type":"system_message","data":{"message":"WebSocket 서버에 연결되었습니다.","level":"info"},"timestamp":)" +
                    std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now().time_since_epoch())
                                       .count()) +
                    "}";
                session->send(welcome_msg);

                // 클라이언트 메시지 수신 루프
                while (session->is_connected())
                {
                    char client_buffer[1024] = {0};
                    int  client_bytes        = recv(client_socket, client_buffer, 1024, 0);

                    if (client_bytes <= 0)
                    {
                        break;
                    }

                    // 간단한 echo 응답
                    std::string response = R"({"type":"echo","data":{"message":"메시지를 받았습니다."},"timestamp":)" +
                                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                              std::chrono::steady_clock::now().time_since_epoch())
                                                              .count()) +
                                           "}";
                    session->send(response);
                }

                // 세션 제거
                remove_session(session);
                std::cout << "WebSocket 클라이언트 연결 종료 (세션 ID: " << session->get_id() << ")" << std::endl;
            }
        }

        close(client_socket);
    }

    bool SimpleWebSocketServer::perform_handshake(int client_socket, const std::string& request)
    {
        // WebSocket 핸드셰이크 요청 파싱
        std::istringstream request_stream(request);
        std::string        line;
        std::string        websocket_key;

        while (std::getline(request_stream, line))
        {
            if (line.find("Sec-WebSocket-Key:") != std::string::npos)
            {
                size_t pos = line.find(": ");
                if (pos != std::string::npos)
                {
                    websocket_key = line.substr(pos + 2);
                    // 개행 문자 제거
                    websocket_key.erase(std::remove(websocket_key.begin(), websocket_key.end(), '\r'),
                                        websocket_key.end());
                }
                break;
            }
        }

        if (websocket_key.empty())
        {
            return false;
        }

        // WebSocket 응답 생성
        std::string response_key = generate_websocket_key(websocket_key);

        std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " +
            response_key +
            "\r\n"
            "\r\n";

        send(client_socket, response.c_str(), response.length(), 0);
        return true;
    }

    std::string SimpleWebSocketServer::generate_websocket_key(const std::string& client_key)
    {
        const std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string       combined     = client_key + magic_string;

        // SHA-1 해시 계산
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);

        // Base64 인코딩
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO* bio = BIO_new(BIO_s_mem());
        bio      = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, hash, SHA_DIGEST_LENGTH);
        BIO_flush(bio);

        BUF_MEM* buffer_ptr;
        BIO_get_mem_ptr(bio, &buffer_ptr);

        std::string result(buffer_ptr->data, buffer_ptr->length);

        BIO_free_all(bio);

        return result;
    }

    void SimpleWebSocketServer::broadcast(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto it = sessions_.begin(); it != sessions_.end();)
        {
            if ((*it)->is_connected())
            {
                if ((*it)->send(message))
                {
                    ++it;
                }
                else
                {
                    it = sessions_.erase(it);
                }
            }
            else
            {
                it = sessions_.erase(it);
            }
        }
    }

    size_t SimpleWebSocketServer::get_connected_clients() const
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return sessions_.size();
    }

    void SimpleWebSocketServer::remove_session(std::shared_ptr<SimpleWebSocketSession> session)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), session), sessions_.end());
    }

} // namespace bt
