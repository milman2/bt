#include "HasTarget.h"
#include "../../../BT/Context.h"
#include "../../AsioTestClient.h"

#include <iostream>

namespace bt
{
namespace condition
{

    NodeStatus HasTarget::Execute(Context& context)
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
        uint32_t target_id = client_executor->GetTargetID();
        bool has_target = client_executor->HasTarget();
        
        std::cout << "HasTarget 조건 체크: target_id=" << target_id 
                  << ", has_target=" << (has_target ? "true" : "false") << std::endl;
        
        if (has_target)
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 타겟 발견: ID " 
                      << target_id << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            std::cout << "플레이어 " << client_executor->GetName() << " 타겟 없음" << std::endl;
            return NodeStatus::FAILURE;
        }
    }

} // namespace condition
} // namespace bt
