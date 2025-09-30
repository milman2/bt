#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "../../BT/Context.h"
#include "../../BT/IExecutor.h"
#include "../../BT/Tree.h"

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

        // IExecutor 인터페이스 구현
        void                  Update(float delta_time) override;                    // AI 업데이트
        void                  SetBehaviorTree(std::shared_ptr<Tree> tree) override; // Behavior Tree 설정
        std::shared_ptr<Tree> GetBehaviorTree() const override { return behavior_tree_; }
        Context&              GetContext() override { return context_; } // 컨텍스트 접근
        const Context&        GetContext() const override { return context_; }
        const std::string&    GetName() const override { return name_; } // 이름 접근
        const std::string&    GetBTName() const override { return bt_name_; }
        bool                  IsActive() const override { return active_.load(); } // 상태 관리
        void                  SetActive(bool active) override { active_.store(active); }

        // 몬스터 참조 설정
        void                     SetMonster(std::shared_ptr<Monster> monster) { monster_ = monster; }
        std::shared_ptr<Monster> GetMonster() const { return monster_; }

    private:
        std::shared_ptr<Tree> behavior_tree_;
        Context               context_;
        std::string           name_;
        std::string           bt_name_;
        std::atomic<bool>     active_;

        std::shared_ptr<Monster>              monster_;
        std::chrono::steady_clock::time_point last_update_time_;
    };

} // namespace bt
