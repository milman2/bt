#include "BTSequence.h"
#include "../BTContext.h"

namespace bt
{

    // BTSequence 구현
    BTSequence::BTSequence(const std::string& name) : BTNode(name, BTNodeType::SEQUENCE) {}

    BTNodeStatus BTSequence::Execute(BTContext& context)
    {
        for (auto& child : children_)
        {
            BTNodeStatus status = child->Execute(context);
            if (status == BTNodeStatus::FAILURE)
            {
                last_status_ = BTNodeStatus::FAILURE;
                return last_status_;
            }
            else if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
        }
        last_status_ = BTNodeStatus::SUCCESS;
        return last_status_;
    }

} // namespace bt
