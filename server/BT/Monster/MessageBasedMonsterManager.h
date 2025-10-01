#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../../../shared/Game/PacketProtocol.h"
#include "../../Common/GameMessageProcessor.h"
#include "../../Common/GameMessages.h"
#include "Monster.h"
#include "MonsterTypes.h"

// 전방 선언
namespace bt
{
    class BeastHttpWebSocketServer;
    class PlayerManager;
    class MessageBasedPlayerManager;
    class Engine;
} // namespace bt

namespace bt
{

    // 메시지 기반 몬스터 매니저
    class MessageBasedMonsterManager : public IGameMessageHandler
    {
    public:
        MessageBasedMonsterManager();
        ~MessageBasedMonsterManager();

        // IGameMessageHandler 구현
        void HandleMessage(std::shared_ptr<GameMessage> message) override;

        // 몬스터 관리 (메시지 기반)
        void                                  AddMonster(std::shared_ptr<Monster> monster);
        void                                  RemoveMonster(uint32_t monster_id);
        std::shared_ptr<Monster>              GetMonster(uint32_t monster_id);
        std::vector<std::shared_ptr<Monster>> GetAllMonsters();
        std::vector<std::shared_ptr<Monster>> GetMonstersInRange(const MonsterPosition& position, float range);

        // 몬스터 생성
        std::shared_ptr<Monster> SpawnMonster(MonsterType            type,
                                              const std::string&     name,
                                              const MonsterPosition& position);

        // 자동 스폰 관리
        void AddSpawnConfig(const MonsterSpawnConfig& config);
        void RemoveSpawnConfig(MonsterType type, const std::string& name);
        void StartAutoSpawn();
        void StopAutoSpawn();

        // 설정 파일 관리
        bool LoadSpawnConfigsFromFile(const std::string& file_path);
        void ClearAllSpawnConfigs();

        // 업데이트 (메시지 큐에서 처리)
        void Update(float delta_time);

        // Behavior Tree 엔진 설정
        void SetBTEngine(std::shared_ptr<Engine> engine);

        // 메시지 프로세서 설정
        void SetMessageProcessor(std::shared_ptr<GameMessageProcessor> processor);

        // PlayerManager 설정
        void SetPlayerManager(std::shared_ptr<MessageBasedPlayerManager> manager);

        // 통계
        size_t GetMonsterCount() const;
        size_t GetMonsterCountByType(MonsterType type) const;
        size_t GetMonsterCountByName(const std::string& name) const;

        // 월드 상태 수집
        std::vector<MonsterState> GetMonsterStates() const;

        // 시작/중지
        void Start();
        void Stop();

    private:
        // 몬스터 데이터 (락 없이 단일 스레드에서만 접근)
        std::unordered_map<uint32_t, std::shared_ptr<Monster>>                 monsters_;
        std::vector<MonsterSpawnConfig>                                        spawn_configs_;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_spawn_times_;
        std::atomic<uint32_t>                                                  next_monster_id_;
        std::atomic<bool>                                                      auto_spawn_enabled_;

        // 의존성
        std::shared_ptr<Engine>                    bt_engine_;
        std::shared_ptr<GameMessageProcessor>      message_processor_;
        std::shared_ptr<MessageBasedPlayerManager> player_manager_;

        // 업데이트 스레드
        std::thread       update_thread_;
        std::atomic<bool> running_{false};
        std::atomic<bool> update_enabled_{true};

        // 플레이어 리스폰 포인트
        std::vector<MonsterPosition> player_respawn_points_;

        // 내부 메서드들
        void ProcessAutoSpawn(float delta_time);
        void ProcessRespawn(float delta_time);
        void ProcessMonsterUpdates(float delta_time);
        void UpdateThread();

        MonsterPosition GetRandomRespawnPoint() const;

        // JSON 파싱 헬퍼 함수들
        void ParsePatrolPoints(const std::string& points_content, std::vector<MonsterPosition>& points);

        // 메시지 처리 메서드들
        void HandleMonsterSpawn(std::shared_ptr<MonsterSpawnMessage> message);
        void HandleMonsterMove(std::shared_ptr<MonsterMoveMessage> message);
        void HandleMonsterDeath(std::shared_ptr<MonsterDeathMessage> message);
        void HandleGameStateUpdate(std::shared_ptr<GameStateUpdateMessage> message);
        void HandleSystemShutdown(std::shared_ptr<SystemShutdownMessage> message);

        // 메시지 전송 헬퍼
        void SendMonsterSpawnMessage(uint32_t               monster_id,
                                     const std::string&     name,
                                     const std::string&     type,
                                     const MonsterPosition& position,
                                     float                  health,
                                     float                  max_health);
        void SendMonsterMoveMessage(uint32_t monster_id, const MonsterPosition& from, const MonsterPosition& to);
        void SendMonsterDeathMessage(uint32_t monster_id, const std::string& name, const MonsterPosition& position);
    };

} // namespace bt
