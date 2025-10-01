#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Repeat 노드 (반복 실행)
    class Repeat : public Node
    {
    public:
        Repeat(const std::string& name, int count = -1) // -1은 무한 반복
            : Node(name, NodeType::REPEAT), repeat_count_(count), current_count_(0)
        {
        }

        NodeStatus Execute(Context& context) override
        {
            if (children_.empty())
            {
                return NodeStatus::SUCCESS;
            }

            // 무한 반복인 경우
            if (repeat_count_ == -1)
            {
                NodeStatus child_status = children_[0]->Execute(context);
                if (child_status == NodeStatus::SUCCESS)
                {
                    current_count_ = 0; // 리셋하고 다시 시작
                    return NodeStatus::RUNNING;
                }
                return child_status;
            }

            // 제한된 반복인 경우
            if (current_count_ < repeat_count_)
            {
                NodeStatus child_status = children_[0]->Execute(context);
                if (child_status == NodeStatus::SUCCESS)
                {
                    current_count_++;
                    if (current_count_ >= repeat_count_)
                    {
                        current_count_ = 0; // 리셋
                        return NodeStatus::SUCCESS;
                    }
                    return NodeStatus::RUNNING;
                }
                else if (child_status == NodeStatus::FAILURE)
                {
                    current_count_ = 0; // 리셋
                    return NodeStatus::FAILURE;
                }
                return NodeStatus::RUNNING;
            }

            current_count_ = 0; // 리셋
            return NodeStatus::SUCCESS;
        }

    private:
        int repeat_count_;
        int current_count_;
    };

} // namespace bt
