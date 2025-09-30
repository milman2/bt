#include "InAttackRange.h"
#include "../../BT/Context.h"
#include "../Monster/MonsterBTExecutor.h"
#include "../Monster/Monster.h"

#include <iostream>
#include <cmath>

namespace bt
{
namespace condition
{

    NodeStatus InAttackRange::Execute(Context& context)
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

        // 타겟이 있는지 확인
        if (monster->GetTargetID() == 0)
        {
            return NodeStatus::FAILURE;
        }

        // TODO: 실제 타겟 위치를 가져와서 거리 계산
        // 현재는 시뮬레이션을 위해 랜덤하게 성공/실패 반환
        static int call_count = 0;
        call_count++;
        
        // 10번 중 3번 정도만 공격 범위 내에 있다고 가정
        bool in_range = (call_count % 10) < 3;
        
        if (in_range)
        {
            std::cout << "Goblin " << monster->GetName() << " is in attack range!" << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace condition
} // namespace bt
