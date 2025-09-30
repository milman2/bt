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
    class MonsterBTExecutor;

    // Behavior Tree 엔진
    class BehaviorTreeEngine
    {
    public:
        BehaviorTreeEngine();
        ~BehaviorTreeEngine();

        // 트리 관리
        void                          RegisterTree(const std::string& name, std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> GetTree(const std::string& name);
        void                          UnregisterTree(const std::string& name);

        // 트리 실행
        BTNodeStatus ExecuteTree(const std::string& name, BTContext& context);

        // 몬스터 AI 관리
        void RegisterMonsterAI(std::shared_ptr<MonsterBTExecutor> ai);
        void UnregisterMonsterAI(std::shared_ptr<MonsterBTExecutor> ai);
        void UpdateAllMonsters(float delta_time);

        // 통계
        size_t GetRegisteredTrees() const { return trees_.size(); }
        size_t GetActiveMonsters() const { return monster_ais_.size(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<BehaviorTree>> trees_;
        std::vector<std::shared_ptr<MonsterBTExecutor>>                monster_ais_;
        mutable std::mutex                                             trees_mutex_;
        mutable std::mutex                                             monsters_mutex_;
    };

} // namespace bt
