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

    // 몬스터 AI 클래스
    class MonsterAI : public std::enable_shared_from_this<MonsterAI>
    {
    public:
        MonsterAI(const std::string& name, const std::string& bt_name);
        ~MonsterAI() = default;

        // AI 업데이트
        void update(float delta_time);

        // Behavior Tree 설정
        void                          set_behavior_tree(std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> get_behavior_tree() const { return behavior_tree_; }

        // 컨텍스트 접근
        BTContext&       get_context() { return context_; }
        const BTContext& get_context() const { return context_; }

        // 몬스터 정보
        const std::string& GetName() const { return name_; }
        const std::string& GetBTName() const { return bt_name_; }

        // 몬스터 참조 설정
        void                     set_monster(std::shared_ptr<Monster> monster) { monster_ = monster; }
        std::shared_ptr<Monster> get_monster() const { return monster_; }

        // 상태 관리
        bool is_active() const { return active_.load(); }
        void set_active(bool active) { active_.store(active); }

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
