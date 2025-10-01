#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketProtocol.h"
#include "../../BT/Engine.h"
#include "AsioClient.h"
#include "../../Common/GameMessageProcessor.h"
#include "../../Common/GameMessages.h"

namespace bt
{

    // 전방 선언
    class Engine;
    class MonsterBTExecutor;
    class MessageBasedMonsterManager;
    class MessageBasedPlayerManager;
    class BeastHttpWebSocketServer;
    class NetworkMessageHandler;

    // Asio 서버 전용 설정 구조체 (공통 ServerConfig 확장)
    struct AsioServerConfig
    {
        std::string                 host            = "0.0.0.0";
        uint16_t                    port            = 7000;
        size_t                      max_clients     = 1000;
        size_t                      worker_threads  = 4;
        bool                        debug_mode      = false;
        size_t                      max_packet_size = 4096;
        boost::chrono::milliseconds connection_timeout{30000}; // 30초
    };

    // 클라이언트 정보 구조체
    struct AsioClientInfo
    {
        boost::shared_ptr<AsioClient>           client;
        std::string                             ip_address;
        uint16_t                                port;
        boost::chrono::steady_clock::time_point connect_time;
        bool                                    is_authenticated;
        uint32_t                                player_id;
        std::string                             client_type; // "player", "monster", "tester"
    };

    // 서버 헬스 정보 구조체
    struct ServerHealthInfo
    {
        bool     is_healthy;
        size_t   connected_clients;
        size_t   total_packets_sent;
        size_t   total_packets_received;
        size_t   worker_threads;
        size_t   max_clients;
        uint64_t uptime_seconds;
    };

    // Asio 기반 서버 클래스
    class AsioServer
    {
    public:
        AsioServer(const AsioServerConfig& config);
        ~AsioServer();

        // 서버 시작/중지
        bool Start();
        void Stop();
        bool IsRunning() const { return running_.load(); }

        // 클라이언트 관리
        void AddClient(boost::shared_ptr<AsioClient> client);
        void RemoveClient(boost::shared_ptr<AsioClient> client);
        void BroadcastPacket(const Packet& packet, boost::shared_ptr<AsioClient> exclude_client = nullptr);
        void SendPacket(boost::shared_ptr<AsioClient> client, const Packet& packet);
        
        // 월드 상태 브로드캐스팅
        void BroadcastWorldState();
        void StartBroadcastLoop();
        void StopBroadcastLoop();
        void BroadcastLoopThread();

        // Behavior Tree 엔진 접근
        Engine* GetBTEngine() { return bt_engine_.get(); }

        // 매니저 접근
        std::shared_ptr<MessageBasedMonsterManager> GetMonsterManager() const { return message_based_monster_manager_; }
        std::shared_ptr<MessageBasedPlayerManager>  GetPlayerManager() const { return message_based_player_manager_; }

        // 통합 HTTP+WebSocket 서버 접근
        std::shared_ptr<BeastHttpWebSocketServer> GetHttpWebSocketServer() const { return http_websocket_server_; }

        // 설정 접근
        const AsioServerConfig& GetConfig() const { return config_; }

        // 통계 정보
        size_t GetConnectedClients() const;
        size_t GetTotalPacketsSent() const { return total_packets_sent_.load(); }
        size_t GetTotalPacketsReceived() const { return total_packets_received_.load(); }

        // 헬스체크 및 모니터링
        bool             IsHealthy() const;
        ServerHealthInfo GetHealthInfo() const;

    private:
        // 서버 초기화
        void StartAccept();
        void HandleAccept(boost::shared_ptr<AsioClient> client, const boost::system::error_code& error);

        // 워커 스레드
        void WorkerThreadFunction();

        // 패킷 처리

    public:
        void ProcessPacket(boost::shared_ptr<AsioClient> client, const Packet& packet);

    private:
        // 패킷 처리 함수들
        void HandlePlayerJoin(boost::shared_ptr<AsioClient> client, const Packet& packet);
        void HandlePlayerMove(boost::shared_ptr<AsioClient> client, const Packet& packet);
        
        // 응답 전송 함수들
        void SendConnectResponse(boost::shared_ptr<AsioClient> client);
        void SendMonsterSpawnResponse(boost::shared_ptr<AsioClient> client, bool success);
        void SendMonsterUpdateResponse(boost::shared_ptr<AsioClient> client, bool success);
        void SendBTExecuteResponse(boost::shared_ptr<AsioClient> client, bool success);
        void SendPlayerJoinResponse(boost::shared_ptr<AsioClient> client, bool success, uint32_t player_id);
        void SendErrorResponse(boost::shared_ptr<AsioClient> client, const std::string& error_message);

        // 로깅
        void LogMessage(const std::string& message, bool is_error = false);

        // HTTP 핸들러 등록
        void RegisterHttpHandlers();

    private:
        AsioServerConfig  config_;
        std::atomic<bool> running_;

        // Boost.Asio 관련
        boost::asio::io_context        io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::thread_group            worker_threads_;

        // 클라이언트 관리
        std::unordered_map<boost::shared_ptr<AsioClient>, AsioClientInfo> clients_;
        mutable boost::mutex                                              clients_mutex_;

        // Behavior Tree 엔진
        std::shared_ptr<Engine> bt_engine_;

        // 몬스터 및 플레이어 매니저 (메시지 큐 기반)
        std::shared_ptr<MessageBasedMonsterManager> message_based_monster_manager_;
        std::shared_ptr<MessageBasedPlayerManager> message_based_player_manager_;

        // 통합 HTTP+WebSocket 서버
        std::shared_ptr<BeastHttpWebSocketServer> http_websocket_server_;
        
        // 메시지 큐 시스템
        std::shared_ptr<GameMessageProcessor> message_processor_;
        std::shared_ptr<NetworkMessageHandler> network_handler_;
        
        // 브로드캐스팅 스레드
        boost::thread                        broadcast_thread_;
        std::atomic<bool>                    broadcast_running_;
        std::chrono::steady_clock::time_point last_broadcast_time_;

        // 통계
        std::atomic<size_t> total_packets_sent_;
        std::atomic<size_t> total_packets_received_;

        // 서버 시작 시간 (헬스체크용)
        std::chrono::steady_clock::time_point server_start_time_;

        // 로깅
        mutable boost::mutex log_mutex_;
    };


} // namespace bt
