#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{
namespace action
{

    // 순찰 액션 노드
    class Patrol : public Node
    {
    public:
        Patrol(const std::string& name) : Node(name, NodeType::ACTION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace action
} // namespace bt
