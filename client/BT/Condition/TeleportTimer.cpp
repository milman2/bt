#include "TeleportTimer.h"
#include "../../../BT/Context.h"
#include "../../AsioTestClient.h"

#include <iostream>

namespace bt
{
namespace condition
{


    NodeStatus TeleportTimer::Execute(Context& context)
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

        // 타겟이 없거나 탐지 범위 밖에 있을 때만 타이머 체크
        if (!client_executor->HasTarget() || client_executor->GetDistanceToTarget() > client_executor->GetDetectionRange())
        {
            // 텔레포트 타이머 가져오기
            float teleport_timer = client_executor->GetTeleportTimer();
            
            if (teleport_timer >= TELEPORT_TIMEOUT)
            {
                std::cout << "TeleportTimer 조건: 텔레포트 타이머 만료 (" << teleport_timer << "초 >= " << TELEPORT_TIMEOUT << "초)" << std::endl;
                return NodeStatus::SUCCESS;  // 텔레포트 실행 필요
            }
            else
            {
                std::cout << "TeleportTimer 조건: 텔레포트 타이머 진행 중 (" << teleport_timer << "초 / " << TELEPORT_TIMEOUT << "초)" << std::endl;
                return NodeStatus::FAILURE;  // 아직 대기 중
            }
        }
        else
        {
            // 타겟이 탐지 범위 내에 있으면 타이머 리셋
            client_executor->ResetTeleportTimer();
            return NodeStatus::FAILURE;  // 텔레포트 불필요
        }
    }

} // namespace condition
} // namespace bt
