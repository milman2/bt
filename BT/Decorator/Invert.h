#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Invert 노드 (결과 반전)
    class Invert : public Node
    {
    public:
        Invert(const std::string& name);
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
