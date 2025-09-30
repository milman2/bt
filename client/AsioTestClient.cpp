#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "AsioTestClient.h"

namespace bt
{

    AsioTestClient::AsioTestClient(const AsioClientConfig& config)
        : config_(config), connected_(false), verbose_(false), tests_passed_(0), tests_failed_(0)
    {
        socket_ = boost::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        LogMessage("AsioTestClient 생성됨");
    }

    AsioTestClient::~AsioTestClient()
    {
        Disconnect();
        LogMessage("AsioTestClient 소멸됨");
    }

    bool AsioTestClient::Connect()
    {
        if (connected_.load())
        {
            LogMessage("이미 연결되어 있습니다", true);
            return true;
        }

        LogMessage("서버에 연결 시도 중: " + config_.server_host + ":" + std::to_string(config_.server_port));

        if (!CreateConnection())
        {
            LogMessage("서버 연결 실패", true);
            return false;
        }

        connected_.store(true);
        LogMessage("서버 연결 성공");

        // 연결 요청 패킷 전송
        Packet connect_packet = CreateConnectRequest();
        if (!SendPacket(connect_packet))
        {
            LogMessage("연결 요청 패킷 전송 실패", true);
            Disconnect();
            return false;
        }

        // 연결 응답 대기
        Packet response;
        if (!ReceivePacket(response))
        {
            LogMessage("연결 응답 수신 실패", true);
            Disconnect();
            return false;
        }

        if (!ParsePacketResponse(response))
        {
            LogMessage("연결 응답 파싱 실패", true);
            Disconnect();
            return false;
        }

        LogMessage("연결 완료");
        return true;
    }

    void AsioTestClient::Disconnect()
    {
        if (!connected_.load())
        {
            return;
        }

        LogMessage("서버 연결 해제 중");

        // 연결 해제 패킷 전송
        Packet disconnect_packet = CreateDisconnectPacket();
        SendPacket(disconnect_packet);

        CloseConnection();
        connected_.store(false);

        LogMessage("서버 연결 해제 완료");
    }

