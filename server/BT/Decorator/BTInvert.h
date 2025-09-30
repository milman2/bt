#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Invert 노드 (결과 반전)
    class BTInvert : public BTNode
    {
    public:
        BTInvert(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

} // namespace bt
