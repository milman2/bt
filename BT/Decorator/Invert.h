#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Invert 노드 (결과 반전)
    class Invert : public Node
    {
    public:
        Invert(const std::string& name) : Node(name, NodeType::INVERT) {}

        NodeStatus Execute(Context& context) override
        {
            if (children_.empty())
            {
                return NodeStatus::SUCCESS; // 자식이 없으면 성공 반환
            }

            NodeStatus child_status = children_[0]->Execute(context);

            // 결과 반전
            switch (child_status)
            {
                case NodeStatus::SUCCESS:
                    return NodeStatus::FAILURE;
                case NodeStatus::FAILURE:
                    return NodeStatus::SUCCESS;
                case NodeStatus::RUNNING:
                    return NodeStatus::RUNNING;
                default:
                    return NodeStatus::FAILURE;
            }
        }
    };

} // namespace bt
