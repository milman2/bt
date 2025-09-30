#pragma once

#include <random>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Random 노드 (랜덤 선택)
    class Random : public Node
    {
    public:
        Random(const std::string& name) : Node(name, NodeType::RANDOM) {}
        
        NodeStatus Execute(Context& context) override
        {
            if (children_.empty())
            {
                return NodeStatus::FAILURE;
            }

            // 랜덤하게 자식 노드 선택
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, children_.size() - 1);
            
            int random_index = dis(gen);
            return children_[random_index]->Execute(context);
        }
    };

} // namespace bt
