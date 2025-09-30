#pragma once

#include <functional>
#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Condition 노드 (조건 노드)
    class BTCondition : public BTNode
    {
    public:
        using ConditionFunction = std::function<bool(BTContext&)>;

        BTCondition(const std::string& name, ConditionFunction func);
        BTNodeStatus Execute(BTContext& context) override;

    private:
        ConditionFunction condition_func_;
    };

} // namespace bt
