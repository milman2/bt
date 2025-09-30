#pragma once

#include <string>

#include "../BTNode.h"

namespace bt
{

    // 전방 선언
    class BTContext;

    // Parallel 노드 (병렬 실행)
    class BTParallel : public BTNode
    {
    public:
        enum class Policy
        {
            SUCCEED_ON_ONE, // 하나라도 성공하면 성공
            SUCCEED_ON_ALL, // 모두 성공해야 성공
            FAIL_ON_ONE     // 하나라도 실패하면 실패
        };

        BTParallel(const std::string& name, Policy policy = Policy::SUCCEED_ON_ONE);
        BTNodeStatus Execute(BTContext& context) override;

    private:
        Policy policy_;
    };

} // namespace bt
