#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Selector 노드 (하나라도 성공하면 성공)
    class BTSelector : public BTNode
    {
    public:
        BTSelector(const std::string& name);
        BTNodeStatus Execute(BTContext& context) override;
    };

} // namespace bt
