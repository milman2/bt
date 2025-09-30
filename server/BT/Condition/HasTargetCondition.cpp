#include "HasTargetCondition.h"
#include "../../BT/Context.h"
#include "../Monster/MonsterBTExecutor.h"
#include "../Monster/Monster.h"

#include <iostream>

namespace bt
{

    NodeStatus HasTargetCondition::Execute(Context& context)
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
        bool has_target = (monster->GetTargetID() != 0);
        
        if (has_target)
        {
            std::cout << "Goblin " << monster->GetName() << " has target: " << monster->GetTargetID() << std::endl;
            return NodeStatus::SUCCESS;
        }
        else
        {
            return NodeStatus::FAILURE;
        }
    }

} // namespace bt
