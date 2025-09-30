#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// 전방 선언
namespace bt
{
    class SimpleWebSocketServer;
    class PlayerManager;
    class Player;
} // namespace bt

namespace bt
{

    // 몬스터 타입 정의
    enum class MonsterType
    {
        GOBLIN,
        ORC,
        DRAGON,
        SKELETON,
        ZOMBIE,
        NPC_MERCHANT,
        NPC_GUARD
    };

    // 몬스터 상태
    enum class MonsterState
    {
        IDLE,
        PATROL,
        CHASE,
        ATTACK,
        FLEE,
        DEAD
    };

    // 몬스터 통계
    struct MonsterStats
    {
        uint32_t level           = 1;
        uint32_t health          = 100;
        uint32_t max_health      = 100;
        uint32_t mana            = 50;
        uint32_t max_mana        = 50;
        uint32_t attack_power    = 10;
        uint32_t defense         = 5;
        float    move_speed      = 1.0f;
        float    attack_range    = 2.0f;
        float    detection_range = 10.0f;
    };

    // 몬스터 위치
    struct MonsterPosition
    {
        float x        = 0.0f;
        float y        = 0.0f;
        float z        = 0.0f;
        float rotation = 0.0f;
    };

    // 몬스터 스폰 설정
    struct MonsterSpawnConfig
    {
        MonsterType                  type;
        std::string                  name;
        MonsterPosition              position;
        float                        respawn_time = 30.0f;    // 리스폰 시간 (초)
        uint32_t                     max_count    = 1;        // 최대 생성 수
        float                        spawn_radius = 5.0f;     // 스폰 반경
        bool                         auto_spawn   = true;     // 자동 스폰 여부
        std::vector<MonsterPosition> patrol_points;           // 순찰 경로
        float                        detection_range = 15.0f; // 탐지 범위
        float                        attack_range    = 3.0f;  // 공격 범위
        float                        chase_range     = 25.0f; // 추적 범위
        uint32_t                     health          = 100;   // 체력
        uint32_t                     damage          = 10;    // 공격력
        float                        move_speed      = 2.0f;  // 이동 속도
    };

    // 환경 인지 정보
    struct EnvironmentInfo
    {
        std::vector<uint32_t> nearby_players;                 // 주변 플레이어 ID들
        std::vector<uint32_t> nearby_monsters;                // 주변 몬스터 ID들
        std::vector<uint32_t> obstacles;                      // 장애물 ID들
        bool                  has_line_of_sight      = true;  // 시야 확보 여부
        float                 nearest_enemy_distance = -1.0f; // 가장 가까운 적과의 거리
        uint32_t              nearest_enemy_id       = 0;     // 가장 가까운 적 ID
    };

    // 전방 선언
    class MonsterBTExecutor;

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
        void  set_respawn_time(float time) { respawn_time_ = time; }
        float get_respawn_time() const { return respawn_time_; }
        void  set_death_time(float time) { death_time_ = time; }
        float get_death_time() const { return death_time_; }
        bool  should_respawn(float current_time) const;

        // 환경 인지
        EnvironmentInfo get_environment_info() const { return environment_info_; }
        void            update_environment_info(const std::vector<std::shared_ptr<Player>>&  players,
                                                const std::vector<std::shared_ptr<Monster>>& monsters);

        // 환경 인지 헬퍼 메서드들
        bool     has_enemy_in_range(float range) const;
        bool     has_enemy_in_attack_range() const;
        bool     has_enemy_in_detection_range() const;
        bool     has_enemy_in_chase_range() const;
        float    get_distance_to_nearest_enemy() const;
        uint32_t get_nearest_enemy_id() const;
        bool     can_see_target(uint32_t target_id) const;
        bool     is_target_in_range(uint32_t target_id, float range) const;
        float    get_distance_to_target(uint32_t target_id) const;
        float    get_chase_range() const { return chase_range_; }

        // 순찰 관련
        void            set_patrol_points(const std::vector<MonsterPosition>& points);
        void            add_patrol_point(const MonsterPosition& point);
        MonsterPosition get_next_patrol_point();
        bool            has_patrol_points() const { return !patrol_points_.empty(); }
        void            reset_patrol_index() { current_patrol_index_ = 0; }

        // 업데이트
        void update(float delta_time);

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

        void set_default_patrol_points();
    };

} // namespace bt
