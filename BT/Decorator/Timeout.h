#pragma once

#include <chrono>
#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Timeout 노드 (시간 제한)
    class Timeout : public Node
    {
    public:
        Timeout(const std::string& name, std::chrono::milliseconds timeout);
        NodeStatus Execute(Context& context) override;

    private:
        std::chrono::milliseconds timeout_;
    };

} // namespace bt
