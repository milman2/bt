#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Parallel 노드 (병렬 실행)
    class Parallel : public Node
    {
    public:
        enum class Policy
        {
            SUCCEED_ON_ONE, // 하나라도 성공하면 성공
            SUCCEED_ON_ALL, // 모두 성공해야 성공
            FAIL_ON_ONE     // 하나라도 실패하면 실패
        };

        Parallel(const std::string& name, Policy policy = Policy::SUCCEED_ON_ONE) 
            : Node(name, NodeType::PARALLEL), policy_(policy) {}
            
        NodeStatus Execute(Context& context) override
        {
            if (children_.empty())
            {
                return NodeStatus::SUCCESS;
            }

            int success_count = 0;
            int failure_count = 0;
            int running_count = 0;

            // 모든 자식 노드를 병렬로 실행
            for (auto& child : children_)
            {
                NodeStatus status = child->Execute(context);
                switch (status)
                {
                    case NodeStatus::SUCCESS:
                        success_count++;
                        break;
                    case NodeStatus::FAILURE:
                        failure_count++;
                        break;
                    case NodeStatus::RUNNING:
                        running_count++;
                        break;
                }
            }

            // 정책에 따라 결과 결정
            switch (policy_)
            {
                case Policy::SUCCEED_ON_ONE:
                    if (success_count > 0) return NodeStatus::SUCCESS;
                    if (running_count > 0) return NodeStatus::RUNNING;
                    return NodeStatus::FAILURE;

                case Policy::SUCCEED_ON_ALL:
                    if (failure_count > 0) return NodeStatus::FAILURE;
                    if (running_count > 0) return NodeStatus::RUNNING;
                    return NodeStatus::SUCCESS;

                case Policy::FAIL_ON_ONE:
                    if (failure_count > 0) return NodeStatus::FAILURE;
                    if (running_count > 0) return NodeStatus::RUNNING;
                    return NodeStatus::SUCCESS;

                default:
                    return NodeStatus::FAILURE;
            }
        }

    private:
        Policy policy_;
    };

} // namespace bt
