#include "BehaviorTreeEngine.h"
#include "Monster/MonsterBTExecutor.h"

#include <algorithm>

namespace bt
{

    // BehaviorTreeEngine 구현
    BehaviorTreeEngine::BehaviorTreeEngine() {}

    BehaviorTreeEngine::~BehaviorTreeEngine() {}

    void BehaviorTreeEngine::RegisterTree(const std::string& name, std::shared_ptr<BehaviorTree> tree)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        trees_[name] = tree;
    }

    std::shared_ptr<BehaviorTree> BehaviorTreeEngine::GetTree(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        auto                        it = trees_.find(name);
        if (it != trees_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void BehaviorTreeEngine::UnregisterTree(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        trees_.erase(name);
    }

    BTNodeStatus BehaviorTreeEngine::ExecuteTree(const std::string& name, BTContext& context)
    {
        auto tree = GetTree(name);
        if (tree)
        {
            return tree->Execute(context);
        }
        return BTNodeStatus::FAILURE;
    }

    void BehaviorTreeEngine::RegisterMonsterAI(std::shared_ptr<MonsterBTExecutor> ai)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        monster_ais_.push_back(ai);
    }

    void BehaviorTreeEngine::UnregisterMonsterAI(std::shared_ptr<MonsterBTExecutor> ai)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        auto                        it = std::find(monster_ais_.begin(), monster_ais_.end(), ai);
        if (it != monster_ais_.end())
        {
            monster_ais_.erase(it);
        }
    }

    void BehaviorTreeEngine::UpdateAllMonsters(float delta_time)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        for (auto& ai : monster_ais_)
        {
            if (ai && ai->IsActive())
            {
                ai->Update(delta_time);
            }
        }
    }

} // namespace bt
