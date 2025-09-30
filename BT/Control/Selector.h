#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Selector 노드 (하나라도 성공하면 성공)
    class Selector : public Node
    {
    public:
        Selector(const std::string& name) : Node(name, NodeType::SELECTOR) {}
        
        NodeStatus Execute(Context& context) override
        {
            for (auto& child : children_)
            {
                if (child)
                {
                    NodeStatus status = child->Execute(context);
                    if (status == NodeStatus::SUCCESS)
                    {
                        return NodeStatus::SUCCESS;
                    }
                }
            }
            return NodeStatus::FAILURE;
        }
    };

} // namespace bt
