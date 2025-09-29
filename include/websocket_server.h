#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <functional>
#include <nlohmann/json.hpp>

namespace bt {

// WebSocket 세션 클래스
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(boost::asio::ip::tcp::socket socket);
    ~WebSocketSession();

    // 세션 시작
    void start();
    
    // 메시지 전송
    void send(const std::string& message);
    
    // 연결 상태 확인
    bool is_connected() const { return connected_.load(); }
    
    // 세션 ID
    uint32_t get_id() const { return session_id_; }

private:
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void close();

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer buffer_;
    std::atomic<bool> connected_;
    uint32_t session_id_;
    static std::atomic<uint32_t> next_session_id_;
};

// 브로드캐스트 메시지 타입
enum class BroadcastType {
    MONSTER_SPAWN,
    MONSTER_UPDATE,
    MONSTER_DEATH,
    PLAYER_JOIN,
    PLAYER_LEAVE,
    PLAYER_UPDATE,
    SERVER_STATS,
    BT_EXECUTION,
    SYSTEM_MESSAGE
};

// 브로드캐스트 메시지 구조체
struct BroadcastMessage {
    BroadcastType type;
    std::string data;
    std::chrono::steady_clock::time_point timestamp;
    
    BroadcastMessage(BroadcastType t, const std::string& d) 
        : type(t), data(d), timestamp(std::chrono::steady_clock::now()) {}
};

// WebSocket 서버 클래스
class WebSocketServer {
public:
    WebSocketServer(uint16_t port = 8082);
    ~WebSocketServer();

    // 서버 시작/중지
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }

    // 브로드캐스트 메시지 전송
    void broadcast(const BroadcastMessage& message);
    void broadcast_to_all(const std::string& message);
    
    // 특정 타입의 메시지만 브로드캐스트
    void broadcast_monster_update(const nlohmann::json& monster_data);
    void broadcast_player_update(const nlohmann::json& player_data);
    void broadcast_server_stats(const nlohmann::json& stats_data);
    void broadcast_system_message(const std::string& message);
    
    // 연결된 클라이언트 수
    size_t get_connected_clients() const;
    
    // 포트 설정
    void set_port(uint16_t port) { port_ = port; }
    uint16_t get_port() const { return port_; }

private:
    void accept_connections();
    void on_accept(std::shared_ptr<WebSocketSession> session, boost::beast::error_code ec);
    void remove_session(std::shared_ptr<WebSocketSession> session);
    
    // 브로드캐스트 워커 스레드
    void broadcast_worker();

private:
    uint16_t port_;
    std::atomic<bool> running_;
    
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::thread server_thread_;
    
    // 세션 관리
    std::vector<std::shared_ptr<WebSocketSession>> sessions_;
    mutable std::mutex sessions_mutex_;
    
    // 브로드캐스트 큐
    std::queue<BroadcastMessage> broadcast_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread broadcast_thread_;
    std::atomic<bool> stop_broadcast_;
};

// JSON 메시지 생성 헬퍼 함수들
namespace WebSocketMessages {
    nlohmann::json create_monster_spawn_message(const nlohmann::json& monster_data);
    nlohmann::json create_monster_update_message(const nlohmann::json& monster_data);
    nlohmann::json create_monster_death_message(uint32_t monster_id, const std::string& name);
    nlohmann::json create_player_join_message(const nlohmann::json& player_data);
    nlohmann::json create_player_leave_message(uint32_t player_id, const std::string& name);
    nlohmann::json create_player_update_message(const nlohmann::json& player_data);
    nlohmann::json create_server_stats_message(const nlohmann::json& stats_data);
    nlohmann::json create_bt_execution_message(const std::string& monster_name, const std::string& bt_name, const std::string& action);
    nlohmann::json create_system_message(const std::string& message, const std::string& level = "info");
}

} // namespace bt
