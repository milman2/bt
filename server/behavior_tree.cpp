#include <algorithm>
#include <iostream>
#include <map>
#include <random>

#include "behavior_tree.h"

namespace bt
{

    // BTNode 기본 구현
    BTNode::BTNode(const std::string& name, BTNodeType type) : name_(name), type_(type) {}

    void BTNode::add_child(std::shared_ptr<BTNode> child)
    {
        children_.push_back(child);
    }

    // BTAction 구현
    BTAction::BTAction(const std::string& name, ActionFunction func)
        : BTNode(name, BTNodeType::ACTION), action_func_(func)
    {
    }

    BTNodeStatus BTAction::execute(BTContext& context)
    {
        if (action_func_)
        {
            last_status_ = action_func_(context);
        }
        else
        {
            last_status_ = BTNodeStatus::FAILURE;
        }
        return last_status_;
    }

    // BTCondition 구현
    BTCondition::BTCondition(const std::string& name, ConditionFunction func)
        : BTNode(name, BTNodeType::CONDITION), condition_func_(func)
    {
    }

    BTNodeStatus BTCondition::execute(BTContext& context)
    {
        if (condition_func_)
        {
            last_status_ = condition_func_(context) ? BTNodeStatus::SUCCESS : BTNodeStatus::FAILURE;
        }
        else
        {
            last_status_ = BTNodeStatus::FAILURE;
        }
        return last_status_;
    }

    // BTSequence 구현
    BTSequence::BTSequence(const std::string& name) : BTNode(name, BTNodeType::SEQUENCE) {}

    BTNodeStatus BTSequence::execute(BTContext& context)
    {
        for (auto& child : children_)
        {
            BTNodeStatus status = child->execute(context);
            if (status == BTNodeStatus::FAILURE)
            {
                last_status_ = BTNodeStatus::FAILURE;
                return last_status_;
            }
            else if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
        }
        last_status_ = BTNodeStatus::SUCCESS;
        return last_status_;
    }

    // BTSelector 구현
    BTSelector::BTSelector(const std::string& name) : BTNode(name, BTNodeType::SELECTOR) {}

    BTNodeStatus BTSelector::execute(BTContext& context)
    {
        for (auto& child : children_)
        {
            BTNodeStatus status = child->execute(context);
            if (status == BTNodeStatus::SUCCESS)
            {
                last_status_ = BTNodeStatus::SUCCESS;
                return last_status_;
            }
            else if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
        }
        last_status_ = BTNodeStatus::FAILURE;
        return last_status_;
    }

    // BTParallel 구현
    BTParallel::BTParallel(const std::string& name, Policy policy) : BTNode(name, BTNodeType::PARALLEL), policy_(policy)
    {
    }

