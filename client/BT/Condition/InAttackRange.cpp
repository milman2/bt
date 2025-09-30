#include "InAttackRange.h"
#include "../../../BT/Context.h"
#include "../../AsioTestClient.h"

#include <iostream>

namespace bt
{
namespace condition
{

    NodeStatus InAttackRange::Execute(Context& context)
    {
        // AI 클라이언트 가져오기
        auto ai = context.GetAI();
        if (!ai)
        {
            return NodeStatus::FAILURE;
        }

        auto client_executor = std::dynamic_pointer_cast<AsioTestClient>(ai);
        if (!client_executor)
        {
            return NodeStatus::FAILURE;
        }

        // 타겟이 있는지 확인
        if (!client_executor->HasTarget())
        {
            return NodeStatus::FAILURE;
        }

        // 공격 범위 내에 있는지 확인
        float distance = client_executor->GetDistanceToTarget();
        float attack_range = client_executor->GetAttackRange();

        if (distance <= attack_range)
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 공격 범위 내: 거리 " 
                      << distance << " <= " << attack_range << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace condition
} // namespace bt
