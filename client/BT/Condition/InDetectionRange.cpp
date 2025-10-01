#include "InDetectionRange.h"
#include "../../../BT/Context.h"
#include "../../TestClient.h"

#include <iostream>

namespace bt
{
namespace condition
{

    NodeStatus InDetectionRange::Execute(Context& context)
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

        // 탐지 범위 내에 있는지 확인
        float distance = client_executor->GetDistanceToTarget();
        float detection_range = client_executor->GetDetectionRange();

        if (distance <= detection_range)
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 탐지 범위 내: 거리 " 
                      << distance << " <= " << detection_range << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace condition
} // namespace bt
