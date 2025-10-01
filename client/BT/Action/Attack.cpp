#include "Attack.h"
#include "../../../BT/Context.h"
#include "../../TestClient.h"

#include <iostream>

namespace bt
{
namespace action
{

    NodeStatus Attack::Execute(Context& context)
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

        // 공격 실행
        if (client_executor->AttackTarget(client_executor->GetTargetID()))
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 공격: 타겟 ID " 
                      << client_executor->GetTargetID() << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace action
} // namespace bt
