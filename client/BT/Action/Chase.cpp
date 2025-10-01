#include "Chase.h"
#include "../../../BT/Context.h"
#include "../../TestClient.h"

#include <iostream>

namespace bt
{
namespace action
{

    NodeStatus Chase::Execute(Context& context)
    {
        // AI 클라이언트 가져오기
        auto ai = context.GetAI();
        if (!ai)
        {
            return NodeStatus::FAILURE;
        }

        auto client_executor = std::dynamic_pointer_cast<TestClient>(ai);
        if (!client_executor)
        {
            return NodeStatus::FAILURE;
        }

        // 타겟이 있는지 확인
        if (!client_executor->HasTarget())
        {
            return NodeStatus::FAILURE;
        }

        // 타겟 위치 가져오기
        auto target_pos = client_executor->GetMonsterPosition(client_executor->GetTargetID());
        if (!target_pos)
        {
            return NodeStatus::FAILURE;
        }

        // 타겟까지의 거리 계산
        float dx = target_pos->x - client_executor->GetPosition().x;
        float dz = target_pos->z - client_executor->GetPosition().z;
        float distance = std::sqrt(dx * dx + dz * dz);

        // 너무 가까우면 추적 완료
        if (distance <= 0.1f)
        {
            return NodeStatus::SUCCESS;
        }

        // 타겟 방향으로 이동
        float step_size = client_executor->GetMoveSpeed() * 0.1f; // delta_time 대신 고정값 사용
        float normalized_dx = dx / distance;
        float normalized_dz = dz / distance;
        
        float new_x = client_executor->GetPosition().x + normalized_dx * step_size;
        float new_z = client_executor->GetPosition().z + normalized_dz * step_size;
        
        if (client_executor->MoveTo(new_x, client_executor->GetPosition().y, new_z))
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 추적 중: 거리 " 
                      << distance << std::endl;
            return NodeStatus::RUNNING;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace action
} // namespace bt
