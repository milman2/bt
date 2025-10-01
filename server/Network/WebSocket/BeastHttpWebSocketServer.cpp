#include "BeastHttpWebSocketServer.h"
#include <boost/beast/version.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

namespace bt
{

    // BeastWebSocketSession 구현
    BeastWebSocketSession::BeastWebSocketSession(boost::asio::ip::tcp::socket socket, uint32_t session_id)
        : ws_(std::move(socket)), session_id_(session_id), connected_(false)
    {
    }

    BeastWebSocketSession::~BeastWebSocketSession()
    {
        if (connected_.load())
        {
            do_close();
        }
    }

    void BeastWebSocketSession::start()
    {
        // WebSocket 핸드셰이크를 위한 HTTP 요청을 받아야 함
        // 이는 서버에서 처리됨
    }

    void BeastWebSocketSession::on_accept(boost::beast::error_code ec)
    {
        if (ec)
        {
            std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
            return;
        }

        connected_.store(true);
        std::cout << "WebSocket session " << session_id_ << " connected" << std::endl;

        // 연결 메시지 전송
        send_message("{\"type\":\"system_message\",\"data\":{\"message\":\"Beast HTTP+WebSocket 서버에 연결되었습니다.\",\"level\":\"info\"},\"timestamp\":" + 
                    std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) + "}");

