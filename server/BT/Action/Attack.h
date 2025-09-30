#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{
namespace action
{

    // 공격 액션 노드
    class Attack : public Node
    {
    public:
        Attack(const std::string& name) : Node(name, NodeType::ACTION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace action
} // namespace bt
