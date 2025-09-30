#pragma once

#include <functional>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Condition 노드 (조건 노드)
    class Condition : public Node
    {
    public:
        using ConditionFunction = std::function<bool(Context&)>;

        Condition(const std::string& name, ConditionFunction func) 
            : Node(name, NodeType::CONDITION), condition_func_(func) {}
            
        NodeStatus Execute(Context& context) override
        {
            if (condition_func_)
            {
                bool result = condition_func_(context);
                return result ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
            }
            return NodeStatus::FAILURE;
        }

    private:
        ConditionFunction condition_func_;
    };

} // namespace bt
