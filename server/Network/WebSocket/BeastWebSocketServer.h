#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

namespace bt
{

    // 전방 선언
    class BeastWebSocketSession;

    // Boost.Beast WebSocket 서버 클래스
    class BeastWebSocketServer
    {
    public:
        BeastWebSocketServer(uint16_t port, boost::asio::io_context& ioc);
        ~BeastWebSocketServer();

        // 서버 시작/중지
        bool start();
        void stop();
        bool is_running() const { return running_.load(); }

        // 브로드캐스트 메시지 전송
        void broadcast(const std::string& message);

        // 연결된 클라이언트 수
        size_t get_connected_clients() const;

        // 포트 설정
        void set_port(uint16_t port) { port_ = port; }
        uint16_t get_port() const { return port_; }

        // IO Context 참조
        boost::asio::io_context& get_ioc() { return ioc_; }

    private:
        void do_accept();
        void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

        boost::asio::io_context& ioc_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::endpoint endpoint_;

        uint16_t port_;
        std::atomic<bool> running_;

        // 세션 관리
        std::vector<std::shared_ptr<BeastWebSocketSession>> sessions_;
        mutable std::mutex sessions_mutex_;

        // 서버 스레드
        std::thread server_thread_;
    };

} // namespace bt