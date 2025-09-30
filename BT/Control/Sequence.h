#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Sequence 노드 (순차 실행)
    class Sequence : public Node
    {
    public:
        Sequence(const std::string& name) : Node(name, NodeType::SEQUENCE) {}
        
        NodeStatus Execute(Context& context) override
        {
            // 모든 자식 노드를 순차적으로 실행
            // 하나라도 실패하면 실패 반환
            for (auto& child : children_)
            {
                if (!child)
                    continue;
                    
                NodeStatus status = child->Execute(context);
                if (status == NodeStatus::FAILURE)
                {
                    return NodeStatus::FAILURE;
                }
                else if (status == NodeStatus::RUNNING)
                {
                    return NodeStatus::RUNNING;
                }
            }
            return NodeStatus::SUCCESS;
        }
    };

} // namespace bt
