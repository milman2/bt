#pragma once

#include <chrono>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Delay 노드 (지연 실행)
    class Delay : public Node
    {
    public:
        Delay(const std::string& name, std::chrono::milliseconds delay);
        NodeStatus Execute(Context& context) override;

    private:
        std::chrono::milliseconds             delay_;
        std::chrono::steady_clock::time_point start_time_;
        bool                                  started_;
    };

} // namespace bt
