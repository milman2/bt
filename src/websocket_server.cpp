#include "websocket_server.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace bt {

// 정적 멤버 초기화
std::atomic<uint32_t> WebSocketSession::next_session_id_{1};

// WebSocketSession 구현
WebSocketSession::WebSocketSession(boost::asio::ip::tcp::socket socket)
    : ws_(std::move(socket)), connected_(false), session_id_(next_session_id_++) {
}

WebSocketSession::~WebSocketSession() {
    close();
}

void WebSocketSession::start() {
    // WebSocket 핸드셰이크 수행
    ws_.async_accept(
        boost::beast::bind_front_handler(
            &WebSocketSession::on_accept,
            shared_from_this()
        )
    );
}

void WebSocketSession::on_accept(boost::beast::error_code ec) {
    if (ec) {
        std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
        return;
    }
    
    connected_.store(true);
    std::cout << "WebSocket 세션 " << session_id_ << " 연결됨" << std::endl;
    
    // 메시지 수신 시작
    do_read();
}

void WebSocketSession::do_read() {
    ws_.async_read(
        buffer_,
        boost::beast::bind_front_handler(
            &WebSocketSession::on_read,
            shared_from_this()
        )
    );
}

void WebSocketSession::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        if (ec == boost::beast::websocket::error::closed) {
            std::cout << "WebSocket 세션 " << session_id_ << " 정상 종료" << std::endl;
        } else {
            std::cerr << "WebSocket read error: " << ec.message() << std::endl;
        }
        close();
        return;
    }
    
    // 받은 메시지 처리 (현재는 echo만 구현)
    std::string message = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    
    std::cout << "WebSocket 세션 " << session_id_ << "에서 메시지 수신: " << message << std::endl;
    
    // 다음 메시지 수신 대기
    do_read();
}

void WebSocketSession::send(const std::string& message) {
    if (!connected_.load()) {
        return;
    }
    
    // 비동기 전송
    ws_.async_write(
        boost::asio::buffer(message),
        boost::beast::bind_front_handler(
            &WebSocketSession::on_write,
            shared_from_this()
        )
    );
}

void WebSocketSession::on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        std::cerr << "WebSocket write error: " << ec.message() << std::endl;
        close();
    }
}

void WebSocketSession::close() {
    if (connected_.exchange(false)) {
        boost::beast::error_code ec;
        ws_.close(boost::beast::websocket::close_code::normal, ec);
        std::cout << "WebSocket 세션 " << session_id_ << " 연결 종료" << std::endl;
    }
}

// WebSocketServer 구현
WebSocketServer::WebSocketServer(uint16_t port) 
    : port_(port), running_(false), acceptor_(io_context_), stop_broadcast_(false) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    if (running_.load()) {
        return true;
    }
    
    try {
        // 엔드포인트 설정
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        
        running_.store(true);
        
        // 서버 스레드 시작
        server_thread_ = std::thread([this]() {
            std::cout << "WebSocket 서버가 포트 " << port_ << "에서 시작되었습니다." << std::endl;
            io_context_.run();
        });
        
        // 브로드캐스트 스레드 시작
        broadcast_thread_ = std::thread([this]() {
            broadcast_worker();
        });
        
        // 연결 수락 시작
        accept_connections();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "WebSocket 서버 시작 실패: " << e.what() << std::endl;
        return false;
    }
}

void WebSocketServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "WebSocket 서버 종료 중..." << std::endl;
    running_.store(false);
    stop_broadcast_.store(true);
    
    // 모든 세션 종료
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& session : sessions_) {
            session->close();
        }
        sessions_.clear();
    }
    
    // 브로드캐스트 스레드 종료
    queue_cv_.notify_all();
    if (broadcast_thread_.joinable()) {
        broadcast_thread_.join();
    }
    
    // 서버 스레드 종료
    io_context_.stop();
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    std::cout << "WebSocket 서버가 종료되었습니다." << std::endl;
}

void WebSocketServer::accept_connections() {
    auto session = std::make_shared<WebSocketSession>(
        boost::asio::ip::tcp::socket(io_context_)
    );
    
    acceptor_.async_accept(
        session->socket(),
        boost::beast::bind_front_handler(
            &WebSocketServer::on_accept,
            this,
            session
        )
    );
}

