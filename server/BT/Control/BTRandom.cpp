#include "BTRandom.h"
#include "../BTContext.h"

#include <random>

namespace bt
{

    // BTRandom 구현
    BTRandom::BTRandom(const std::string& name) : BTNode(name, BTNodeType::RANDOM) {}

    BTNodeStatus BTRandom::Execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        static std::random_device       rd;
        static std::mt19937             gen(rd());
        std::uniform_int_distribution<> dis(0, children_.size() - 1);

        int random_index = dis(gen);
        last_status_     = children_[random_index]->Execute(context);
        return last_status_;
    }

} // namespace bt
