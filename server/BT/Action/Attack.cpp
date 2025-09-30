#include "Attack.h"
#include "../../BT/Context.h"
#include "../Monster/MonsterBTExecutor.h"
#include "../Monster/Monster.h"

#include <iostream>

namespace bt
{
namespace action
{

    NodeStatus Attack::Execute(Context& context)
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

        // 공격 실행
        std::cout << "Goblin " << monster->GetName() << " attacks target!" << std::endl;
        
        // 공격 애니메이션/이펙트 처리
        monster->SetState(MonsterState::ATTACK);
        
        return NodeStatus::SUCCESS;
    }

} // namespace action
} // namespace bt
