#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <queue>

namespace bt
{

    // HTTP 요청 타입
    using http_request = boost::beast::http::request<boost::beast::http::string_body>;
    using http_response = boost::beast::http::response<boost::beast::http::string_body>;
    using http_status = boost::beast::http::status;

    // HTTP 핸들러 타입
    using http_handler = std::function<void(const http_request&, http_response&)>;

    // Boost.Beast WebSocket 세션 클래스
    class BeastWebSocketSession : public std::enable_shared_from_this<BeastWebSocketSession>
    {
    public:
        BeastWebSocketSession(boost::asio::ip::tcp::socket socket, uint32_t session_id);
        ~BeastWebSocketSession();

        // 세션 시작
        void start();

        // 메시지 전송
        void send_message(const std::string& message);

        // 연결 상태 확인
        bool is_connected() const { return connected_.load(); }

        // 세션 ID
        uint32_t GetID() const { return session_id_; }

        // 정적 ID 생성기
        static uint32_t get_next_session_id() { return next_session_id_++; }

        // WebSocket 스트림 접근 (BeastWebSocketServer에서 사용)
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& get_ws() { return ws_; }
        
        // on_accept 메서드 접근 (BeastWebSocketServer에서 사용)
        void on_accept(boost::beast::error_code ec);

    private:
        void do_read();
        void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
        void do_write();
        void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
        void do_close();

        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
        boost::beast::flat_buffer buffer_;

        std::atomic<bool> connected_;
        uint32_t session_id_;
        static std::atomic<uint32_t> next_session_id_;

        std::queue<std::string> message_queue_;
        std::mutex message_mutex_;
        std::atomic<bool> writing_;
    };

    // 통합 HTTP + WebSocket 서버 클래스
    class BeastHttpWebSocketServer
    {
    public:
        BeastHttpWebSocketServer(uint16_t port, boost::asio::io_context& ioc);
        ~BeastHttpWebSocketServer();

        // 서버 시작/중지
        bool start();
        void stop();
        bool is_running() const { return running_.load(); }

        // WebSocket 브로드캐스트 메시지 전송
        void broadcast(const std::string& message);

        // 연결된 WebSocket 클라이언트 수
        size_t get_connected_clients() const;

        // 포트 설정
        void set_port(uint16_t port) { port_ = port; }
        uint16_t get_port() const { return port_; }

        // IO Context 참조
        boost::asio::io_context& get_ioc() { return ioc_; }

        // HTTP 핸들러 등록
        void register_http_handler(const std::string& path, const std::string& method, http_handler handler);
        void register_get_handler(const std::string& path, http_handler handler);
        void register_post_handler(const std::string& path, http_handler handler);

        // 통계 정보
        size_t get_total_requests() const { return total_requests_.load(); }
        size_t get_websocket_connections() const { return websocket_connections_.load(); }

        // HTTP 응답 생성 (public으로 변경)
        http_response create_http_response(http_status status, const std::string& body, const std::string& content_type = "text/html");
        http_response create_json_response(const std::string& json);
        http_response create_error_response(http_status status, const std::string& message);

    private:
        void do_accept();
        void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
        void handle_http_request(boost::asio::ip::tcp::socket socket, http_request req);
        void handle_websocket_upgrade(boost::asio::ip::tcp::socket socket, http_request req);


        boost::asio::io_context& ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::endpoint endpoint_;

        uint16_t port_;
        std::atomic<bool> running_;

        // HTTP 핸들러 맵
        std::unordered_map<std::string, http_handler> http_handlers_;
        mutable std::mutex handlers_mutex_;

        // WebSocket 세션 관리
        std::vector<std::shared_ptr<BeastWebSocketSession>> sessions_;
        mutable std::mutex sessions_mutex_;

        // 통계
        std::atomic<size_t> total_requests_;
        std::atomic<size_t> websocket_connections_;

        // 서버 스레드
        std::thread server_thread_;
    };

} // namespace bt
