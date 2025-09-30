#include "BTTimeout.h"
#include "../BTContext.h"

namespace bt
{

    // BTTimeout 구현
    BTTimeout::BTTimeout(const std::string& name, std::chrono::milliseconds timeout)
        : BTNode(name, BTNodeType::TIMEOUT), timeout_(timeout)
    {
    }

    BTNodeStatus BTTimeout::Execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        auto  start_time = std::chrono::steady_clock::now();
        auto& child      = children_[0];

        BTNodeStatus status = child->Execute(context);

        if (status == BTNodeStatus::RUNNING)
        {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_)
            {
                last_status_ = BTNodeStatus::FAILURE;
            }
            else
            {
                last_status_ = BTNodeStatus::RUNNING;
            }
        }
        else
        {
            last_status_ = status;
        }

        return last_status_;
    }

} // namespace bt