        do_read();
    }

    void BeastWebSocketSession::do_read()
    {
        ws_.async_read(
            buffer_,
            [self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred)
            {
                self->on_read(ec, bytes_transferred);
            });
    }

    void BeastWebSocketSession::on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != boost::beast::websocket::error::closed)
            {
                std::cerr << "WebSocket read error: " << ec.message() << std::endl;
            }
            do_close();
            return;
        }

        // 메시지 처리 (현재는 에코만)
        std::string message = boost::beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        // 에코 응답
        if (!message.empty())
        {
            send_message("Echo: " + message);
        }

        do_read();
    }

    void BeastWebSocketSession::send_message(const std::string& message)
    {
        if (!connected_.load())
            return;

        std::lock_guard<std::mutex> lock(message_mutex_);
        message_queue_ = message;

        ws_.async_write(
            boost::asio::buffer(message_queue_),
            [self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
    }

    void BeastWebSocketSession::on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << "WebSocket write error: " << ec.message() << std::endl;
            do_close();
            return;
        }
    }

    void BeastWebSocketSession::do_close()
    {
        if (!connected_.load())
            return;

        connected_.store(false);
        std::cout << "WebSocket session " << session_id_ << " disconnected" << std::endl;

        boost::beast::error_code ec;
        ws_.close(boost::beast::websocket::close_code::normal, ec);
    }

    std::atomic<uint32_t> BeastWebSocketSession::next_session_id_{1};

    // BeastHttpWebSocketServer 구현
    BeastHttpWebSocketServer::BeastHttpWebSocketServer(uint16_t port, boost::asio::io_context& ioc)
        : ioc_(ioc), acceptor_(ioc), port_(port), running_(false), total_requests_(0), websocket_connections_(0)
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }

    BeastHttpWebSocketServer::~BeastHttpWebSocketServer()
    {
        stop();
    }

    bool BeastHttpWebSocketServer::start()
    {
        if (running_.load())
            return false;

        running_.store(true);
        server_thread_ = std::thread([this]() { 
            do_accept();
            ioc_.run();
        });

        std::cout << "Beast HTTP+WebSocket 서버가 포트 " << port_ << "에서 시작되었습니다." << std::endl;
        return true;
    }

    void BeastHttpWebSocketServer::stop()
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

        std::cout << "Beast HTTP+WebSocket 서버가 중지되었습니다." << std::endl;
    }

    void BeastHttpWebSocketServer::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
            {
                on_accept(ec, std::move(socket));
            });
    }

    void BeastHttpWebSocketServer::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        if (ec)
        {
            if (running_.load())
            {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            return;
        }

        // 소켓이 유효한지 확인
        if (!socket.is_open())
        {
            std::cerr << "Socket is not open after accept" << std::endl;
            // 다음 연결을 받기 위해 계속 대기
            if (running_.load())
            {
                do_accept();
            }
            return;
        }

        std::cout << "새로운 연결 수락됨" << std::endl;

        // HTTP 요청 읽기
        auto req = std::make_shared<http_request>();
        auto buffer = std::make_shared<boost::beast::flat_buffer>();
        
        std::cout << "HTTP 요청 읽기 시작..." << std::endl;
        
        boost::beast::http::async_read(
            socket,
            *buffer,
            *req,
            [this, socket = std::move(socket), req, buffer](boost::beast::error_code ec, std::size_t bytes_transferred) mutable
            {
                if (ec)
                {
                    // 연결이 끊어진 경우는 정상적인 상황이므로 로그 레벨을 낮춤
                    if (ec == boost::beast::http::error::end_of_stream || 
                        ec == boost::asio::error::connection_reset ||
                        ec == boost::asio::error::connection_aborted ||
                        ec == boost::asio::error::bad_descriptor)
                    {
                        // 정상적인 연결 종료
                        std::cout << "정상적인 연결 종료: " << ec.message() << std::endl;
                        return;
                    }
                    std::cerr << "HTTP read error: " << ec.message() << " (code: " << ec.value() << ")" << std::endl;
                    return;
                }

                std::cout << "HTTP 요청 읽기 완료: " << bytes_transferred << " bytes" << std::endl;
                std::cout << "요청 메서드: " << req->method_string() << std::endl;
                std::cout << "요청 경로: " << req->target() << std::endl;

                // WebSocket 업그레이드 요청인지 확인
                if (boost::beast::websocket::is_upgrade(*req))
                {
                    std::cout << "WebSocket 업그레이드 요청 감지" << std::endl;
                    handle_websocket_upgrade(std::move(socket), *req);
                }
                else
                {
                    std::cout << "일반 HTTP 요청 처리" << std::endl;
                    handle_http_request(std::move(socket), *req);
                }
            });

        // 다음 연결을 받기 위해 계속 대기
        if (running_.load())
        {
            do_accept();
        }
    }

    void BeastHttpWebSocketServer::handle_http_request(boost::asio::ip::tcp::socket socket, http_request req)
    {
        total_requests_.fetch_add(1);

        http_response res;
        std::string path = std::string(req.target());
        std::string method = std::string(req.method_string());

        std::cout << "HTTP 요청 처리: " << method << " " << path << std::endl;

        // 핸들러 찾기
        std::string handler_key = method + ":" + path;
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        
        std::cout << "핸들러 키: " << handler_key << std::endl;
        std::cout << "등록된 핸들러 수: " << http_handlers_.size() << std::endl;
        
        auto it = http_handlers_.find(handler_key);
        if (it != http_handlers_.end())
        {
            std::cout << "핸들러 찾음, 실행 중..." << std::endl;
            it->second(req, res);
            std::cout << "핸들러 실행 완료" << std::endl;
        }
        else
        {
            std::cout << "핸들러를 찾을 수 없음, 404 응답 생성" << std::endl;
            // 기본 404 응답
            res = create_error_response(boost::beast::http::status::not_found, "Not Found");
        }

        // 응답 전송
        res.version(req.version());
        res.keep_alive(req.keep_alive());

        boost::beast::http::async_write(
            socket,
            res,
            [socket = std::move(socket)](boost::beast::error_code ec, std::size_t bytes_transferred) mutable
            {
                if (ec)
                {
                    std::cerr << "HTTP write error: " << ec.message() << std::endl;
                }
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            });
    }

    void BeastHttpWebSocketServer::handle_websocket_upgrade(boost::asio::ip::tcp::socket socket, http_request req)
    {
        websocket_connections_.fetch_add(1);

        // WebSocket 세션 생성
        auto session = std::make_shared<BeastWebSocketSession>(std::move(socket), BeastWebSocketSession::get_next_session_id());
        
        // 세션을 리스트에 추가
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.push_back(session);
        }

        // WebSocket 핸드셰이크 수행
        session->get_ws().async_accept(
            req,
            [session](boost::beast::error_code ec)
            {
                if (ec)
                {
                    std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
                    return;
                }
                session->on_accept(ec);
            });
    }

    void BeastHttpWebSocketServer::broadcast(const std::string& message)
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

        std::cout << "Beast HTTP+WebSocket 브로드캐스트: " << sessions_.size() << "개 연결 중 " << sent_count << "개 전송 성공" << std::endl;
    }

    size_t BeastHttpWebSocketServer::get_connected_clients() const
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return std::count_if(sessions_.begin(), sessions_.end(),
            [](const std::shared_ptr<BeastWebSocketSession>& session)
            {
                return session && session->is_connected();
            });
    }

    void BeastHttpWebSocketServer::register_http_handler(const std::string& path, const std::string& method, http_handler handler)
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        std::string key = method + ":" + path;
        http_handlers_[key] = handler;
    }

    void BeastHttpWebSocketServer::register_get_handler(const std::string& path, http_handler handler)
    {
        register_http_handler(path, "GET", handler);
    }

    void BeastHttpWebSocketServer::register_post_handler(const std::string& path, http_handler handler)
    {
        register_http_handler(path, "POST", handler);
    }

    http_response BeastHttpWebSocketServer::create_http_response(http_status status, const std::string& body, const std::string& content_type)
    {
        http_response res{status, 11}; // HTTP/1.1
        res.set(boost::beast::http::field::server, "BT-MMORPG-HTTP-WebSocket-Server");
        res.set(boost::beast::http::field::content_type, content_type);
        res.body() = body;
        res.prepare_payload();
        return res;
    }

    http_response BeastHttpWebSocketServer::create_json_response(const std::string& json)
    {
        return create_http_response(boost::beast::http::status::ok, json, "application/json");
    }

    http_response BeastHttpWebSocketServer::create_error_response(http_status status, const std::string& message)
    {
        std::string body = "{\"error\":\"" + message + "\",\"status\":" + std::to_string(static_cast<int>(status)) + "}";
        return create_http_response(status, body, "application/json");
    }

} // namespace bt
