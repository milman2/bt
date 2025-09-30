#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketProtocol.h"

namespace bt
{

    // 테스트 클라이언트 전용 설정 (공통 ClientConfig 확장)
    struct AsioClientConfig
    {
        std::string                 server_host = "127.0.0.1";
        uint16_t                    server_port = 7000;
        boost::chrono::milliseconds connection_timeout{5000};
        bool                        auto_reconnect         = false;
        int                         max_reconnect_attempts = 3;
    };

    // 테스트 클라이언트 클래스
    class AsioTestClient
    {
    public:
        AsioTestClient(const AsioClientConfig& config);
        ~AsioTestClient();

        // 연결 관리
        bool Connect();
        void Disconnect();
        bool IsConnected() const { return connected_.load(); }

        // 패킷 송수신
        bool SendPacket(const Packet& packet);
        bool ReceivePacket(Packet& packet);

        // 테스트 시나리오
        bool TestConnection();
        bool TestPlayerJoin(const std::string& player_name);
        bool TestPlayerMove(float x, float y, float z);
        bool TestPlayerAttack(uint32_t target_id);
        bool TestBTExecute(const std::string& bt_name);
        bool TestMonsterUpdate();
        bool TestDisconnect();

        // 자동 테스트
        bool RunAutomatedTest();
        bool RunStressTest(int num_connections, int duration_seconds);

        // 유틸리티
        void SetVerbose(bool verbose) { verbose_ = verbose; }
        void LogMessage(const std::string& message, bool is_error = false);

    private:
        // 네트워킹
        bool CreateConnection();
        void CloseConnection();

        // 패킷 처리
        Packet CreateConnectRequest();
        Packet CreatePlayerJoinPacket(const std::string& name);
        Packet CreatePlayerMovePacket(float x, float y, float z);
        Packet CreatePlayerAttackPacket(uint32_t target_id);
        Packet CreateBTExecutePacket(const std::string& bt_name);
        Packet CreateMonsterUpdatePacket();
        Packet CreateDisconnectPacket();

        bool ParsePacketResponse(const Packet& packet);

        // 테스트 결과
        void RecordTestResult(const std::string& test_name, bool success, const std::string& message = "");

    private:
        AsioClientConfig                                config_;
        std::atomic<bool>                               connected_;
        boost::asio::io_context                         io_context_;
        boost::shared_ptr<boost::asio::ip::tcp::socket> socket_;
        bool                                            verbose_;

        // 테스트 통계
        int                      tests_passed_;
        int                      tests_failed_;
        std::vector<std::string> test_results_;

        // 로깅
        boost::mutex log_mutex_;
    };

} // namespace bt
