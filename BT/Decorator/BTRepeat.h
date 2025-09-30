#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Repeat 노드 (반복 실행)
    class BTRepeat : public BTNode
    {
    public:
        BTRepeat(const std::string& name, int count = -1); // -1은 무한 반복
        BTNodeStatus Execute(BTContext& context) override;

    private:
        int repeat_count_;
        int current_count_;
    };

} // namespace bt
