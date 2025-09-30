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
        BehaviorTree(const std::string& name);
        ~BehaviorTree() = default;

        // 트리 구성
        void                    set_root(std::shared_ptr<BTNode> root);
        std::shared_ptr<BTNode> get_root() const { return root_; }

        // 트리 실행
        BTNodeStatus execute(BTContext& context);

        // 트리 정보
        const std::string& get_name() const { return name_; }

    private:
        std::string             name_;
        std::shared_ptr<BTNode> root_;
    };

} // namespace bt
