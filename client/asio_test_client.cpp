#include "asio_test_client.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

namespace bt {

AsioTestClient::AsioTestClient(const AsioClientConfig& config) 
    : config_(config), connected_(false), verbose_(false),
      tests_passed_(0), tests_failed_(0) {
    
    socket_ = boost::make_shared<boost::asio::ip::tcp::socket>(io_context_);
    log_message("AsioTestClient 생성됨");
}

AsioTestClient::~AsioTestClient() {
    disconnect();
    log_message("AsioTestClient 소멸됨");
}

bool AsioTestClient::connect() {
    if (connected_.load()) {
        log_message("이미 연결되어 있습니다", true);
        return true;
    }
    
    log_message("서버에 연결 시도 중: " + config_.server_host + ":" + std::to_string(config_.server_port));
    
    if (!create_connection()) {
        log_message("서버 연결 실패", true);
        return false;
    }
    
    connected_.store(true);
    log_message("서버 연결 성공");
    
    // 연결 요청 패킷 전송
    Packet connect_packet = create_connect_request();
    if (!send_packet(connect_packet)) {
        log_message("연결 요청 패킷 전송 실패", true);
        disconnect();
        return false;
    }
    
    // 연결 응답 대기
    Packet response;
    if (!receive_packet(response)) {
        log_message("연결 응답 수신 실패", true);
        disconnect();
        return false;
    }
    
    if (!parse_packet_response(response)) {
        log_message("연결 응답 파싱 실패", true);
        disconnect();
        return false;
    }
    
    log_message("연결 완료");
    return true;
}

void AsioTestClient::disconnect() {
    if (!connected_.load()) {
        return;
    }
    
    log_message("서버 연결 해제 중");
    
    // 연결 해제 패킷 전송
    Packet disconnect_packet = create_disconnect_packet();
    send_packet(disconnect_packet);
    
    close_connection();
    connected_.store(false);
    
    log_message("서버 연결 해제 완료");
}

bool AsioTestClient::create_connection() {
    try {
        boost::asio::ip::tcp::resolver resolver(io_context_);
        boost::asio::ip::tcp::resolver::results_type endpoints = 
            resolver.resolve(config_.server_host, std::to_string(config_.server_port));
        
        boost::asio::connect(*socket_, endpoints);
        return true;
        
    } catch (const std::exception& e) {
        log_message("연결 생성 실패: " + std::string(e.what()), true);
        return false;
    }
}

void AsioTestClient::close_connection() {
    if (socket_ && socket_->is_open()) {
        try {
            socket_->close();
        } catch (const std::exception& e) {
            log_message("연결 종료 중 오류: " + std::string(e.what()), true);
        }
    }
}

bool AsioTestClient::send_packet(const Packet& packet) {
    if (!connected_.load()) {
        log_message("연결되지 않은 상태에서 패킷 전송 시도", true);
        return false;
    }
    
    try {
        // 패킷 크기 + 타입 + 데이터 전송
        uint32_t total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
        std::vector<uint8_t> send_buffer(total_size);
        
        // 패킷 크기
        *reinterpret_cast<uint32_t*>(send_buffer.data()) = total_size;
        // 패킷 타입
        *reinterpret_cast<uint16_t*>(send_buffer.data() + sizeof(uint32_t)) = packet.type;
        // 패킷 데이터
        std::copy(packet.data.begin(), packet.data.end(), 
                  send_buffer.begin() + sizeof(uint32_t) + sizeof(uint16_t));
        
        boost::asio::write(*socket_, boost::asio::buffer(send_buffer));
        
        if (verbose_) {
            log_message("패킷 전송: 타입=" + std::to_string(packet.type) + 
                       ", 크기=" + std::to_string(total_size) + "바이트");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        log_message("패킷 전송 실패: " + std::string(e.what()), true);
        return false;
    }
}

bool AsioTestClient::receive_packet(Packet& packet) {
    if (!connected_.load()) {
        log_message("연결되지 않은 상태에서 패킷 수신 시도", true);
        return false;
    }
    
    try {
        // 패킷 크기 수신
        uint32_t packet_size;
        boost::asio::read(*socket_, boost::asio::buffer(&packet_size, sizeof(packet_size)));
        
        // 나머지 패킷 데이터 수신
        std::vector<uint8_t> buffer(packet_size - sizeof(uint32_t));
        boost::asio::read(*socket_, boost::asio::buffer(buffer));
        
        // 패킷 파싱
        packet.size = packet_size;
        packet.type = *reinterpret_cast<uint16_t*>(buffer.data());
        packet.data.assign(buffer.begin() + sizeof(uint16_t), buffer.end());
        
        if (verbose_) {
            log_message("패킷 수신: 타입=" + std::to_string(packet.type) + 
                       ", 크기=" + std::to_string(packet_size) + "바이트");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        log_message("패킷 수신 실패: " + std::string(e.what()), true);
        return false;
    }
}

bool AsioTestClient::test_connection() {
    log_message("=== 연결 테스트 시작 ===");
    
    bool success = connect();
    if (success) {
        log_message("연결 성공! 연결 유지 테스트 중...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (is_connected()) {
            log_message("연결 유지 확인됨");
        } else {
            log_message("연결이 끊어짐");
            success = false;
        }
        
        log_message("연결 해제 중...");
        disconnect();
        log_message("연결 테스트 완료");
    } else {
        log_message("연결 실패");
    }
    
    record_test_result("연결 테스트", success, success ? "연결 성공" : "연결 실패");
    return success;
}

bool AsioTestClient::test_player_join(const std::string& player_name) {
    log_message("=== 플레이어 접속 테스트 시작 ===");
    log_message("플레이어 이름: " + player_name);
    
    if (!connect()) {
        record_test_result("플레이어 접속 테스트", false, "연결 실패");
        return false;
    }
    
    log_message("플레이어 접속 요청 전송 중...");
    // 플레이어 접속 요청 전송
    Packet join_packet = create_player_join_packet(player_name);
    if (!send_packet(join_packet)) {
        record_test_result("플레이어 접속 테스트", false, "접속 요청 전송 실패");
        disconnect();
        return false;
    }
    
    log_message("서버 응답 대기 중...");
    // 응답 대기
    Packet response;
    if (!receive_packet(response)) {
        record_test_result("플레이어 접속 테스트", false, "응답 수신 실패");
        disconnect();
        return false;
    }
    
    bool success = parse_packet_response(response);
    log_message("플레이어 접속 " + std::string(success ? "성공" : "실패"));
    record_test_result("플레이어 접속 테스트", success, success ? "접속 성공" : "접속 실패");
    
    log_message("연결 해제 중...");
    disconnect();
    log_message("플레이어 접속 테스트 완료");
    return success;
}

bool AsioTestClient::test_player_move(float x, float y, float z) {
    log_message("=== 플레이어 이동 테스트 시작 ===");
    log_message("이동 좌표: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
    
    if (!connect()) {
        record_test_result("플레이어 이동 테스트", false, "연결 실패");
        return false;
    }
    
    // 플레이어 이동 요청 전송
    Packet move_packet = create_player_move_packet(x, y, z);
    if (!send_packet(move_packet)) {
        record_test_result("플레이어 이동 테스트", false, "이동 요청 전송 실패");
        disconnect();
        return false;
    }
    
    record_test_result("플레이어 이동 테스트", true, "이동 요청 전송 성공");
    disconnect();
    return true;
}

bool AsioTestClient::test_player_attack(uint32_t target_id) {
    log_message("=== 플레이어 공격 테스트 시작 ===");
    log_message("공격 대상 ID: " + std::to_string(target_id));
    
    if (!connect()) {
        record_test_result("플레이어 공격 테스트", false, "연결 실패");
        return false;
    }
    
    // 플레이어 공격 요청 전송
    Packet attack_packet = create_player_attack_packet(target_id);
    if (!send_packet(attack_packet)) {
        record_test_result("플레이어 공격 테스트", false, "공격 요청 전송 실패");
        disconnect();
        return false;
    }
    
    record_test_result("플레이어 공격 테스트", true, "공격 요청 전송 성공");
    disconnect();
    return true;
}

bool AsioTestClient::test_bt_execute(const std::string& bt_name) {
    log_message("=== Behavior Tree 실행 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("BT 실행 테스트", false, "연결 실패");
        return false;
    }
    
    // BT 실행 요청 전송
    Packet bt_packet = create_bt_execute_packet(bt_name);
    if (!send_packet(bt_packet)) {
        record_test_result("BT 실행 테스트", false, "BT 요청 전송 실패");
        disconnect();
        return false;
    }
    
    // 응답 대기
    Packet response;
    if (!receive_packet(response)) {
        record_test_result("BT 실행 테스트", false, "응답 수신 실패");
        disconnect();
        return false;
    }
    
    bool success = parse_packet_response(response);
    record_test_result("BT 실행 테스트", success, success ? "BT 실행 성공" : "BT 실행 실패");
    
    disconnect();
    return success;
}

bool AsioTestClient::test_monster_update() {
    log_message("=== 몬스터 업데이트 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("몬스터 업데이트 테스트", false, "연결 실패");
        return false;
    }
    
    // 몬스터 업데이트 요청 전송
    Packet update_packet = create_monster_update_packet();
    if (!send_packet(update_packet)) {
        record_test_result("몬스터 업데이트 테스트", false, "업데이트 요청 전송 실패");
        disconnect();
        return false;
    }
    
    record_test_result("몬스터 업데이트 테스트", true, "업데이트 요청 전송 성공");
    disconnect();
    return true;
}

bool AsioTestClient::test_disconnect() {
    log_message("=== 연결 해제 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("연결 해제 테스트", false, "연결 실패");
        return false;
    }
    
    disconnect();
    record_test_result("연결 해제 테스트", true, "연결 해제 성공");
    return true;
}

bool AsioTestClient::run_automated_test() {
    log_message("=== 자동화된 테스트 시작 ===");
    log_message("총 5개 테스트를 순차적으로 실행합니다.");
    
    tests_passed_ = 0;
    tests_failed_ = 0;
    test_results_.clear();
    
    // 1. 연결 테스트
    log_message("\n[1/6] 연결 테스트 실행 중...");
    test_connection();
    
    // 2. 플레이어 접속 테스트
    log_message("\n[2/6] 플레이어 접속 테스트 실행 중...");
    test_player_join("test_player");
    
    // 3. 플레이어 이동 테스트
    log_message("\n[3/6] 플레이어 이동 테스트 실행 중...");
    test_player_move(100.0f, 200.0f, 300.0f);
    
    // 4. 플레이어 공격 테스트
    log_message("\n[4/6] 플레이어 공격 테스트 실행 중...");
    test_player_attack(1);
    
    // 5. Behavior Tree 실행 테스트
    log_message("\n[5/6] Behavior Tree 실행 테스트 실행 중...");
    test_bt_execute("goblin_bt");
    
    // 6. 연결 해제 테스트
    log_message("\n[6/6] 연결 해제 테스트 실행 중...");
    test_disconnect();
    
    // 테스트 결과 출력
    log_message("\n=== 테스트 결과 요약 ===");
    log_message("성공: " + std::to_string(tests_passed_) + "개");
    log_message("실패: " + std::to_string(tests_failed_) + "개");
    
    log_message("\n=== 상세 결과 ===");
    for (const auto& result : test_results_) {
        log_message(result);
    }
    
    bool all_passed = (tests_failed_ == 0);
    log_message("\n=== 전체 테스트 결과 ===");
    log_message("결과: " + std::string(all_passed ? "✅ 모든 테스트 성공" : "❌ 일부 테스트 실패"));
    log_message("자동화된 테스트 완료");
    
    return all_passed;
}

bool AsioTestClient::run_stress_test(int num_connections, int duration_seconds) {
    log_message("=== 스트레스 테스트 시작 ===");
    log_message("연결 수: " + std::to_string(num_connections) + 
               ", 지속 시간: " + std::to_string(duration_seconds) + "초");
    
    std::vector<std::unique_ptr<AsioTestClient>> clients;
    std::vector<std::thread> client_threads;
    
    // 클라이언트 생성 및 연결
    for (int i = 0; i < num_connections; ++i) {
        auto client = std::make_unique<AsioTestClient>(config_);
        client->set_verbose(false); // 스트레스 테스트에서는 상세 로그 비활성화
        
        client_threads.emplace_back([&client, i]() {
            if (client->connect()) {
                // 연결 유지
                std::this_thread::sleep_for(std::chrono::seconds(1));
                client->disconnect();
            }
        });
        
        clients.push_back(std::move(client));
    }
    
    // 모든 스레드 완료 대기
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    log_message("스트레스 테스트 완료");
    return true;
}

Packet AsioTestClient::create_connect_request() {
    std::vector<uint8_t> data;
    data.push_back(0x01); // 프로토콜 버전
    return Packet(static_cast<uint16_t>(PacketType::CONNECT_REQUEST), data);
}

Packet AsioTestClient::create_player_join_packet(const std::string& name) {
    std::vector<uint8_t> data;
    
    // 플레이어 이름 길이와 데이터
    uint16_t name_len = name.length();
    data.push_back(name_len & 0xFF);
    data.push_back((name_len >> 8) & 0xFF);
    data.insert(data.end(), name.begin(), name.end());
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_JOIN), data);
}

Packet AsioTestClient::create_player_move_packet(float x, float y, float z) {
    std::vector<uint8_t> data;
    
    // 좌표 (float를 바이트로 변환)
    uint32_t x_bits = *reinterpret_cast<const uint32_t*>(&x);
    uint32_t y_bits = *reinterpret_cast<const uint32_t*>(&y);
    uint32_t z_bits = *reinterpret_cast<const uint32_t*>(&z);
    
    data.push_back(x_bits & 0xFF);
    data.push_back((x_bits >> 8) & 0xFF);
    data.push_back((x_bits >> 16) & 0xFF);
    data.push_back((x_bits >> 24) & 0xFF);
    
    data.push_back(y_bits & 0xFF);
    data.push_back((y_bits >> 8) & 0xFF);
    data.push_back((y_bits >> 16) & 0xFF);
    data.push_back((y_bits >> 24) & 0xFF);
    
    data.push_back(z_bits & 0xFF);
    data.push_back((z_bits >> 8) & 0xFF);
    data.push_back((z_bits >> 16) & 0xFF);
    data.push_back((z_bits >> 24) & 0xFF);
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_MOVE), data);
}

