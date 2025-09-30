#pragma once

#include <memory>
#include <string>

#include "BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Behavior Tree 클래스
    class BehaviorTree
    {
    public:
        BehaviorTree(const std::string& name) : name_(name) {}
        ~BehaviorTree() = default;

        // 트리 구성
        void SetRoot(std::shared_ptr<BTNode> root) { root_ = root; }
        std::shared_ptr<BTNode> GetRoot() const { return root_; }

        // 트리 실행
        BTNodeStatus Execute(BTContext& context)
        {
            if (root_)
            {
                return root_->Execute(context);
            }
            return BTNodeStatus::FAILURE;
        }

        // 트리 정보
        const std::string& GetName() const { return name_; }

    private:
        std::string             name_;
        std::shared_ptr<BTNode> root_;
    };

} // namespace bt
