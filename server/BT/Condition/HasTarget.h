#pragma once

#include "../../BT/Node.h"
#include "../Monster/MonsterTypes.h"

namespace bt
{
namespace condition
{

    // 타겟이 있는지 확인하는 조건 노드
    class HasTarget : public Node
    {
    public:
        HasTarget(const std::string& name) : Node(name, NodeType::CONDITION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace condition
} // namespace bt
