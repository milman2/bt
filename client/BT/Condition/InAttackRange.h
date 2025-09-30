#pragma once

#include "../../../BT/Node.h"

namespace bt
{
namespace condition
{

    // 공격 범위 내에 있는지 확인하는 조건 노드
    class InAttackRange : public Node
    {
    public:
        InAttackRange(const std::string& name) : Node(name, NodeType::CONDITION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace condition
} // namespace bt
