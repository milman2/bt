#include "BeastWebSocketServer.h"
#include "BeastHttpWebSocketServer.h" // BeastWebSocketSession 구현을 위해 포함
#include <iostream>

namespace bt
{

    // BeastWebSocketServer 구현
    BeastWebSocketServer::BeastWebSocketServer(uint16_t port, boost::asio::io_context& ioc)
        : ioc_(ioc), acceptor_(ioc), port_(port), running_(false)
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }

    BeastWebSocketServer::~BeastWebSocketServer()
    {
        stop();
    }

    bool BeastWebSocketServer::start()
    {
        if (running_.load())
            return false;

        running_.store(true);
        server_thread_ = std::thread([this]() { do_accept(); });

        std::cout << "Beast WebSocket 서버가 포트 " << port_ << "에서 시작되었습니다." << std::endl;
        return true;
    }

    void BeastWebSocketServer::stop()
    {
        if (!running_.load())
            return;

        running_.store(false);

        boost::beast::error_code ec;
        acceptor_.close(ec);

        // 모든 WebSocket 세션 종료
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& session : sessions_)
            {
                if (session && session->is_connected())
                {
                    // 세션 종료는 소멸자에서 처리됨
                }
            }
            sessions_.clear();
        }

        if (server_thread_.joinable())
        {
            server_thread_.join();
        }

        std::cout << "Beast WebSocket 서버가 중지되었습니다." << std::endl;
    }

    void BeastWebSocketServer::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
            {
                on_accept(ec, std::move(socket));
            });
    }

    void BeastWebSocketServer::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        if (ec)
        {
            if (running_.load())
            {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            return;
        }

        // WebSocket 세션 생성
        auto session = std::make_shared<BeastWebSocketSession>(std::move(socket), BeastWebSocketSession::get_next_session_id());
        
        // 세션을 리스트에 추가
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.push_back(session);
        }

        // WebSocket 핸드셰이크 수행
        session->get_ws().async_accept(
            [session](boost::beast::error_code ec)
            {
                if (ec)
                {
                    std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
                    return;
                }
                session->on_accept(ec);
            });

        // 다음 연결을 받기 위해 계속 대기
        if (running_.load())
        {
            do_accept();
        }
    }

    void BeastWebSocketServer::broadcast(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        
        // 연결이 끊어진 세션들을 정리
        sessions_.erase(
            std::remove_if(sessions_.begin(), sessions_.end(),
                [](const std::shared_ptr<BeastWebSocketSession>& session)
                {
                    return !session || !session->is_connected();
                }),
            sessions_.end());

        // 모든 연결된 세션에 메시지 전송
        size_t sent_count = 0;
        for (auto& session : sessions_)
        {
            if (session && session->is_connected())
            {
                session->send_message(message);
                sent_count++;
            }
        }

        std::cout << "Beast WebSocket 브로드캐스트: " << sessions_.size() << "개 연결 중 " << sent_count << "개 전송 성공" << std::endl;
    }

    size_t BeastWebSocketServer::get_connected_clients() const
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return std::count_if(sessions_.begin(), sessions_.end(),
            [](const std::shared_ptr<BeastWebSocketSession>& session)
            {
                return session && session->is_connected();
            });
    }

} // namespace bt