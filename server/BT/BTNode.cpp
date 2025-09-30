#include "BTNode.h"

namespace bt
{

    // BTNode 기본 구현
    BTNode::BTNode(const std::string& name, BTNodeType type) : name_(name), type_(type) {}

    void BTNode::add_child(std::shared_ptr<BTNode> child)
    {
        children_.push_back(child);
    }

} // namespace bt
