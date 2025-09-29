#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace bt {

// 테스트 클라이언트 설정
struct ClientConfig {
    std::string server_host = "127.0.0.1";
    int server_port = 8080;
    int connection_timeout = 5;
    bool auto_reconnect = false;
    int max_reconnect_attempts = 3;
};

// 패킷 구조체 (서버와 동일)
struct Packet {
    uint32_t size;
    uint16_t type;
    std::vector<uint8_t> data;
    
    Packet() : size(0), type(0) {}
    Packet(uint16_t packet_type, const std::vector<uint8_t>& packet_data) 
        : size(packet_data.size() + sizeof(uint16_t)), type(packet_type), data(packet_data) {}
};

// 패킷 타입 (서버와 동일)
enum class PacketType : uint16_t {
    CONNECT_REQUEST = 0x0001,
    CONNECT_RESPONSE = 0x0002,
    DISCONNECT = 0x0003,
    LOGIN_REQUEST = 0x0100,
    LOGIN_RESPONSE = 0x0101,
    LOGOUT_REQUEST = 0x0102,
    PLAYER_MOVE = 0x0200,
    PLAYER_ATTACK = 0x0201,
    PLAYER_CHAT = 0x0202,
    PLAYER_STATS = 0x0203,
    WORLD_UPDATE = 0x0300,
    MAP_CHANGE = 0x0301,
    NPC_SPAWN = 0x0302,
    NPC_UPDATE = 0x0303,
    ITEM_PICKUP = 0x0400,
    ITEM_DROP = 0x0401,
    INVENTORY_UPDATE = 0x0402,
    CHAT_MESSAGE = 0x0500,
    WHISPER_MESSAGE = 0x0501,
    ERROR_MESSAGE = 0xFF00
};

// 테스트 클라이언트 클래스
class TestClient {
public:
    TestClient(const ClientConfig& config);
    ~TestClient();

    // 연결 관리
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_.load(); }

    // 패킷 송수신
    bool send_packet(const Packet& packet);
    bool receive_packet(Packet& packet);
    
    // 테스트 시나리오
    bool test_connection();
    bool test_login(const std::string& username, const std::string& password);
    bool test_player_move(float x, float y, float z);
    bool test_chat_message(const std::string& message);
    bool test_disconnect();
    
    // 자동 테스트
    bool run_automated_test();
    bool run_stress_test(int num_connections, int duration_seconds);
    
    // 유틸리티
    void set_verbose(bool verbose) { verbose_ = verbose; }
    void log_message(const std::string& message, bool is_error = false);

private:
    // 네트워킹
    bool create_socket();
    bool connect_to_server();
    void close_connection();
    
    // 패킷 처리
    Packet create_connect_request();
    Packet create_login_request(const std::string& username, const std::string& password);
    Packet create_player_move_packet(float x, float y, float z);
    Packet create_chat_packet(const std::string& message);
    Packet create_disconnect_packet();
    
    bool parse_packet_response(const Packet& packet);
    
    // 테스트 결과
    void record_test_result(const std::string& test_name, bool success, const std::string& message = "");

private:
    ClientConfig config_;
    std::atomic<bool> connected_;
    int socket_fd_;
    bool verbose_;
    
    // 테스트 통계
    int tests_passed_;
    int tests_failed_;
    std::vector<std::string> test_results_;
    
    // 로깅
    std::mutex log_mutex_;
};

} // namespace bt
