#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{

    // 공격 범위 내에 있는지 확인하는 조건 노드
    class InAttackRangeCondition : public Node
    {
    public:
        InAttackRangeCondition(const std::string& name) : Node(name, NodeType::CONDITION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
