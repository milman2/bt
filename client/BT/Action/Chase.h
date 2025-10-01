#pragma once

#include "../../../BT/Node.h"

namespace bt
{
namespace action
{

    // 플레이어 추적 액션 노드
    class Chase : public Node
    {
    public:
        Chase(const std::string& name) : Node(name, NodeType::ACTION) {}
        
        NodeStatus Execute(Context& context) override;
    };

} // namespace action
} // namespace bt
