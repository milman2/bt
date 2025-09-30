#pragma once

#include <chrono>
#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Timeout 노드 (시간 제한)
    class BTTimeout : public BTNode
    {
    public:
        BTTimeout(const std::string& name, std::chrono::milliseconds timeout);
        BTNodeStatus Execute(BTContext& context) override;

    private:
        std::chrono::milliseconds timeout_;
    };

} // namespace bt
