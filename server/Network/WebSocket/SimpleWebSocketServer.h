#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace bt
{

    // 간단한 WebSocket 세션 클래스
    class SimpleWebSocketSession
    {
    public:
        SimpleWebSocketSession(int socket_fd, uint32_t session_id);
        ~SimpleWebSocketSession();

        // 메시지 전송
        bool send(const std::string& message);

        // 연결 상태 확인
        bool is_connected() const { return connected_.load(); }

        // 세션 ID
        uint32_t GetID() const { return session_id_; }

        // 정적 ID 생성기
        static uint32_t get_next_session_id() { return next_session_id_++; }

    private:
        int                          socket_fd_;
        std::atomic<bool>            connected_;
        uint32_t                     session_id_;
        static std::atomic<uint32_t> next_session_id_;

        // WebSocket 프레임 생성
        std::string create_websocket_frame(const std::string& message);
    };

    // 간단한 WebSocket 서버 클래스
    class SimpleWebSocketServer
    {
    public:
        SimpleWebSocketServer(uint16_t port = 8082);
        ~SimpleWebSocketServer();

        // 서버 시작/중지
        bool start();
        void stop();
        bool is_running() const { return running_.load(); }

        // 브로드캐스트 메시지 전송
        void broadcast(const std::string& message);

        // 연결된 클라이언트 수
        size_t get_connected_clients() const;

        // 포트 설정
        void     set_port(uint16_t port) { port_ = port; }
        uint16_t get_port() const { return port_; }

    private:
        void server_loop();
        void handle_client(int client_socket);
        void remove_session(std::shared_ptr<SimpleWebSocketSession> session);

        // WebSocket 핸드셰이크
        bool        perform_handshake(int client_socket, const std::string& request);
        std::string generate_websocket_key(const std::string& client_key);

        // WebSocket 프레임 처리
        std::string create_websocket_frame(const std::string& message);
        std::string parse_websocket_frame(const std::string& frame);

    private:
        uint16_t          port_;
        std::atomic<bool> running_;

        int         server_socket_;
        std::thread server_thread_;

        // 세션 관리
        std::vector<std::shared_ptr<SimpleWebSocketSession>> sessions_;
        mutable std::mutex                                   sessions_mutex_;
    };

} // namespace bt
