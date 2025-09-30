#include "BTDelay.h"
#include "../BTContext.h"

namespace bt
{

    // BTDelay 구현
    BTDelay::BTDelay(const std::string& name, std::chrono::milliseconds delay)
        : BTNode(name, BTNodeType::DELAY), delay_(delay), started_(false)
    {
    }

    BTNodeStatus BTDelay::Execute(BTContext& /* context */)
    {
        auto now = std::chrono::steady_clock::now();

        if (!started_)
        {
            start_time_  = now;
            started_     = true;
            last_status_ = BTNodeStatus::RUNNING;
            return last_status_;
        }

        auto elapsed = now - start_time_;
        if (elapsed >= delay_)
        {
            started_     = false;
            last_status_ = BTNodeStatus::SUCCESS;
        }
        else
        {
            last_status_ = BTNodeStatus::RUNNING;
        }

        return last_status_;
    }

} // namespace bt
