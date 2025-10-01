#include "Patrol.h"
#include "../../BT/Context.h"
#include "../Monster/MonsterBTExecutor.h"
#include "../Monster/Monster.h"

#include <iostream>
#include <cmath>

namespace bt
{
namespace action
{

    NodeStatus Patrol::Execute(Context& context)
    {
        auto ai = context.GetAI();
        if (!ai)
        {
            return NodeStatus::FAILURE;
        }

        auto monster_executor = std::dynamic_pointer_cast<MonsterBTExecutor>(ai);
        if (!monster_executor)
        {
            return NodeStatus::FAILURE;
        }

        auto monster = monster_executor->GetMonster();
        if (!monster)
        {
            return NodeStatus::FAILURE;
        }

        // 순찰점이 있는지 확인
        if (!monster->HasPatrolPoints())
        {
            return NodeStatus::FAILURE;
        }

        // 다음 순찰점 가져오기
        auto target_point = monster->GetNextPatrolPoint();
        const auto& current_pos = monster->GetPosition();

        // 목표 지점까지의 거리 계산
        float dx = target_point.x - current_pos.x;
        float dz = target_point.z - current_pos.z;
        float distance = std::sqrt(dx * dx + dz * dz);

        // 도착 거리 임계값 (5.0f) - 더 넉넉한 도착 범위
        const float arrival_threshold = 5.0f;

        if (distance <= arrival_threshold)
        {
            // std::cout << "Goblin " << monster->GetName() << " reached patrol point: (" 
            //           << target_point.x << ", " << target_point.y << ", " << target_point.z << ")" << std::endl;
            // 도착 지점에 도달했을 때도 정확한 위치로 이동
            monster->MoveTo(target_point.x, target_point.y, target_point.z, current_pos.rotation);
            // 다음 순찰점으로 인덱스 이동
            monster->AdvanceToNextPatrolPoint();
            return NodeStatus::SUCCESS;
        }
        else
        {
            // 목표 지점으로 이동
            float move_speed = monster->GetStats().move_speed;
            
            // 정규화된 방향 벡터 계산
            float normalized_dx = dx / distance;
            float normalized_dz = dz / distance;
            
            // 새로운 위치 계산 (더 큰 스텝으로 이동)
            float step_size = move_speed * 0.5f; // 더 큰 스텝으로 빠른 이동
            float new_x = current_pos.x + normalized_dx * step_size;
            float new_z = current_pos.z + normalized_dz * step_size;
            
            monster->MoveTo(new_x, current_pos.y, new_z, current_pos.rotation);
            
            // std::cout << "Goblin " << monster->GetName() << " moving to patrol point: (" 
            //           << new_x << ", " << current_pos.y << ", " << new_z << ")" << std::endl;
            
            return NodeStatus::RUNNING;
        }
    }

} // namespace action
} // namespace bt
