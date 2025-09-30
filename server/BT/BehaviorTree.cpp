#include "BehaviorTree.h"
#include "BTContext.h"

namespace bt
{

    // BehaviorTree 구현
    BehaviorTree::BehaviorTree(const std::string& name) : name_(name) {}

    void BehaviorTree::SetRoot(std::shared_ptr<BTNode> root)
    {
        root_ = root;
    }

    BTNodeStatus BehaviorTree::Execute(BTContext& context)
    {
        if (root_)
        {
            return root_->Execute(context);
        }
        return BTNodeStatus::FAILURE;
    }

} // namespace bt
