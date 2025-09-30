#include "BTParallel.h"
#include "../BTContext.h"

namespace bt
{

    // BTParallel 구현
    BTParallel::BTParallel(const std::string& name, Policy policy) : BTNode(name, BTNodeType::PARALLEL), policy_(policy)
    {
    }

    BTNodeStatus BTParallel::execute(BTContext& context)
    {
        std::vector<BTNodeStatus> results;
        results.reserve(children_.size());

        for (auto& child : children_)
        {
            results.push_back(child->execute(context));
        }

        switch (policy_)
        {
            case Policy::SUCCEED_ON_ONE:
                for (auto status : results)
                {
                    if (status == BTNodeStatus::SUCCESS)
                    {
                        last_status_ = BTNodeStatus::SUCCESS;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::FAILURE;
                break;

            case Policy::SUCCEED_ON_ALL:
                for (auto status : results)
                {
                    if (status != BTNodeStatus::SUCCESS)
                    {
                        last_status_ = status;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::SUCCESS;
                break;

            case Policy::FAIL_ON_ONE:
                for (auto status : results)
                {
                    if (status == BTNodeStatus::FAILURE)
                    {
                        last_status_ = BTNodeStatus::FAILURE;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::SUCCESS;
                break;
        }

        return last_status_;
    }

} // namespace bt
