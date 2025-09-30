#pragma once

#include <chrono>
#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Delay 노드 (지연 실행)
    class BTDelay : public BTNode
    {
    public:
        BTDelay(const std::string& name, std::chrono::milliseconds delay);
        BTNodeStatus execute(BTContext& context) override;

    private:
        std::chrono::milliseconds             delay_;
        std::chrono::steady_clock::time_point start_time_;
        bool                                  started_;
    };

} // namespace bt
