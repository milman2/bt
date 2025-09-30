#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Sequence 노드 (순차 실행)
    class BTSequence : public BTNode
    {
    public:
        BTSequence(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

} // namespace bt
