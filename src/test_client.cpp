#include "test_client.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

namespace bt {

TestClient::TestClient(const ClientConfig& config) 
    : config_(config), connected_(false), socket_fd_(-1), verbose_(false),
      tests_passed_(0), tests_failed_(0) {
    log_message("테스트 클라이언트 생성됨");
}

TestClient::~TestClient() {
    disconnect();
    log_message("테스트 클라이언트 소멸됨");
}

bool TestClient::connect() {
    if (connected_.load()) {
        log_message("이미 연결되어 있습니다", true);
        return true;
    }
    
    log_message("서버에 연결 시도 중: " + config_.server_host + ":" + std::to_string(config_.server_port));
    
    if (!create_socket() || !connect_to_server()) {
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

void TestClient::disconnect() {
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

bool TestClient::create_socket() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        log_message("소켓 생성 실패: " + std::string(strerror(errno)), true);
        return false;
    }
    
    // 소켓 옵션 설정
    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log_message("소켓 옵션 설정 실패: " + std::string(strerror(errno)), true);
        close(socket_fd_);
        return false;
    }
    
    return true;
}

bool TestClient::connect_to_server() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config_.server_port);
    
    if (inet_pton(AF_INET, config_.server_host.c_str(), &server_addr.sin_addr) <= 0) {
        log_message("잘못된 서버 주소: " + config_.server_host, true);
        return false;
    }
    
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        log_message("서버 연결 실패: " + std::string(strerror(errno)), true);
        return false;
    }
    
    return true;
}

void TestClient::close_connection() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool TestClient::send_packet(const Packet& packet) {
    if (!connected_.load()) {
        log_message("연결되지 않은 상태에서 패킷 전송 시도", true);
        return false;
    }
    
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
    
    ssize_t bytes_sent = send(socket_fd_, send_buffer.data(), total_size, 0);
    if (bytes_sent == -1) {
        log_message("패킷 전송 실패: " + std::string(strerror(errno)), true);
        return false;
    }
    
    if (verbose_) {
        log_message("패킷 전송: 타입=" + std::to_string(packet.type) + 
                   ", 크기=" + std::to_string(total_size) + "바이트");
    }
    
    return true;
}

bool TestClient::receive_packet(Packet& packet) {
    if (!connected_.load()) {
        log_message("연결되지 않은 상태에서 패킷 수신 시도", true);
        return false;
    }
    
    // 패킷 크기 수신
    uint32_t packet_size;
    ssize_t bytes_received = recv(socket_fd_, &packet_size, sizeof(packet_size), 0);
    if (bytes_received == -1) {
        log_message("패킷 크기 수신 실패: " + std::string(strerror(errno)), true);
        return false;
    } else if (bytes_received == 0) {
        log_message("서버 연결 종료 감지");
        return false;
    }
    
    if (bytes_received != sizeof(packet_size)) {
        log_message("패킷 크기 수신 불완전", true);
        return false;
    }
    
    // 나머지 패킷 데이터 수신
    std::vector<uint8_t> buffer(packet_size - sizeof(uint32_t));
    bytes_received = recv(socket_fd_, buffer.data(), buffer.size(), 0);
    if (bytes_received == -1) {
        log_message("패킷 데이터 수신 실패: " + std::string(strerror(errno)), true);
        return false;
    }
    
    if (bytes_received != static_cast<ssize_t>(buffer.size())) {
        log_message("패킷 데이터 수신 불완전", true);
        return false;
    }
    
    // 패킷 파싱
    packet.size = packet_size;
    packet.type = *reinterpret_cast<uint16_t*>(buffer.data());
    packet.data.assign(buffer.begin() + sizeof(uint16_t), buffer.end());
    
    if (verbose_) {
        log_message("패킷 수신: 타입=" + std::to_string(packet.type) + 
                   ", 크기=" + std::to_string(packet_size) + "바이트");
    }
    
    return true;
}

bool TestClient::test_connection() {
    log_message("=== 연결 테스트 시작 ===");
    
    bool success = connect();
    record_test_result("연결 테스트", success, success ? "연결 성공" : "연결 실패");
    
    if (success) {
        disconnect();
    }
    
    return success;
}

bool TestClient::test_login(const std::string& username, const std::string& password) {
    log_message("=== 로그인 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("로그인 테스트", false, "연결 실패");
        return false;
    }
    
    // 로그인 요청 전송
    Packet login_packet = create_login_request(username, password);
    if (!send_packet(login_packet)) {
        record_test_result("로그인 테스트", false, "로그인 요청 전송 실패");
        disconnect();
        return false;
    }
    
    // 로그인 응답 수신
    Packet response;
    if (!receive_packet(response)) {
        record_test_result("로그인 테스트", false, "로그인 응답 수신 실패");
        disconnect();
        return false;
    }
    
    bool success = parse_packet_response(response);
    record_test_result("로그인 테스트", success, success ? "로그인 성공" : "로그인 실패");
    
    disconnect();
    return success;
}

bool TestClient::test_player_move(float x, float y, float z) {
    log_message("=== 플레이어 이동 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("플레이어 이동 테스트", false, "연결 실패");
        return false;
    }
    
    // 플레이어 이동 패킷 전송
    Packet move_packet = create_player_move_packet(x, y, z);
    if (!send_packet(move_packet)) {
        record_test_result("플레이어 이동 테스트", false, "이동 패킷 전송 실패");
        disconnect();
        return false;
    }
    
    // 응답 대기 (선택사항)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    record_test_result("플레이어 이동 테스트", true, "이동 패킷 전송 성공");
    disconnect();
    return true;
}