    BTNodeStatus BTParallel::execute(BTContext& context)
    {
        std::vector<BTNodeStatus> results;
        results.reserve(children_.size());

        for (auto& child : children_)
        {
            results.push_back(child->execute(context));
        }

        switch (policy_)
        {
            case Policy::SUCCEED_ON_ONE:
                for (auto status : results)
                {
                    if (status == BTNodeStatus::SUCCESS)
                    {
                        last_status_ = BTNodeStatus::SUCCESS;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::FAILURE;
                break;

            case Policy::SUCCEED_ON_ALL:
                for (auto status : results)
                {
                    if (status != BTNodeStatus::SUCCESS)
                    {
                        last_status_ = status;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::SUCCESS;
                break;

            case Policy::FAIL_ON_ONE:
                for (auto status : results)
                {
                    if (status == BTNodeStatus::FAILURE)
                    {
                        last_status_ = BTNodeStatus::FAILURE;
                        return last_status_;
                    }
                }
                last_status_ = BTNodeStatus::SUCCESS;
                break;
        }

        return last_status_;
    }

    // BTRandom 구현
    BTRandom::BTRandom(const std::string& name) : BTNode(name, BTNodeType::RANDOM) {}

    BTNodeStatus BTRandom::execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        static std::random_device       rd;
        static std::mt19937             gen(rd());
        std::uniform_int_distribution<> dis(0, children_.size() - 1);

        int random_index = dis(gen);
        last_status_     = children_[random_index]->execute(context);
        return last_status_;
    }

    // BTRepeat 구현
    BTRepeat::BTRepeat(const std::string& name, int count)
        : BTNode(name, BTNodeType::REPEAT), repeat_count_(count), current_count_(0)
    {
    }

    BTNodeStatus BTRepeat::execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        auto& child = children_[0];

        while (repeat_count_ == -1 || current_count_ < repeat_count_)
        {
            BTNodeStatus status = child->execute(context);

            if (status == BTNodeStatus::RUNNING)
            {
                last_status_ = BTNodeStatus::RUNNING;
                return last_status_;
            }
            else if (status == BTNodeStatus::FAILURE)
            {
                last_status_ = BTNodeStatus::FAILURE;
                return last_status_;
            }

            current_count_++;
        }

        last_status_ = BTNodeStatus::SUCCESS;
        return last_status_;
    }

    // BTInvert 구현
    BTInvert::BTInvert(const std::string& name) : BTNode(name, BTNodeType::INVERT) {}

    BTNodeStatus BTInvert::execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        BTNodeStatus status = children_[0]->execute(context);

        switch (status)
        {
            case BTNodeStatus::SUCCESS:
                last_status_ = BTNodeStatus::FAILURE;
                break;
            case BTNodeStatus::FAILURE:
                last_status_ = BTNodeStatus::SUCCESS;
                break;
            case BTNodeStatus::RUNNING:
                last_status_ = BTNodeStatus::RUNNING;
                break;
        }

        return last_status_;
    }

    // BTDelay 구현
    BTDelay::BTDelay(const std::string& name, std::chrono::milliseconds delay)
        : BTNode(name, BTNodeType::DELAY), delay_(delay), started_(false)
    {
    }

    BTNodeStatus BTDelay::execute(BTContext& context)
    {
        auto now = std::chrono::steady_clock::now();

        if (!started_)
        {
            start_time_  = now;
            started_     = true;
            last_status_ = BTNodeStatus::RUNNING;
            return last_status_;
        }

        auto elapsed = now - start_time_;
        if (elapsed >= delay_)
        {
            started_     = false;
            last_status_ = BTNodeStatus::SUCCESS;
        }
        else
        {
            last_status_ = BTNodeStatus::RUNNING;
        }

        return last_status_;
    }

    // BTTimeout 구현
    BTTimeout::BTTimeout(const std::string& name, std::chrono::milliseconds timeout)
        : BTNode(name, BTNodeType::TIMEOUT), timeout_(timeout)
    {
    }

    BTNodeStatus BTTimeout::execute(BTContext& context)
    {
        if (children_.empty())
        {
            last_status_ = BTNodeStatus::FAILURE;
            return last_status_;
        }

        auto  start_time = std::chrono::steady_clock::now();
        auto& child      = children_[0];

        BTNodeStatus status = child->execute(context);

        if (status == BTNodeStatus::RUNNING)
        {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_)
            {
                last_status_ = BTNodeStatus::FAILURE;
            }
            else
            {
                last_status_ = BTNodeStatus::RUNNING;
            }
        }
        else
        {
            last_status_ = status;
        }

        return last_status_;
    }

    // BTContext 구현
    BTContext::BTContext()
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void BTContext::set_data(const std::string& key, const std::any& value)
    {
        data_[key] = value;
    }

    std::any BTContext::get_data(const std::string& key) const
    {
        auto it = data_.find(key);
        if (it != data_.end())
        {
            return it->second;
        }
        return std::any{};
    }

    bool BTContext::has_data(const std::string& key) const
    {
        return data_.find(key) != data_.end();
    }

    void BTContext::remove_data(const std::string& key)
    {
        data_.erase(key);
    }

    // BehaviorTree 구현
    BehaviorTree::BehaviorTree(const std::string& name) : name_(name) {}

    void BehaviorTree::set_root(std::shared_ptr<BTNode> root)
    {
        root_ = root;
    }

    BTNodeStatus BehaviorTree::execute(BTContext& context)
    {
        if (root_)
        {
            return root_->execute(context);
        }
        return BTNodeStatus::FAILURE;
    }

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

    // MonsterAI 구현
    MonsterAI::MonsterAI(const std::string& name, const std::string& bt_name)
        : name_(name), bt_name_(bt_name), active_(true)
    {
        last_update_time_ = std::chrono::steady_clock::now();
    }

    void MonsterAI::update(float delta_time)
    {
        static std::map<std::string, int> update_counts;
        std::string                       monster_name = name_; // AI 이름 사용
        update_counts[monster_name]++;

        if (update_counts[monster_name] % 100 == 0)
        { // 10초마다 로그 출력
            std::cout << "MonsterAI::update 호출됨: " << monster_name << " (카운트: " << update_counts[monster_name]
                      << ")" << std::endl;
        }

        if (!active_.load() || !behavior_tree_)
        {
            if (update_counts[monster_name] % 100 == 0)
            {
                std::cout << "MonsterAI::update - active: " << active_.load()
                          << ", behavior_tree: " << (behavior_tree_ ? "있음" : "없음") << std::endl;
            }
            return;
        }

        auto now = std::chrono::steady_clock::now();
        context_.set_start_time(now);

        // 몬스터 참조를 컨텍스트에 설정
        context_.set_monster_ai(shared_from_this());

        // Behavior Tree 실행
        behavior_tree_->execute(context_);

        last_update_time_ = now;
    }

    void MonsterAI::set_behavior_tree(std::shared_ptr<BehaviorTree> tree)
    {
        behavior_tree_ = tree;
    }

} // namespace bt
