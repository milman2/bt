#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "MonsterTypes.h"
#include "Monster.h"

// 전방 선언
namespace bt
{
    class SimpleWebSocketServer;
    class PlayerManager;
    class BehaviorTreeEngine;
} // namespace bt

namespace bt
{

    // 몬스터 매니저
    class MonsterManager
    {
    public:
        MonsterManager();
        ~MonsterManager();

        // 몬스터 관리
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

        // 업데이트
        void Update(float delta_time);

        // Behavior Tree 엔진 설정
        void SetBTEngine(std::shared_ptr<BehaviorTreeEngine> engine);

        // WebSocket 서버 설정
        void SetWebSocketServer(std::shared_ptr<bt::SimpleWebSocketServer> server);

        // PlayerManager 설정
        void SetPlayerManager(std::shared_ptr<PlayerManager> manager);

        // 통계
        size_t GetMonsterCount() const { return monsters_.size(); }
        size_t GetMonsterCountByType(MonsterType type) const;
        size_t GetMonsterCountByName(const std::string& name) const;

    private:
        std::unordered_map<uint32_t, std::shared_ptr<Monster>>                 monsters_;
        std::vector<MonsterSpawnConfig>                                        spawn_configs_;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_spawn_times_;
        std::atomic<uint32_t>                                                  next_monster_id_;
        std::atomic<bool>                                                      auto_spawn_enabled_;
        std::shared_ptr<BehaviorTreeEngine>                                    bt_engine_;
        std::shared_ptr<bt::SimpleWebSocketServer>                             websocket_server_;
        std::shared_ptr<PlayerManager>                                         player_manager_;
        mutable std::mutex                                                     monsters_mutex_;
        mutable std::mutex                                                     spawn_mutex_;

        void ProcessAutoSpawn(float delta_time);
        void ProcessRespawn(float delta_time);

        // 플레이어 리스폰 포인트
        std::vector<MonsterPosition> player_respawn_points_;

        // JSON 파싱 헬퍼 함수들
        void            ParsePatrolPoints(const std::string& points_content, std::vector<MonsterPosition>& points);
        MonsterPosition GetRandomRespawnPoint() const;
    };

} // namespace bt
