#pragma once

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
        Random(const std::string& name);
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
