#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{

    // 타겟이 있는지 확인하는 조건 노드
    class HasTargetCondition : public Node
    {
    public:
        HasTargetCondition(const std::string& name) : Node(name, NodeType::CONDITION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
