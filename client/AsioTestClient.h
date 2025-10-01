#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cmath>
#include <random>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketProtocol.h"
#include "../BT/Node.h"
#include "../BT/Tree.h"
#include "../BT/Context.h"
#include "../BT/Engine.h"
#include "../BT/IExecutor.h"
#include "../BT/EnvironmentInfo.h"
#include "BT/PlayerBTs.h"
#include "Common/ClientMessageProcessor.h"
#include "Network/ClientNetworkMessageHandler.h"
#include "AI/ClientAIMessageHandler.h"

namespace bt
{

    // 플레이어 AI 클라이언트 설정
    struct PlayerAIConfig
    {
        std::string                 server_host = "127.0.0.1";
        uint16_t                    server_port = 7000;
        std::string                 player_name = "AI_Player";
        float                       spawn_x = 0.0f;
        float                       spawn_z = 0.0f;
        float                       patrol_radius = 50.0f;
        float                       detection_range = 30.0f;
        float                       attack_range = 5.0f;
        float                       move_speed = 3.0f;
        int                         health = 100;
        int                         damage = 20;
    };

    // 플레이어 위치 정보
    struct PlayerPosition
    {
        float x, y, z, rotation;
        PlayerPosition(float x = 0, float y = 0, float z = 0, float r = 0) 
            : x(x), y(y), z(z), rotation(r) {}
    };

    // 플레이어 AI 클라이언트 클래스
    class AsioTestClient : public IExecutor, public std::enable_shared_from_this<AsioTestClient>
    {
    public:
        AsioTestClient(const PlayerAIConfig& config);
        ~AsioTestClient();

        // 연결 관리
        bool Connect();
        void Disconnect();
        bool IsConnected() const { return connected_.load(); }

        // AI 동작
        void StartAI();
        void StopAI();
        void UpdateAI(float delta_time);

        // IExecutor 인터페이스 구현
        void Update(float delta_time) override;
        void SetBehaviorTree(std::shared_ptr<Tree> tree) override;
        std::shared_ptr<Tree> GetBehaviorTree() const override;
        Context& GetContext() override;
        const Context& GetContext() const override;
        const std::string& GetName() const override;
        const std::string& GetBTName() const override;
        bool IsActive() const override;
        void SetActive(bool active) override;

        // 패킷 송수신
        bool SendPacket(const Packet& packet);
        bool ReceivePacket(Packet& packet);

        // 플레이어 액션
        bool JoinGame();
        bool MoveTo(float x, float y, float z);
        bool AttackTarget(uint32_t target_id);
        bool Respawn();

        // AI 상태
        bool IsAlive() const { return health_ > 0; }
        bool HasTarget() const { return target_id_ != 0; }
        float GetDistanceToTarget() const;
        uint32_t GetNearestMonster() const;
        
        // Context 설정 (shared_from_this() 사용을 위해)
        void SetContextAI();
        
        // BT 노드에서 사용할 헬퍼 메서드들
        bool HasPatrolPoints() const { return !patrol_points_.empty(); }
        PlayerPosition GetNextPatrolPoint() const;
        void AdvanceToNextPatrolPoint();
        PlayerPosition GetPosition() const { return position_; }
        uint32_t GetTargetID() const { return target_id_; }
        float GetMoveSpeed() const { return config_.move_speed; }
        float GetAttackRange() const { return config_.attack_range; }
        float GetDetectionRange() const { return config_.detection_range; }

        // 유틸리티
        void SetVerbose(bool verbose) { verbose_ = verbose; }
        bool IsVerbose() const { return verbose_; }
        void LogMessage(const std::string& message, bool is_error = false);

        // 메시지 큐 관련 메서드
        void InitializeMessageQueue();
        void ShutdownMessageQueue();
        void SendNetworkPacket(const std::vector<uint8_t>& data, uint16_t packet_type);
        void UpdateMonsters(const std::unordered_map<uint32_t, std::tuple<float, float, float, float>>& monsters);
        void HandleCombatResult(uint32_t attacker_id, uint32_t target_id, uint32_t damage, uint32_t remaining_health);
        void SetPlayerID(uint32_t player_id) { player_id_ = player_id; }
        void SetConnected(bool connected) { connected_.store(connected); }
        
        // 환경 인지 (메시지 핸들러에서 접근 가능하도록 public)
        void UpdateEnvironmentInfo();

    private:
        // 네트워킹
        bool CreateConnection();
        void CloseConnection();

        // 패킷 처리
        Packet CreateConnectRequest();
        Packet CreatePlayerJoinPacket(const std::string& name);
        Packet CreatePlayerMovePacket(float x, float y, float z);
        Packet CreatePlayerAttackPacket(uint32_t target_id);
        Packet CreateDisconnectPacket();

        bool ParsePacketResponse(const Packet& packet);

        // AI 로직
        void CreatePatrolPoints();
        void UpdatePatrol(float delta_time);
        void UpdateCombat(float delta_time);
        void FindNearestMonster();
        bool IsInRange(float x, float z, float range) const;
        
        // 환경 인지
        const EnvironmentInfo& GetEnvironmentInfo() const { return environment_info_; }

        // 패킷 처리
        void HandleMonsterUpdate(const Packet& packet);
        void HandlePlayerUpdate(const Packet& packet);
        void HandleCombatResult(const Packet& packet);

    private:
        PlayerAIConfig                                  config_;
        std::atomic<bool>                               connected_;
        boost::asio::io_context                         io_context_;
        boost::shared_ptr<boost::asio::ip::tcp::socket> socket_;
        bool                                            verbose_;

        // AI 상태
        std::atomic<bool>                               ai_running_;
        std::shared_ptr<Tree>                           behavior_tree_;
        Context                                         context_;
        std::string                                     bt_name_;
        PlayerPosition                                  position_;
        PlayerPosition                                  spawn_position_;
        std::vector<PlayerPosition>                     patrol_points_;
        size_t                                         current_patrol_index_;
        
        // 전투 상태
        uint32_t                                        player_id_;
        uint32_t                                        target_id_;
        int                                             health_;
        int                                             max_health_;
        float                                           last_attack_time_;
        float                                           attack_cooldown_;
        
        // 몬스터 정보
        std::map<uint32_t, PlayerPosition>             monsters_;
        float                                           last_monster_update_;
        
        // 환경 인지 정보
        EnvironmentInfo                                 environment_info_;

        // 메시지 큐 시스템
        std::shared_ptr<ClientMessageProcessor>         message_processor_;
        std::shared_ptr<ClientNetworkMessageHandler>    network_handler_;
        std::shared_ptr<ClientAIMessageHandler>         ai_handler_;

        // 로깅
        boost::mutex log_mutex_;
    };

} // namespace bt
