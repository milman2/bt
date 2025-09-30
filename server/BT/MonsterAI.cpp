#include "Monster/MonsterAI.h"

#include <iostream>
#include <map>

namespace bt
{

    // MonsterAI 구현
    MonsterAI::MonsterAI(const std::string& name, const std::string& bt_name)
        : name_(name), bt_name_(bt_name), active_(true)
    {
        last_update_time_ = std::chrono::steady_clock::now();
    }

    void MonsterAI::update(float /* delta_time */)
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
