#pragma once

#include <memory>
#include <string>

#include "Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Behavior Tree 클래스
    class Tree
    {
    public:
        Tree(const std::string& name) : name_(name) {}
        ~Tree() = default;

        // 트리 구성
        void SetRoot(std::shared_ptr<Node> root) { root_ = root; }
        std::shared_ptr<Node> GetRoot() const { return root_; }

        // 트리 실행
        NodeStatus Execute(Context& context)
        {
            if (root_)
            {
                return root_->Execute(context);
            }
            return NodeStatus::FAILURE;
        }

        // 트리 정보
        const std::string& GetName() const { return name_; }

    private:
        std::string             name_;
        std::shared_ptr<Node> root_;
    };

} // namespace bt
