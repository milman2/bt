#pragma once

#include "../../../BT/Node.h"

namespace bt
{
namespace condition
{

    // 탐지 범위 내에 있는지 확인하는 조건 노드
    class InDetectionRange : public Node
    {
    public:
        InDetectionRange(const std::string& name) : Node(name, NodeType::CONDITION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace condition
} // namespace bt
