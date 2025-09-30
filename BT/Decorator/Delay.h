#pragma once

#include <chrono>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Delay 노드 (지연 실행)
    class Delay : public Node
    {
    public:
        Delay(const std::string& name, std::chrono::milliseconds delay) 
            : Node(name, NodeType::DELAY), delay_(delay), started_(false) {}
            
        NodeStatus Execute(Context& context) override
        {
            auto now = std::chrono::steady_clock::now();
            
            if (!started_)
            {
                start_time_ = now;
                started_ = true;
                return NodeStatus::RUNNING;
            }
            
            auto elapsed = now - start_time_;
            if (elapsed >= delay_)
            {
                started_ = false; // 다음 실행을 위해 리셋
                
                // 자식 노드가 있으면 실행
                if (!children_.empty())
                {
                    return children_[0]->Execute(context);
                }
                return NodeStatus::SUCCESS;
            }
            
            return NodeStatus::RUNNING;
        }

    private:
        std::chrono::milliseconds             delay_;
        std::chrono::steady_clock::time_point start_time_;
        bool                                  started_;
    };

} // namespace bt
