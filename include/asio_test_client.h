#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>

namespace bt {

// 테스트 클라이언트 설정
struct AsioClientConfig {
    std::string server_host = "127.0.0.1";
    uint16_t server_port = 8080;
    boost::chrono::milliseconds connection_timeout{5000};
    bool auto_reconnect = false;
    int max_reconnect_attempts = 3;
};

// 패킷 구조체 (서버와 동일)
struct AsioTestPacket {
    uint32_t size;
    uint16_t type;
    std::vector<uint8_t> data;
    
    AsioTestPacket() : size(0), type(0) {}
    AsioTestPacket(uint16_t packet_type, const std::vector<uint8_t>& packet_data) 
        : size(packet_data.size() + sizeof(uint16_t)), type(packet_type), data(packet_data) {}
};

// 패킷 타입 (서버와 동일)
enum class AsioTestPacketType : uint16_t {
    CONNECT_REQUEST = 0x0001,
    CONNECT_RESPONSE = 0x0002,
    DISCONNECT = 0x0003,
    PLAYER_JOIN = 0x1000,
    PLAYER_JOIN_RESPONSE = 0x1001,
    PLAYER_MOVE = 0x2000,
    PLAYER_ATTACK = 0x2001,
    PLAYER_CHAT = 0x2002,
    MONSTER_UPDATE = 0x3000,
    MONSTER_ACTION = 0x3001,
    MONSTER_DEATH = 0x3002,
    BT_EXECUTE = 0x4000,
    BT_RESULT = 0x4001,
    BT_DEBUG = 0x4002,
    ERROR_MESSAGE = 0xFF00
};

// 테스트 클라이언트 클래스
class AsioTestClient {
public:
    AsioTestClient(const AsioClientConfig& config);
    ~AsioTestClient();

    // 연결 관리
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_.load(); }

    // 패킷 송수신
    bool send_packet(const AsioTestPacket& packet);
    bool receive_packet(AsioTestPacket& packet);
    
    // 테스트 시나리오
    bool test_connection();
    bool test_player_join(const std::string& player_name);
    bool test_player_move(float x, float y, float z);
    bool test_player_attack(uint32_t target_id);
    bool test_bt_execute(const std::string& bt_name);
    bool test_monster_update();
    bool test_disconnect();
    
    // 자동 테스트
    bool run_automated_test();
    bool run_stress_test(int num_connections, int duration_seconds);
    
    // 유틸리티
    void set_verbose(bool verbose) { verbose_ = verbose; }
    void log_message(const std::string& message, bool is_error = false);

private:
    // 네트워킹
    bool create_connection();
    void close_connection();
    
    // 패킷 처리
    AsioTestPacket create_connect_request();
    AsioTestPacket create_player_join_packet(const std::string& name);
    AsioTestPacket create_player_move_packet(float x, float y, float z);
    AsioTestPacket create_player_attack_packet(uint32_t target_id);
    AsioTestPacket create_bt_execute_packet(const std::string& bt_name);
    AsioTestPacket create_monster_update_packet();
    AsioTestPacket create_disconnect_packet();
    
    bool parse_packet_response(const AsioTestPacket& packet);
    
    // 테스트 결과
    void record_test_result(const std::string& test_name, bool success, const std::string& message = "");

private:
    AsioClientConfig config_;
    std::atomic<bool> connected_;
    boost::asio::io_context io_context_;
    boost::shared_ptr<boost::asio::ip::tcp::socket> socket_;
    bool verbose_;
    
    // 테스트 통계
    int tests_passed_;
    int tests_failed_;
    std::vector<std::string> test_results_;
    
    // 로깅
    boost::mutex log_mutex_;
};

} // namespace bt