void WebSocketServer::on_accept(std::shared_ptr<WebSocketSession> session, boost::beast::error_code ec) {
    if (ec) {
        std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
        return;
    }
    
    // 세션을 리스트에 추가
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.push_back(session);
    }
    
    // 세션 시작
    session->start();
    
    // 다음 연결 수락
    if (running_.load()) {
        accept_connections();
    }
}

void WebSocketServer::remove_session(std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(
        std::remove(sessions_.begin(), sessions_.end(), session),
        sessions_.end()
    );
}

void WebSocketServer::broadcast(const BroadcastMessage& message) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        broadcast_queue_.push(message);
    }
    queue_cv_.notify_one();
}

void WebSocketServer::broadcast_to_all(const std::string& message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& session : sessions_) {
        if (session->is_connected()) {
            session->send(message);
        }
    }
}

void WebSocketServer::broadcast_monster_update(const nlohmann::json& monster_data) {
    auto message = WebSocketMessages::create_monster_update_message(monster_data);
    broadcast(BroadcastMessage(BroadcastType::MONSTER_UPDATE, message.dump()));
}

void WebSocketServer::broadcast_player_update(const nlohmann::json& player_data) {
    auto message = WebSocketMessages::create_player_update_message(player_data);
    broadcast(BroadcastMessage(BroadcastType::PLAYER_UPDATE, message.dump()));
}

void WebSocketServer::broadcast_server_stats(const nlohmann::json& stats_data) {
    auto message = WebSocketMessages::create_server_stats_message(stats_data);
    broadcast(BroadcastMessage(BroadcastType::SERVER_STATS, message.dump()));
}

void WebSocketServer::broadcast_system_message(const std::string& message) {
    auto json_message = WebSocketMessages::create_system_message(message);
    broadcast(BroadcastMessage(BroadcastType::SYSTEM_MESSAGE, json_message.dump()));
}

size_t WebSocketServer::get_connected_clients() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

void WebSocketServer::broadcast_worker() {
    while (!stop_broadcast_.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !broadcast_queue_.empty() || stop_broadcast_.load(); });
        
        if (stop_broadcast_.load()) {
            break;
        }
        
        if (!broadcast_queue_.empty()) {
            auto message = broadcast_queue_.front();
            broadcast_queue_.pop();
            lock.unlock();
            
            // 모든 연결된 클라이언트에게 메시지 전송
            std::lock_guard<std::mutex> sessions_lock(sessions_mutex_);
            for (auto it = sessions_.begin(); it != sessions_.end();) {
                if ((*it)->is_connected()) {
                    (*it)->send(message.data);
                    ++it;
                } else {
                    it = sessions_.erase(it);
                }
            }
        }
    }
}

// JSON 메시지 생성 헬퍼 함수들
namespace WebSocketMessages {
    
nlohmann::json create_monster_spawn_message(const nlohmann::json& monster_data) {
    return {
        {"type", "monster_spawn"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", monster_data}
    };
}

nlohmann::json create_monster_update_message(const nlohmann::json& monster_data) {
    return {
        {"type", "monster_update"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", monster_data}
    };
}

nlohmann::json create_monster_death_message(uint32_t monster_id, const std::string& name) {
    return {
        {"type", "monster_death"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", {
            {"id", monster_id},
            {"name", name}
        }}
    };
}

nlohmann::json create_player_join_message(const nlohmann::json& player_data) {
    return {
        {"type", "player_join"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", player_data}
    };
}

nlohmann::json create_player_leave_message(uint32_t player_id, const std::string& name) {
    return {
        {"type", "player_leave"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", {
            {"id", player_id},
            {"name", name}
        }}
    };
}

nlohmann::json create_player_update_message(const nlohmann::json& player_data) {
    return {
        {"type", "player_update"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", player_data}
    };
}

nlohmann::json create_server_stats_message(const nlohmann::json& stats_data) {
    return {
        {"type", "server_stats"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", stats_data}
    };
}

nlohmann::json create_bt_execution_message(const std::string& monster_name, const std::string& bt_name, const std::string& action) {
    return {
        {"type", "bt_execution"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", {
            {"monster_name", monster_name},
            {"bt_name", bt_name},
            {"action", action}
        }}
    };
}

nlohmann::json create_system_message(const std::string& message, const std::string& level) {
    return {
        {"type", "system_message"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()},
        {"data", {
            {"message", message},
            {"level", level}
        }}
    };
}

} // namespace WebSocketMessages

} // namespace bt
