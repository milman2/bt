#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "../../BT/Tree.h"
#include "../../BT/Context.h"
#include "../../BT/IExecutor.h"

namespace bt
{

    // 전방 선언
    class Monster;

    // 몬스터 Behavior Tree 실행자 클래스
    class MonsterBTExecutor : public IExecutor, public std::enable_shared_from_this<MonsterBTExecutor>
    {
    public:
        MonsterBTExecutor(const std::string& name, const std::string& bt_name);
        ~MonsterBTExecutor() = default;

        // AI 업데이트
        void Update(float delta_time) override;

        // Behavior Tree 설정
        void                          SetBehaviorTree(std::shared_ptr<Tree> tree) override;
        std::shared_ptr<Tree> GetBehaviorTree() const override { return behavior_tree_; }

        // 컨텍스트 접근
        Context&       GetContext() override { return context_; }
        const Context& GetContext() const override { return context_; }

        // 몬스터 정보
        const std::string& GetName() const override { return name_; }
        const std::string& GetBTName() const override { return bt_name_; }

        // 몬스터 참조 설정
        void                     SetMonster(std::shared_ptr<Monster> monster) { monster_ = monster; }
        std::shared_ptr<Monster> GetMonster() const { return monster_; }

        // 상태 관리
        bool IsActive() const override { return active_.load(); }
        void SetActive(bool active) override { active_.store(active); }

    private:
        std::string                           name_;
        std::string                           bt_name_;
        std::shared_ptr<Tree>         behavior_tree_;
        Context                             context_;
        std::shared_ptr<Monster>              monster_;
        std::atomic<bool>                     active_;
        std::chrono::steady_clock::time_point last_update_time_;
    };

} // namespace bt
