#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Selector 노드 (하나라도 성공하면 성공)
    class Selector : public Node
    {
    public:
        Selector(const std::string& name);
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
