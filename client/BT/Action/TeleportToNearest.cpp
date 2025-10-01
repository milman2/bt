#include <iostream>
#include <limits>

#include "../../../BT/Context.h"
#include "../../TestClient.h"
#include "TeleportToNearest.h"

namespace bt
{
    namespace action
    {

        NodeStatus TeleportToNearest::Execute(Context& context)
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

            // 텔레포트 실행
            bool teleport_success = client_executor->ExecuteTeleportToNearest();

            if (teleport_success)
            {
                std::cout << "TeleportToNearest 액션: 텔레포트 성공 - 플레이어 " << client_executor->GetName()
                          << std::endl;
                return NodeStatus::SUCCESS;
            }
            else
            {
                std::cout << "TeleportToNearest 액션: 텔레포트 실패 - 플레이어 " << client_executor->GetName()
                          << std::endl;
                return NodeStatus::FAILURE;
            }
        }

    } // namespace action
} // namespace bt
