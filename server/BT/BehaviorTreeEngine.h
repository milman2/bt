#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "BehaviorTree.h"
#include "BTContext.h"

namespace bt
{

    // 전방 선언
    class MonsterAI;

    // Behavior Tree 엔진
    class BehaviorTreeEngine
    {
    public:
        BehaviorTreeEngine();
        ~BehaviorTreeEngine();

        // 트리 관리
        void                          register_tree(const std::string& name, std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> get_tree(const std::string& name);
        void                          unregister_tree(const std::string& name);

        // 트리 실행
        BTNodeStatus execute_tree(const std::string& name, BTContext& context);

        // 몬스터 AI 관리
        void register_monster_ai(std::shared_ptr<MonsterAI> ai);
        void unregister_monster_ai(std::shared_ptr<MonsterAI> ai);
        void update_all_monsters(float delta_time);

        // 통계
        size_t get_registered_trees() const { return trees_.size(); }
        size_t get_active_monsters() const { return monster_ais_.size(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<BehaviorTree>> trees_;
        std::vector<std::shared_ptr<MonsterAI>>                        monster_ais_;
        mutable std::mutex                                             trees_mutex_;
        mutable std::mutex                                             monsters_mutex_;
    };

} // namespace bt
