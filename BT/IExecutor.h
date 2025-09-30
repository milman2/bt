#pragma once

#include <memory>
#include <string>

namespace bt
{

    // 전방 선언
    class Context;
    class Tree;

    // Behavior Tree 실행자 인터페이스
    class IExecutor
    {
    public:
        virtual ~IExecutor() = default;

        // AI 업데이트
        virtual void Update(float delta_time) = 0;

        // Behavior Tree 설정
        virtual void SetBehaviorTree(std::shared_ptr<Tree> tree) = 0;
        virtual std::shared_ptr<Tree> GetBehaviorTree() const = 0;

        // 컨텍스트 접근
        virtual Context& GetContext() = 0;
        virtual const Context& GetContext() const = 0;

        // 실행자 정보
        virtual const std::string& GetName() const = 0;
        virtual const std::string& GetBTName() const = 0;

        // 상태 관리
        virtual bool IsActive() const = 0;
        virtual void SetActive(bool active) = 0;
    };
} // namespace bt
