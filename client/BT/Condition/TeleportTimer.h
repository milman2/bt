#pragma once

#include "../../../BT/Node.h"

namespace bt
{
namespace condition
{

    // 텔레포트 타이머 조건 노드
    // 3초 동안 타겟을 찾지 못했는지 확인
    class TeleportTimer : public Node
    {
    public:
        TeleportTimer(const std::string& name) : Node(name, NodeType::CONDITION) {}
        virtual ~TeleportTimer() = default;

        NodeStatus Execute(Context& context) override;

    private:
        static constexpr float TELEPORT_TIMEOUT = 3.0f;  // 3초
    };

} // namespace condition
} // namespace bt
