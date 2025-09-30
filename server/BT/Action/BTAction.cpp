#include "BTAction.h"
#include "../BTContext.h"

namespace bt
{

    // BTAction 구현
    BTAction::BTAction(const std::string& name, ActionFunction func)
        : BTNode(name, BTNodeType::ACTION), action_func_(func)
    {
    }

    BTNodeStatus BTAction::execute(BTContext& context)
    {
        if (action_func_)
        {
            last_status_ = action_func_(context);
        }
        else
        {
            last_status_ = BTNodeStatus::FAILURE;
        }
        return last_status_;
    }

} // namespace bt
