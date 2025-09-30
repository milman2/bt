#pragma once

#include <string>

#include "../Node.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Repeat 노드 (반복 실행)
    class Repeat : public Node
    {
    public:
        Repeat(const std::string& name, int count = -1); // -1은 무한 반복
        NodeStatus Execute(Context& context) override;

    private:
        int repeat_count_;
        int current_count_;
    };

} // namespace bt
