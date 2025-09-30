#include "BehaviorTreeEngine.h"
#include "MonsterAI.h"

#include <algorithm>

namespace bt
{

    // BehaviorTreeEngine 구현
    BehaviorTreeEngine::BehaviorTreeEngine() {}

    BehaviorTreeEngine::~BehaviorTreeEngine() {}

    void BehaviorTreeEngine::register_tree(const std::string& name, std::shared_ptr<BehaviorTree> tree)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        trees_[name] = tree;
    }

    std::shared_ptr<BehaviorTree> BehaviorTreeEngine::get_tree(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        auto                        it = trees_.find(name);
        if (it != trees_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void BehaviorTreeEngine::unregister_tree(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(trees_mutex_);
        trees_.erase(name);
    }

    BTNodeStatus BehaviorTreeEngine::execute_tree(const std::string& name, BTContext& context)
    {
        auto tree = get_tree(name);
        if (tree)
        {
            return tree->execute(context);
        }
        return BTNodeStatus::FAILURE;
    }

    void BehaviorTreeEngine::register_monster_ai(std::shared_ptr<MonsterAI> ai)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        monster_ais_.push_back(ai);
    }

    void BehaviorTreeEngine::unregister_monster_ai(std::shared_ptr<MonsterAI> ai)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        auto                        it = std::find(monster_ais_.begin(), monster_ais_.end(), ai);
        if (it != monster_ais_.end())
        {
            monster_ais_.erase(it);
        }
    }

    void BehaviorTreeEngine::update_all_monsters(float delta_time)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        for (auto& ai : monster_ais_)
        {
            if (ai && ai->is_active())
            {
                ai->update(delta_time);
            }
        }
    }

} // namespace bt