bool TestClient::test_chat_message(const std::string& message) {
    log_message("=== 채팅 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("채팅 테스트", false, "연결 실패");
        return false;
    }
    
    // 채팅 패킷 전송
    Packet chat_packet = create_chat_packet(message);
    if (!send_packet(chat_packet)) {
        record_test_result("채팅 테스트", false, "채팅 패킷 전송 실패");
        disconnect();
        return false;
    }
    
    record_test_result("채팅 테스트", true, "채팅 패킷 전송 성공");
    disconnect();
    return true;
}

bool TestClient::test_disconnect() {
    log_message("=== 연결 해제 테스트 시작 ===");
    
    if (!connect()) {
        record_test_result("연결 해제 테스트", false, "연결 실패");
        return false;
    }
    
    disconnect();
    record_test_result("연결 해제 테스트", true, "연결 해제 성공");
    return true;
}

bool TestClient::run_automated_test() {
    log_message("=== 자동화된 테스트 시작 ===");
    
    tests_passed_ = 0;
    tests_failed_ = 0;
    test_results_.clear();
    
    // 1. 연결 테스트
    test_connection();
    
    // 2. 로그인 테스트
    test_login("testuser", "testpass");
    
    // 3. 플레이어 이동 테스트
    test_player_move(100.0f, 200.0f, 300.0f);
    
    // 4. 채팅 테스트
    test_chat_message("안녕하세요! 테스트 메시지입니다.");
    
    // 5. 연결 해제 테스트
    test_disconnect();
    
    // 테스트 결과 출력
    log_message("=== 테스트 결과 ===");
    log_message("성공: " + std::to_string(tests_passed_) + "개");
    log_message("실패: " + std::to_string(tests_failed_) + "개");
    
    for (const auto& result : test_results_) {
        log_message(result);
    }
    
    bool all_passed = (tests_failed_ == 0);
    log_message("전체 테스트 결과: " + std::string(all_passed ? "성공" : "실패"));
    
    return all_passed;
}

bool TestClient::run_stress_test(int num_connections, int duration_seconds) {
    log_message("=== 스트레스 테스트 시작 ===");
    log_message("연결 수: " + std::to_string(num_connections) + 
               ", 지속 시간: " + std::to_string(duration_seconds) + "초");
    
    std::vector<std::unique_ptr<TestClient>> clients;
    std::vector<std::thread> client_threads;
    
    // 클라이언트 생성 및 연결
    for (int i = 0; i < num_connections; ++i) {
        auto client = std::make_unique<TestClient>(config_);
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

Packet TestClient::create_connect_request() {
    std::vector<uint8_t> data;
    data.push_back(0x01); // 프로토콜 버전
    return Packet(static_cast<uint16_t>(PacketType::CONNECT_REQUEST), data);
}

Packet TestClient::create_login_request(const std::string& username, const std::string& password) {
    std::vector<uint8_t> data;
    
    // 사용자명 길이와 데이터
    uint16_t username_len = username.length();
    data.push_back(username_len & 0xFF);
    data.push_back((username_len >> 8) & 0xFF);
    data.insert(data.end(), username.begin(), username.end());
    
    // 비밀번호 길이와 데이터
    uint16_t password_len = password.length();
    data.push_back(password_len & 0xFF);
    data.push_back((password_len >> 8) & 0xFF);
    data.insert(data.end(), password.begin(), password.end());
    
    return Packet(static_cast<uint16_t>(PacketType::LOGIN_REQUEST), data);
}

Packet TestClient::create_player_move_packet(float x, float y, float z) {
    std::vector<uint8_t> data;
    
    // 좌표 (float를 바이트로 변환)
    uint32_t x_bits = *reinterpret_cast<const uint32_t*>(&x);
    uint32_t y_bits = *reinterpret_cast<const uint32_t*>(&y);
    uint32_t z_bits = *reinterpret_cast<const uint32_t*>(&z);
    
    for (int i = 0; i < 4; ++i) {
        data.push_back((x_bits >> (i * 8)) & 0xFF);
        data.push_back((y_bits >> (i * 8)) & 0xFF);
        data.push_back((z_bits >> (i * 8)) & 0xFF);
    }
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_MOVE), data);
}

Packet TestClient::create_chat_packet(const std::string& message) {
    std::vector<uint8_t> data(message.begin(), message.end());
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_CHAT), data);
}

Packet TestClient::create_disconnect_packet() {
    std::vector<uint8_t> data;
    return Packet(static_cast<uint16_t>(PacketType::DISCONNECT), data);
}

bool TestClient::parse_packet_response(const Packet& packet) {
    if (packet.data.empty()) {
        return false;
    }
    
    // 간단한 응답 파싱 (첫 번째 바이트가 성공/실패)
    return packet.data[0] == 1;
}

void TestClient::record_test_result(const std::string& test_name, bool success, const std::string& message) {
    if (success) {
        tests_passed_++;
        test_results_.push_back("✓ " + test_name + ": " + message);
    } else {
        tests_failed_++;
        test_results_.push_back("✗ " + test_name + ": " + message);
    }
}

void TestClient::log_message(const std::string& message, bool is_error) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
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
