#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "BehaviorTree.h"
#include "BTContext.h"

namespace bt
{

    // 전방 선언
    class Monster;

    // 몬스터 Behavior Tree 실행자 클래스
    class MonsterBTExecutor : public std::enable_shared_from_this<MonsterBTExecutor>
    {
    public:
        MonsterBTExecutor(const std::string& name, const std::string& bt_name);
        ~MonsterBTExecutor() = default;

        // AI 업데이트
        void Update(float delta_time);

        // Behavior Tree 설정
        void                          SetBehaviorTree(std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> GetBehaviorTree() const { return behavior_tree_; }

        // 컨텍스트 접근
        BTContext&       GetContext() { return context_; }
        const BTContext& GetContext() const { return context_; }

        // 몬스터 정보
        const std::string& GetName() const { return name_; }
        const std::string& GetBTName() const { return bt_name_; }

        // 몬스터 참조 설정
        void                     SetMonster(std::shared_ptr<Monster> monster) { monster_ = monster; }
        std::shared_ptr<Monster> GetMonster() const { return monster_; }

        // 상태 관리
        bool IsActive() const { return active_.load(); }
        void SetActive(bool active) { active_.store(active); }

    private:
        std::string                           name_;
        std::string                           bt_name_;
        std::shared_ptr<BehaviorTree>         behavior_tree_;
        BTContext                             context_;
        std::shared_ptr<Monster>              monster_;
        std::atomic<bool>                     active_;
        std::chrono::steady_clock::time_point last_update_time_;
    };

} // namespace bt
