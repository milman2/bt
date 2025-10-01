#pragma once

#include "../../../BT/Node.h"

namespace bt
{
namespace action
{

    // 가장 가까운 몬스터로 텔레포트하는 액션 노드
    class TeleportToNearest : public Node
    {
    public:
        TeleportToNearest(const std::string& name) : Node(name, NodeType::ACTION) {}
        virtual ~TeleportToNearest() = default;

        NodeStatus Execute(Context& context) override;
    };

} // namespace action
} // namespace bt
