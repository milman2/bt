#pragma once

#include <vector>

#include <cstdint>

namespace bt
{

    // 환경 인지 정보 (서버와 클라이언트 공통 사용)
    struct EnvironmentInfo
    {
        std::vector<uint32_t> nearby_players;                 // 주변 플레이어 ID들
        std::vector<uint32_t> nearby_monsters;                // 주변 몬스터 ID들
        std::vector<uint32_t> obstacles;                      // 장애물 ID들
        bool                  has_line_of_sight      = true;  // 시야 확보 여부
        float                 nearest_enemy_distance = -1.0f; // 가장 가까운 적과의 거리
        uint32_t              nearest_enemy_id       = 0;     // 가장 가까운 적 ID

        // 생성자
        EnvironmentInfo() = default;

        // 유틸리티 함수들
        void Clear()
        {
            nearby_players.clear();
            nearby_monsters.clear();
            obstacles.clear();
            has_line_of_sight      = true;
            nearest_enemy_distance = -1.0f;
            nearest_enemy_id       = 0;
        }

        bool HasNearbyPlayers() const { return !nearby_players.empty(); }
        bool HasNearbyMonsters() const { return !nearby_monsters.empty(); }
        bool HasObstacles() const { return !obstacles.empty(); }
        bool HasEnemy() const { return nearest_enemy_id != 0; }

        // 거리 기반 유틸리티
        bool IsEnemyInRange(float range) const { return HasEnemy() && nearest_enemy_distance <= range; }

        // 가장 가까운 적이 플레이어인지 확인
        bool IsNearestEnemyPlayer() const
        {
            if (!HasEnemy())
                return false;

            for (uint32_t player_id : nearby_players)
            {
                if (player_id == nearest_enemy_id)
                    return true;
            }
            return false;
        }

        // 가장 가까운 적이 몬스터인지 확인
        bool IsNearestEnemyMonster() const
        {
            if (!HasEnemy())
                return false;

            for (uint32_t monster_id : nearby_monsters)
            {
                if (monster_id == nearest_enemy_id)
                    return true;
            }
            return false;
        }
    };

} // namespace bt
