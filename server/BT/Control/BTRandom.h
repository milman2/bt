#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Random 노드 (랜덤 선택)
    class BTRandom : public BTNode
    {
    public:
        BTRandom(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

} // namespace bt
