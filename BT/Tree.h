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
        Tree(const std::string& name) : name_(name), last_status_(NodeStatus::FAILURE), is_running_(false) {}
        ~Tree() = default;

        // 트리 구성
        void                  SetRoot(std::shared_ptr<Node> root) { root_ = root; }
        std::shared_ptr<Node> GetRoot() const { return root_; }

        // 트리 실행
        NodeStatus Execute(Context& context)
        {
            if (!root_)
                return NodeStatus::FAILURE;

            // 이전 상태가 RUNNING이 아니면 트리 초기화
            if (last_status_ != NodeStatus::RUNNING)
            {
                InitializeTree();
            }

            // 트리 실행
            last_status_ = root_->Execute(context);
            is_running_  = (last_status_ == NodeStatus::RUNNING);

            return last_status_;
        }

        // 트리 정보
        const std::string& GetName() const { return name_; }
        NodeStatus         GetLastStatus() const { return last_status_; }
        bool               IsRunning() const { return is_running_; }

        // 트리 초기화
        void InitializeTree()
        {
            if (root_)
            {
                InitializeNode(root_);
            }
        }

    private:
        void InitializeNode(std::shared_ptr<Node> node)
        {
            if (!node)
                return;

            node->Initialize();
            for (auto& child : node->GetChildren())
            {
                InitializeNode(child);
            }
        }

        std::string           name_;
        std::shared_ptr<Node> root_;
        NodeStatus            last_status_;
        bool                  is_running_;
    };

} // namespace bt
