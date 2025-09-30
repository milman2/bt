#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Sequence 노드 (순차 실행)
    class Sequence : public Node
    {
    public:
        Sequence(const std::string& name);
        NodeStatus Execute(Context& context) override;
    };

} // namespace bt
