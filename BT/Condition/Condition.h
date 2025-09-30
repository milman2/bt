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

        Condition(const std::string& name, ConditionFunction func);
        NodeStatus Execute(Context& context) override;

    private:
        ConditionFunction condition_func_;
    };

} // namespace bt
