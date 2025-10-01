#pragma once

#include <functional>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Action 노드 (실행 노드)
    class Action : public Node
    {
    public:
        using ActionFunction = std::function<NodeStatus(Context&)>;

        Action(const std::string& name, ActionFunction func) : Node(name, NodeType::ACTION), action_func_(func) {}

        NodeStatus Execute(Context& context) override
        {
            if (action_func_)
            {
                return action_func_(context);
            }
            return NodeStatus::FAILURE;
        }

    private:
        ActionFunction action_func_;
    };

} // namespace bt
