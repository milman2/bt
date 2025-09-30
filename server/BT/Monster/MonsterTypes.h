#pragma once

#include <string>
#include <vector>

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

} // namespace bt
