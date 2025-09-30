#include "BTRepeat.h"
#include "../BTContext.h"

namespace bt
{

    // BTRepeat 구현
    BTRepeat::BTRepeat(const std::string& name, int count)
        : BTNode(name, BTNodeType::REPEAT), repeat_count_(count), current_count_(0)
    {
    }

    BTNodeStatus BTRepeat::execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        auto& child = children_[0];

        while (repeat_count_ == -1 || current_count_ < repeat_count_)
        {
            BTNodeStatus status = child->execute(context);

            if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
            else if (status == BTNodeStatus::FAILURE)
            {
                last_status_ = BTNodeStatus::FAILURE;
                return last_status_;
            }

            current_count_++;
        }

        last_status_ = BTNodeStatus::SUCCESS;
        return last_status_;
    }

} // namespace bt