Packet AsioTestClient::create_player_attack_packet(uint32_t target_id) {
    std::vector<uint8_t> data;
    
    // 타겟 ID
    data.push_back(target_id & 0xFF);
    data.push_back((target_id >> 8) & 0xFF);
    data.push_back((target_id >> 16) & 0xFF);
    data.push_back((target_id >> 24) & 0xFF);
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_ATTACK), data);
}

Packet AsioTestClient::create_bt_execute_packet(const std::string& bt_name) {
    std::vector<uint8_t> data;
    
    // BT 이름 길이와 데이터
    uint16_t name_len = bt_name.length();
    data.push_back(name_len & 0xFF);
    data.push_back((name_len >> 8) & 0xFF);
    data.insert(data.end(), bt_name.begin(), bt_name.end());
    
    return Packet(static_cast<uint16_t>(PacketType::BT_EXECUTE), data);
}

Packet AsioTestClient::create_monster_update_packet() {
    std::vector<uint8_t> data;
    data.push_back(0x01); // 업데이트 요청
    return Packet(static_cast<uint16_t>(PacketType::MONSTER_UPDATE), data);
}

Packet AsioTestClient::create_disconnect_packet() {
    std::vector<uint8_t> data;
    return Packet(static_cast<uint16_t>(PacketType::DISCONNECT), data);
}

