#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "MonsterTypes.h"

// 전방 선언
namespace bt
{
    class SimpleWebSocketServer;
    class PlayerManager;
    class Player;
    class MonsterBTExecutor;
} // namespace bt

namespace bt
{

    // 몬스터 클래스
    class Monster
    {
    public:
        Monster(const std::string& name, MonsterType type, const MonsterPosition& position);
        ~Monster();

        // 기본 정보
        uint32_t           GetID() const { return id_; }
        void               SetID(uint32_t id) { id_ = id; }
        const std::string& GetName() const { return name_; }
        MonsterType        GetType() const { return type_; }
        MonsterState       GetState() const { return state_; }
        void               SetState(MonsterState state) { state_ = state; }

        // 위치 관련
        const MonsterPosition& GetPosition() const { return position_; }
        void                   SetPosition(const MonsterPosition& pos) { position_ = pos; }
        void                   SetPosition(float x, float y, float z, float rotation = 0.0f);
        void                   MoveTo(float x, float y, float z, float rotation = 0.0f);

        // 통계 관련
        const MonsterStats& GetStats() const { return stats_; }
        void                SetStats(const MonsterStats& stats) { stats_ = stats; }
        void                TakeDamage(uint32_t damage);
        void                Heal(uint32_t amount);
        bool                IsAlive() const { return stats_.health > 0; }
        uint32_t            GetMaxHealth() const { return stats_.max_health; }

        // AI 관련
        std::shared_ptr<MonsterBTExecutor> GetAI() const { return ai_; }
        void                               SetAI(std::shared_ptr<MonsterBTExecutor> ai) { ai_ = ai; }
        void                               SetAIName(const std::string& ai_name) { ai_name_ = ai_name; }
        const std::string&                 GetAIName() const { return ai_name_; }
        void                               SetBTName(const std::string& bt_name) { bt_name_ = bt_name; }
        const std::string&                 GetBTName() const { return bt_name_; }

        // 타겟 관리
        uint32_t GetTargetID() const { return target_id_; }
        void     SetTargetID(uint32_t id) { target_id_ = id; }
        bool     HasTarget() const { return target_id_ != 0; }
        void     ClearTarget() { target_id_ = 0; }

        // 전투 관련
        void AttackTarget();
        bool IsInAttackRange(uint32_t target_id) const;
        bool IsInDetectionRange(uint32_t target_id) const;
        bool IsInChaseRange(uint32_t target_id) const;

        // 리스폰 관련
        void  SetRespawnTime(float time) { respawn_time_ = time; }
        float GetRespawnTime() const { return respawn_time_; }
        void  SetDeathTime(float time) { death_time_ = time; }
        float GetDeathTime() const { return death_time_; }
        bool  ShouldRespawn(float current_time) const;

        // 환경 인지
        EnvironmentInfo GetEnvironmentInfo() const { return environment_info_; }
        void            UpdateEnvironmentInfo(const std::vector<std::shared_ptr<Player>>&  players,
                                                const std::vector<std::shared_ptr<Monster>>& monsters);

        // 환경 인지 헬퍼 메서드들
        bool     HasEnemyInRange(float range) const;
        bool     HasEnemyInAttackRange() const;
        bool     HasEnemyInDetectionRange() const;
        bool     HasEnemyInChaseRange() const;
        float    GetDistanceToNearestEnemy() const;
        uint32_t GetNearestEnemyID() const;
        bool     CanSeeTarget(uint32_t target_id) const;
        bool     IsTargetInRange(uint32_t target_id, float range) const;
        float    GetDistanceToTarget(uint32_t target_id) const;
        float    GetChaseRange() const { return chase_range_; }

        // 순찰 관련
        void            SetPatrolPoints(const std::vector<MonsterPosition>& points);
        void            AddPatrolPoint(const MonsterPosition& point);
        MonsterPosition GetNextPatrolPoint();
        bool            HasPatrolPoints() const { return !patrol_points_.empty(); }
        void            ResetPatrolIndex() { current_patrol_index_ = 0; }

        // 업데이트
        void Update(float delta_time);

    private:
        uint32_t                              id_;
        std::string                           name_;
        MonsterType                           type_;
        MonsterState                          state_;
        MonsterPosition                       position_;
        MonsterStats                          stats_;
        std::shared_ptr<MonsterBTExecutor>    ai_;
        std::string                           ai_name_;
        std::string                           bt_name_;
        uint32_t                              target_id_;
        std::chrono::steady_clock::time_point last_update_time_;
        EnvironmentInfo                       environment_info_;

        // 순찰 관련
        std::vector<MonsterPosition> patrol_points_;
        size_t                       current_patrol_index_;
        MonsterPosition              spawn_position_; // 스폰 위치 (기본 순찰점)

        // 전투 관련
        float    detection_range_ = 15.0f;
        float    attack_range_    = 3.0f;
        float    chase_range_     = 25.0f;
        uint32_t damage_          = 10;
        float    move_speed_      = 2.0f;

        // 리스폰 관련
        float respawn_time_ = 30.0f;
        float death_time_   = 0.0f;

        void SetDefaultPatrolPoints();
    };

} // namespace bt
