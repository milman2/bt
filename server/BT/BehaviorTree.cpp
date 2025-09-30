#include "BehaviorTree.h"
#include "BTContext.h"

namespace bt
{

    // BehaviorTree 구현
    BehaviorTree::BehaviorTree(const std::string& name) : name_(name) {}

    void BehaviorTree::set_root(std::shared_ptr<BTNode> root)
    {
        root_ = root;
    }

    BTNodeStatus BehaviorTree::execute(BTContext& context)
    {
        if (root_)
        {
            return root_->execute(context);
        }
        return BTNodeStatus::FAILURE;
    }

} // namespace bt
