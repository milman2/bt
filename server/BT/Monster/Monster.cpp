#include "Monster.h"
#include "MonsterBTExecutor.h"
#include "MonsterFactory.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <cmath>
#include <nlohmann/json.hpp>

namespace bt
{

    // Monster 구현
    Monster::Monster(const std::string& name, MonsterType type, const MonsterPosition& position)
        : id_(0)
        , name_(name)
        , type_(type)
        , state_(MonsterState::IDLE)
        , position_(position)
        , ai_name_("")
        , bt_name_("")
        , target_id_(0)
        , current_patrol_index_(0)
        , spawn_position_(position)
    {
        // 기본 통계 설정
        stats_            = MonsterFactory::GetDefaultStats(type);
        last_update_time_ = std::chrono::steady_clock::now();

        // 기본 순찰점 설정 (스폰 위치 주변)
        SetDefaultPatrolPoints();

        std::cout << "몬스터 생성: " << name_ << " (타입: " << static_cast<int>(type_) << ")" << std::endl;
    }

    Monster::~Monster()
    {
        std::cout << "몬스터 소멸: " << name_ << std::endl;
    }

    void Monster::SetPosition(float x, float y, float z, float rotation)
    {
        position_.x        = x;
        position_.y        = y;
        position_.z        = z;
        position_.rotation = rotation;
    }

    void Monster::MoveTo(float x, float y, float z, float rotation)
    {
        SetPosition(x, y, z, rotation);
        std::cout << "몬스터 " << name_ << " 이동: (" << x << ", " << y << ", " << z << ")" << std::endl;
    }

    void Monster::TakeDamage(uint32_t damage)
    {
        if (damage >= stats_.health)
        {
            stats_.health = 0;
            state_        = MonsterState::DEAD;
            death_time_   = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count() /
                          1000.0f;
            std::cout << "몬스터 " << name_ << " 사망!" << std::endl;
        }
        else
        {
            stats_.health -= damage;
            std::cout << "몬스터 " << name_ << " 데미지 받음: " << damage << " (남은 체력: " << stats_.health << ")"
                      << std::endl;
        }
    }

    void Monster::Heal(uint32_t amount)
    {
        if (state_ == MonsterState::DEAD)
        {
            return;
        }

        uint32_t old_health = stats_.health;
        stats_.health += amount;

        if (stats_.health > stats_.max_health)
        {
            stats_.health = stats_.max_health;
        }

        uint32_t actual_heal = stats_.health - old_health;
        if (actual_heal > 0)
        {
            std::cout << "몬스터 " << name_ << " 치료됨: " << actual_heal << " (현재 체력: " << stats_.health << ")"
                      << std::endl;
        }
    }

    void Monster::UpdateEnvironmentInfo(const std::vector<std::shared_ptr<Player>>&  players,
                                          const std::vector<std::shared_ptr<Monster>>& monsters)
    {
        environment_info_ = EnvironmentInfo();

        // 주변 플레이어 검색 (서버에서 주입받은 정보 사용)
        // TODO: 서버에서 플레이어 정보를 주입받아 처리
        // 현재는 빈 구현으로 두고 서버에서 실제 로직 처리

        // 주변 몬스터 검색
        for (const auto& monster : monsters)
        {
            if (!monster || monster.get() == this || !monster->IsAlive())
                continue;

            const auto& monster_pos = monster->GetPosition();
            float       distance =
                std::sqrt(std::pow(monster_pos.x - position_.x, 2) + std::pow(monster_pos.y - position_.y, 2) +
                          std::pow(monster_pos.z - position_.z, 2));

            if (distance <= stats_.detection_range)
            {
                environment_info_.nearby_monsters.push_back(monster->GetTargetID());
            }
        }

        // 시야 확보 여부 (간단한 구현)
        environment_info_.has_line_of_sight = true; // TODO: 실제 시야 계산 구현
    }

    // 환경 인지 헬퍼 메서드들 구현
    bool Monster::HasEnemyInRange(float range) const
    {
        return environment_info_.nearest_enemy_distance >= 0 && environment_info_.nearest_enemy_distance <= range;
    }