bool AsioTestClient::parse_packet_response(const Packet& packet) {
    if (packet.data.size() < sizeof(uint32_t)) {
        log_message("응답 패킷 크기가 너무 작음: " + std::to_string(packet.data.size()) + "바이트", true);
        return false;
    }
    
    // 서버 응답 형식: [패킷 타입(4바이트)][성공 여부(4바이트)]
    uint32_t packet_type = *reinterpret_cast<const uint32_t*>(packet.data.data());
    uint32_t success_value = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(uint32_t));
    
    bool success = (success_value == 1);
    
    if (verbose_) {
        log_message("응답 패킷 파싱: 타입=" + std::to_string(packet_type) + 
                   ", 성공=" + std::string(success ? "true" : "false"));
    }
    
    return success;
}

void AsioTestClient::record_test_result(const std::string& test_name, bool success, const std::string& message) {
    if (success) {
        tests_passed_++;
        test_results_.push_back("✓ " + test_name + ": " + message);
    } else {
        tests_failed_++;
        test_results_.push_back("✗ " + test_name + ": " + message);
    }
}

void AsioTestClient::log_message(const std::string& message, bool is_error) {
    boost::lock_guard<boost::mutex> lock(log_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] ";
    if (is_error) {
        std::cout << "[ERROR] ";
    } else {
        std::cout << "[CLIENT] ";
    }
    std::cout << message << std::endl;
}

} // namespace bt
