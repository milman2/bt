#include "BTCondition.h"
#include "../BTContext.h"

namespace bt
{

    // BTCondition 구현
    BTCondition::BTCondition(const std::string& name, ConditionFunction func)
        : BTNode(name, BTNodeType::CONDITION), condition_func_(func)
    {
    }

    BTNodeStatus BTCondition::Execute(BTContext& context)
    {
        if (condition_func_)
        {
            last_status_ = condition_func_(context) ? BTNodeStatus::SUCCESS : BTNodeStatus::FAILURE;
        }
        else
        {
            last_status_ = BTNodeStatus::FAILURE;
        }
        return last_status_;
    }

} // namespace bt
