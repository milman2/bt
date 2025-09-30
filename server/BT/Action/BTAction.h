#pragma once

#include <functional>
#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Action 노드 (실행 노드)
    class BTAction : public BTNode
    {
    public:
        using ActionFunction = std::function<BTNodeStatus(BTContext&)>;

        BTAction(const std::string& name, ActionFunction func);
        BTNodeStatus Execute(BTContext& context) override;

    private:
        ActionFunction action_func_;
    };

} // namespace bt