    bool AsioTestClient::CreateConnection()
    {
        try
        {
            boost::asio::ip::tcp::resolver               resolver(io_context_);
            boost::asio::ip::tcp::resolver::results_type endpoints =
                resolver.resolve(config_.server_host, std::to_string(config_.server_port));

            boost::asio::connect(*socket_, endpoints);
            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("연결 생성 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    void AsioTestClient::CloseConnection()
    {
        if (socket_ && socket_->is_open())
        {
            try
            {
                socket_->close();
            }
            catch (const std::exception& e)
            {
                LogMessage("연결 종료 중 오류: " + std::string(e.what()), true);
            }
        }
    }

    bool AsioTestClient::SendPacket(const Packet& packet)
    {
        if (!connected_.load())
        {
            LogMessage("연결되지 않은 상태에서 패킷 전송 시도", true);
            return false;
        }

        try
        {
            // 패킷 크기 + 타입 + 데이터 전송
            uint32_t             total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
            std::vector<uint8_t> send_buffer(total_size);

            // 패킷 크기
            *reinterpret_cast<uint32_t*>(send_buffer.data()) = total_size;
            // 패킷 타입
            *reinterpret_cast<uint16_t*>(send_buffer.data() + sizeof(uint32_t)) = packet.type;
            // 패킷 데이터
            std::copy(
                packet.data.begin(), packet.data.end(), send_buffer.begin() + sizeof(uint32_t) + sizeof(uint16_t));

            boost::asio::write(*socket_, boost::asio::buffer(send_buffer));

            if (verbose_)
            {
                LogMessage("패킷 전송: 타입=" + std::to_string(packet.type) + ", 크기=" + std::to_string(total_size) +
                            "바이트");
            }

            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("패킷 전송 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    bool AsioTestClient::ReceivePacket(Packet& packet)
    {
        if (!connected_.load())
        {
            LogMessage("연결되지 않은 상태에서 패킷 수신 시도", true);
            return false;
        }

        try
        {
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

            if (verbose_)
            {
                LogMessage("패킷 수신: 타입=" + std::to_string(packet.type) + ", 크기=" + std::to_string(packet_size) +
                            "바이트");
            }

            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("패킷 수신 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    bool AsioTestClient::TestConnection()
    {
        LogMessage("=== 연결 테스트 시작 ===");

        bool success = Connect();
        if (success)
        {
            LogMessage("연결 성공! 연결 유지 테스트 중...");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (IsConnected())
            {
                LogMessage("연결 유지 확인됨");
            }
            else
            {
                LogMessage("연결이 끊어짐");
                success = false;
            }

            LogMessage("연결 해제 중...");
            Disconnect();
            LogMessage("연결 테스트 완료");
        }
        else
        {
            LogMessage("연결 실패");
        }

        RecordTestResult("연결 테스트", success, success ? "연결 성공" : "연결 실패");
        return success;
    }

    bool AsioTestClient::TestPlayerJoin(const std::string& player_name)
    {
        LogMessage("=== 플레이어 접속 테스트 시작 ===");
        LogMessage("플레이어 이름: " + player_name);

        if (!Connect())
        {
            RecordTestResult("플레이어 접속 테스트", false, "연결 실패");
            return false;
        }

        LogMessage("플레이어 접속 요청 전송 중...");
        // 플레이어 접속 요청 전송
        Packet join_packet = CreatePlayerJoinPacket(player_name);
        if (!SendPacket(join_packet))
        {
            RecordTestResult("플레이어 접속 테스트", false, "접속 요청 전송 실패");
            Disconnect();
            return false;
        }

        LogMessage("서버 응답 대기 중...");
        // 응답 대기
        Packet response;
        if (!ReceivePacket(response))
        {
            RecordTestResult("플레이어 접속 테스트", false, "응답 수신 실패");
            Disconnect();
            return false;
        }

        bool success = ParsePacketResponse(response);
        LogMessage("플레이어 접속 " + std::string(success ? "성공" : "실패"));
        RecordTestResult("플레이어 접속 테스트", success, success ? "접속 성공" : "접속 실패");

        LogMessage("연결 해제 중...");
        Disconnect();
        LogMessage("플레이어 접속 테스트 완료");
        return success;
    }

    bool AsioTestClient::TestPlayerMove(float x, float y, float z)
    {
        LogMessage("=== 플레이어 이동 테스트 시작 ===");
        LogMessage("이동 좌표: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");

        if (!Connect())
        {
            RecordTestResult("플레이어 이동 테스트", false, "연결 실패");
            return false;
        }

        // 플레이어 이동 요청 전송
        Packet move_packet = CreatePlayerMovePacket(x, y, z);
        if (!SendPacket(move_packet))
        {
            RecordTestResult("플레이어 이동 테스트", false, "이동 요청 전송 실패");
            Disconnect();
            return false;
        }

        RecordTestResult("플레이어 이동 테스트", true, "이동 요청 전송 성공");
        Disconnect();
        return true;
    }

    bool AsioTestClient::TestPlayerAttack(uint32_t target_id)
    {
        LogMessage("=== 플레이어 공격 테스트 시작 ===");
        LogMessage("공격 대상 ID: " + std::to_string(target_id));

        if (!Connect())
        {
            RecordTestResult("플레이어 공격 테스트", false, "연결 실패");
            return false;
        }

        // 플레이어 공격 요청 전송
        Packet attack_packet = CreatePlayerAttackPacket(target_id);
        if (!SendPacket(attack_packet))
        {
            RecordTestResult("플레이어 공격 테스트", false, "공격 요청 전송 실패");
            Disconnect();
            return false;
        }

        RecordTestResult("플레이어 공격 테스트", true, "공격 요청 전송 성공");
        Disconnect();
        return true;
    }

    bool AsioTestClient::TestBTExecute(const std::string& bt_name)
    {
        LogMessage("=== Behavior Tree 실행 테스트 시작 ===");

        if (!Connect())
        {
            RecordTestResult("BT 실행 테스트", false, "연결 실패");
            return false;
        }

        // BT 실행 요청 전송
        Packet bt_packet = CreateBTExecutePacket(bt_name);
        if (!SendPacket(bt_packet))
        {
            RecordTestResult("BT 실행 테스트", false, "BT 요청 전송 실패");
            Disconnect();
            return false;
        }

        // 응답 대기
        Packet response;
        if (!ReceivePacket(response))
        {
            RecordTestResult("BT 실행 테스트", false, "응답 수신 실패");
            Disconnect();
            return false;
        }

        bool success = ParsePacketResponse(response);
        RecordTestResult("BT 실행 테스트", success, success ? "BT 실행 성공" : "BT 실행 실패");

        Disconnect();
        return success;
    }

    bool AsioTestClient::TestMonsterUpdate()
    {
        LogMessage("=== 몬스터 업데이트 테스트 시작 ===");

        if (!Connect())
        {
            RecordTestResult("몬스터 업데이트 테스트", false, "연결 실패");
            return false;
        }

        // 몬스터 업데이트 요청 전송
        Packet update_packet = CreateMonsterUpdatePacket();
        if (!SendPacket(update_packet))
        {
            RecordTestResult("몬스터 업데이트 테스트", false, "업데이트 요청 전송 실패");
            Disconnect();
            return false;
        }

        RecordTestResult("몬스터 업데이트 테스트", true, "업데이트 요청 전송 성공");
        Disconnect();
        return true;
    }

    bool AsioTestClient::TestDisconnect()
    {
        LogMessage("=== 연결 해제 테스트 시작 ===");

        if (!Connect())
        {
            RecordTestResult("연결 해제 테스트", false, "연결 실패");
            return false;
        }

        Disconnect();
        RecordTestResult("연결 해제 테스트", true, "연결 해제 성공");
        return true;
    }

    bool AsioTestClient::RunAutomatedTest()
    {
        LogMessage("=== 자동화된 테스트 시작 ===");
        LogMessage("총 5개 테스트를 순차적으로 실행합니다.");

        tests_passed_ = 0;
        tests_failed_ = 0;
        test_results_.clear();

        // 1. 연결 테스트
        LogMessage("\n[1/6] 연결 테스트 실행 중...");
        TestConnection();

        // 2. 플레이어 접속 테스트
        LogMessage("\n[2/6] 플레이어 접속 테스트 실행 중...");
        TestPlayerJoin("test_player");

        // 3. 플레이어 이동 테스트
        LogMessage("\n[3/6] 플레이어 이동 테스트 실행 중...");
        TestPlayerMove(100.0f, 200.0f, 300.0f);

        // 4. 플레이어 공격 테스트
        LogMessage("\n[4/6] 플레이어 공격 테스트 실행 중...");
        TestPlayerAttack(1);

        // 5. Behavior Tree 실행 테스트
        LogMessage("\n[5/6] Behavior Tree 실행 테스트 실행 중...");
        TestBTExecute("goblin_bt");

        // 6. 연결 해제 테스트
        LogMessage("\n[6/6] 연결 해제 테스트 실행 중...");
        TestDisconnect();

        // 테스트 결과 출력
        LogMessage("\n=== 테스트 결과 요약 ===");
        LogMessage("성공: " + std::to_string(tests_passed_) + "개");
        LogMessage("실패: " + std::to_string(tests_failed_) + "개");

        LogMessage("\n=== 상세 결과 ===");
        for (const auto& result : test_results_)
        {
            LogMessage(result);
        }

        bool all_passed = (tests_failed_ == 0);
        LogMessage("\n=== 전체 테스트 결과 ===");
        LogMessage("결과: " + std::string(all_passed ? "✅ 모든 테스트 성공" : "❌ 일부 테스트 실패"));
        LogMessage("자동화된 테스트 완료");

        return all_passed;
    }

    bool AsioTestClient::RunStressTest(int num_connections, int duration_seconds)
    {
        LogMessage("=== 스트레스 테스트 시작 ===");
        LogMessage("연결 수: " + std::to_string(num_connections) + ", 지속 시간: " + std::to_string(duration_seconds) +
                    "초");

        std::vector<std::unique_ptr<AsioTestClient>> clients;
        std::vector<std::thread>                     client_threads;

        // 클라이언트 생성 및 연결
        for (int i = 0; i < num_connections; ++i)
        {
            auto client = std::make_unique<AsioTestClient>(config_);
            client->SetVerbose(false); // 스트레스 테스트에서는 상세 로그 비활성화

            client_threads.emplace_back(
                [&client]()
                {
                    if (client->Connect())
                    {
                        // 연결 유지
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        client->Disconnect();
                    }
                });

            clients.push_back(std::move(client));
        }

        // 모든 스레드 완료 대기
        for (auto& thread : client_threads)
        {
            thread.join();
        }

        LogMessage("스트레스 테스트 완료");
        return true;
    }

    Packet AsioTestClient::CreateConnectRequest()
    {
        std::vector<uint8_t> data;
        data.push_back(0x01); // 프로토콜 버전
        return Packet(static_cast<uint16_t>(PacketType::CONNECT_REQUEST), data);
    }

    Packet AsioTestClient::CreatePlayerJoinPacket(const std::string& name)
    {
        std::vector<uint8_t> data;

        // 플레이어 이름 길이와 데이터
        uint16_t name_len = name.length();
        data.push_back(name_len & 0xFF);
        data.push_back((name_len >> 8) & 0xFF);
        data.insert(data.end(), name.begin(), name.end());

        return Packet(static_cast<uint16_t>(PacketType::PLAYER_JOIN), data);
    }

    Packet AsioTestClient::CreatePlayerMovePacket(float x, float y, float z)
    {
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

    Packet AsioTestClient::CreatePlayerAttackPacket(uint32_t target_id)
    {
        std::vector<uint8_t> data;

        // 타겟 ID
        data.push_back(target_id & 0xFF);
        data.push_back((target_id >> 8) & 0xFF);
        data.push_back((target_id >> 16) & 0xFF);
        data.push_back((target_id >> 24) & 0xFF);

        return Packet(static_cast<uint16_t>(PacketType::PLAYER_ATTACK), data);
    }

    Packet AsioTestClient::CreateBTExecutePacket(const std::string& bt_name)
    {
        std::vector<uint8_t> data;

        // BT 이름 길이와 데이터
        uint16_t name_len = bt_name.length();
        data.push_back(name_len & 0xFF);
        data.push_back((name_len >> 8) & 0xFF);
        data.insert(data.end(), bt_name.begin(), bt_name.end());

        return Packet(static_cast<uint16_t>(PacketType::BT_EXECUTE), data);
    }

    Packet AsioTestClient::CreateMonsterUpdatePacket()
    {
        std::vector<uint8_t> data;
        data.push_back(0x01); // 업데이트 요청
        return Packet(static_cast<uint16_t>(PacketType::MONSTER_UPDATE), data);
    }

    Packet AsioTestClient::CreateDisconnectPacket()
    {
        std::vector<uint8_t> data;
        return Packet(static_cast<uint16_t>(PacketType::DISCONNECT), data);
    }

    bool AsioTestClient::ParsePacketResponse(const Packet& packet)
    {
        if (packet.data.size() < sizeof(uint32_t))
        {
            LogMessage("응답 패킷 크기가 너무 작음: " + std::to_string(packet.data.size()) + "바이트", true);
            return false;
        }

        // 서버 응답 형식: [패킷 타입(4바이트)][성공 여부(4바이트)]
        uint32_t packet_type   = *reinterpret_cast<const uint32_t*>(packet.data.data());
        uint32_t success_value = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(uint32_t));

        bool success = (success_value == 1);

        if (verbose_)
        {
            LogMessage("응답 패킷 파싱: 타입=" + std::to_string(packet_type) +
                        ", 성공=" + std::string(success ? "true" : "false"));
        }

        return success;
    }

    void AsioTestClient::RecordTestResult(const std::string& test_name, bool success, const std::string& message)
    {
        if (success)
        {
            tests_passed_++;
            test_results_.push_back("✓ " + test_name + ": " + message);
        }
        else
        {
            tests_failed_++;
            test_results_.push_back("✗ " + test_name + ": " + message);
        }
    }

    void AsioTestClient::LogMessage(const std::string& message, bool is_error)
    {
        boost::lock_guard<boost::mutex> lock(log_mutex_);

        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm     = *std::localtime(&time_t);

        std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] ";
        if (is_error)
        {
            std::cout << "[ERROR] ";
        }
        else
        {
            std::cout << "[CLIENT] ";
        }
        std::cout << message << std::endl;
    }

} // namespace bt
