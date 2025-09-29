#pragma once

#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <condition_variable>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace bt {

// 전방 선언
class Client;
class GameWorld;
class PacketHandler;

// 서버 설정 구조체
struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    int max_clients = 1000;
    int worker_threads = 4;
    bool debug_mode = false;
};

// 클라이언트 정보 구조체
struct ClientInfo {
    int socket_fd;
    std::string ip_address;
    uint16_t port;
    std::chrono::steady_clock::time_point connect_time;
    bool is_authenticated;
    uint32_t player_id;
};

// 패킷 구조체
struct Packet {
    uint32_t size;
    uint16_t type;
    std::vector<uint8_t> data;
    
    Packet() : size(0), type(0) {}
    Packet(uint16_t packet_type, const std::vector<uint8_t>& packet_data) 
        : size(packet_data.size() + sizeof(uint16_t)), type(packet_type), data(packet_data) {}
};

// MMORPG 서버 클래스
class Server {
public:
    Server(const ServerConfig& config);
    ~Server();

    // 서버 시작/중지
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }

    // 클라이언트 관리
    void add_client(int socket_fd, const std::string& ip, uint16_t port);
    void remove_client(int socket_fd);
    void broadcast_packet(const Packet& packet, int exclude_socket = -1);
    void send_packet(int socket_fd, const Packet& packet);

    // 게임 월드 접근
    GameWorld* get_game_world() { return game_world_.get(); }

    // 설정 접근
    const ServerConfig& get_config() const { return config_; }

private:
    // 서버 소켓 관련
    bool create_socket();
    bool bind_socket();
    bool listen_socket();
    void accept_connections();
    
    // 클라이언트 처리
    void handle_client(int socket_fd);
    void process_packet(int socket_fd, const Packet& packet);
    
    // 워커 스레드
    void worker_thread_function();
    
    // 유틸리티 함수
    bool set_socket_non_blocking(int socket_fd);
    void log_message(const std::string& message, bool is_error = false);

private:
    ServerConfig config_;
    std::atomic<bool> running_;
    
    // 소켓 관련
    int server_socket_;
    struct sockaddr_in server_addr_;
    
    // 클라이언트 관리
    std::unordered_map<int, std::unique_ptr<ClientInfo>> clients_;
    std::mutex clients_mutex_;
    
    // 워커 스레드
    std::vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_queue_cv_;
    
    // 게임 컴포넌트
    std::unique_ptr<GameWorld> game_world_;
    std::unique_ptr<PacketHandler> packet_handler_;
    
    // 로깅
    std::mutex log_mutex_;
};

} // namespace bt
