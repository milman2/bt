#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{

    // 순찰 액션 노드
    class PatrolAction : public Node
    {
    public:
        PatrolAction(const std::string& name) : Node(name, NodeType::ACTION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