    bool Monster::HasEnemyInAttackRange() const
    {
        return HasEnemyInRange(stats_.attack_range);
    }

    bool Monster::HasEnemyInDetectionRange() const
    {
        return HasEnemyInRange(stats_.detection_range);
    }

    bool Monster::HasEnemyInChaseRange() const
    {
        return HasEnemyInRange(chase_range_);
    }

    float Monster::GetDistanceToNearestEnemy() const
    {
        return environment_info_.nearest_enemy_distance;
    }

    uint32_t Monster::GetNearestEnemyID() const
    {
        return environment_info_.nearest_enemy_id;
    }

    bool Monster::CanSeeTarget(uint32_t target_id) const
    {
        // 간단한 시야 구현: 거리와 시야 확보 여부 확인
        if (target_id == 0)
            return false;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.has_line_of_sight &&
                   environment_info_.nearest_enemy_distance <= stats_.detection_range;
        }

        return false;
    }

    bool Monster::IsTargetInRange(uint32_t target_id, float range) const
    {
        if (target_id == 0)
            return false;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.nearest_enemy_distance <= range;
        }

        return false;
    }

    float Monster::GetDistanceToTarget(uint32_t target_id) const
    {
        if (target_id == 0)
            return -1.0f;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.nearest_enemy_distance;
        }

        return -1.0f;
    }

    void Monster::AttackTarget()
    {
        if (!HasTarget())
            return;

        // TODO: 실제 타겟에게 데미지 주기 (PlayerManager를 통해)
        std::cout << "Monster " << name_ << " attacks target " << target_id_ << " for " << damage_ << " damage!"
                  << std::endl;

        // 공격 상태로 변경
        SetState(MonsterState::ATTACK);
    }

    void Monster::Update(float delta_time)
    {
        static std::map<std::string, int> update_counts;
        update_counts[name_]++;

        if (update_counts[name_] % 50 == 0)
        { // 5초마다 로그 출력
            std::cout << "Monster::update 호출됨: " << name_ << " (카운트: " << update_counts[name_] << ")"
                      << std::endl;
        }

        // AI 업데이트
        if (ai_)
        {
            ai_->Update(delta_time);
            if (update_counts[name_] % 50 == 0)
            {
                std::cout << "Monster::update - AI 업데이트 완료: " << name_ << std::endl;
            }
        }
        else
        {
            if (update_counts[name_] % 50 == 0)
            {
                std::cout << "Monster::update - AI가 없음: " << name_ << std::endl;
            }
        }

        last_update_time_ = std::chrono::steady_clock::now();
    }

    void Monster::SetDefaultPatrolPoints()
    {
        patrol_points_.clear();

        // 스폰 위치를 중심으로 4방향 순찰점 생성
        float patrol_radius = 15.0f; // 순찰 반경

        patrol_points_.push_back(spawn_position_); // 스폰 위치
        patrol_points_.push_back({spawn_position_.x + patrol_radius, spawn_position_.y, spawn_position_.z, 0.0f});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z + patrol_radius, 0.0f});
        patrol_points_.push_back({spawn_position_.x - patrol_radius, spawn_position_.y, spawn_position_.z, 0.0f});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z - patrol_radius, 0.0f});

        current_patrol_index_ = 0;
    }

    void Monster::SetPatrolPoints(const std::vector<MonsterPosition>& points)
    {
        patrol_points_        = points;
        current_patrol_index_ = 0;
    }

    void Monster::AddPatrolPoint(const MonsterPosition& point)
    {
        patrol_points_.push_back(point);
    }

    MonsterPosition Monster::GetNextPatrolPoint()
    {
        if (patrol_points_.empty())
        {
            return spawn_position_;
        }

        MonsterPosition point = patrol_points_[current_patrol_index_];
        current_patrol_index_ = (current_patrol_index_ + 1) % patrol_points_.size();
        return point;
    }

} // namespace bt
