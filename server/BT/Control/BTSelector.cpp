#include "BTSelector.h"
#include "../BTContext.h"

namespace bt
{

    // BTSelector 구현
    BTSelector::BTSelector(const std::string& name) : BTNode(name, BTNodeType::SELECTOR) {}

    BTNodeStatus BTSelector::execute(BTContext& context)
    {
        for (auto& child : children_)
        {
            BTNodeStatus status = child->execute(context);
            if (status == BTNodeStatus::SUCCESS)
            {
                last_status_ = BTNodeStatus::SUCCESS;
                return last_status_;
            }
            else if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
        }
        last_status_ = BTNodeStatus::FAILURE;
        return last_status_;
    }

} // namespace bt
