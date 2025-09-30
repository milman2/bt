#pragma once

#include <chrono>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Timeout 노드 (시간 제한)
    class Timeout : public Node
    {
    public:
        Timeout(const std::string& name, std::chrono::milliseconds timeout) 
            : Node(name, NodeType::TIMEOUT), timeout_(timeout), start_time_(std::chrono::steady_clock::now()) {}
            
        NodeStatus Execute(Context& context) override
        {
            if (children_.empty())
            {
                return NodeStatus::SUCCESS;
            }
            
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - start_time_;
            
            // 시간 초과 확인
            if (elapsed >= timeout_)
            {
                start_time_ = now; // 리셋
                return NodeStatus::FAILURE;
            }
            
            // 자식 노드 실행
            NodeStatus child_status = children_[0]->Execute(context);
            
            // 자식이 완료되면 시간 리셋
            if (child_status != NodeStatus::RUNNING)
            {
                start_time_ = now;
            }
            
            return child_status;
        }

    private:
        std::chrono::milliseconds             timeout_;
        std::chrono::steady_clock::time_point start_time_;
    };

} // namespace bt
