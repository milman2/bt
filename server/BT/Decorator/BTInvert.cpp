#include "BTInvert.h"
#include "../BTContext.h"

namespace bt
{

    // BTInvert 구현
    BTInvert::BTInvert(const std::string& name) : BTNode(name, BTNodeType::INVERT) {}

    BTNodeStatus BTInvert::Execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        BTNodeStatus status = children_[0]->Execute(context);

        switch (status)
        {
            case BTNodeStatus::SUCCESS:
                last_status_ = BTNodeStatus::FAILURE;
                break;
            case BTNodeStatus::FAILURE:
                last_status_ = BTNodeStatus::SUCCESS;
                break;
            case BTNodeStatus::RUNNING:
                last_status_ = BTNodeStatus::RUNNING;
                break;
        }

        return last_status_;
    }

} // namespace bt
