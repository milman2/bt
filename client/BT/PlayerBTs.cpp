#include "PlayerBTs.h"
#include "../../BT/Tree.h"
#include "../../BT/Control/Sequence.h"
#include "../../BT/Control/Selector.h"
#include "Action/Patrol.h"
#include "Action/Attack.h"
#include "Condition/HasTarget.h"
#include "Condition/InAttackRange.h"

#include <iostream>

namespace bt
{

    std::shared_ptr<Tree> PlayerBTs::CreatePlayerBT()
    {
        auto root = std::make_shared<bt::Selector>("player_root");
        
        // 공격 시퀀스
        auto attack_sequence = std::make_shared<bt::Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));
        
        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");
        
        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);
        
        auto tree = std::make_shared<Tree>("player_bt");
        tree->SetRoot(root);
        
        std::cout << "기본 플레이어 Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> PlayerBTs::CreateWarriorBT()
    {
        auto root = std::make_shared<bt::Selector>("warrior_root");
        
        // 공격 시퀀스 (전사는 더 공격적)
        auto attack_sequence = std::make_shared<bt::Sequence>("warrior_attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("warrior_attack"));
        
        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("warrior_patrol");
        
        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);
        
        auto tree = std::make_shared<Tree>("warrior_bt");
        tree->SetRoot(root);
        
        std::cout << "전사 플레이어 Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> PlayerBTs::CreateArcherBT()
    {
        auto root = std::make_shared<bt::Selector>("archer_root");
        
        // 공격 시퀀스 (궁수는 원거리 공격)
        auto attack_sequence = std::make_shared<bt::Sequence>("archer_attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("archer_attack"));
        
        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("archer_patrol");
        
        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);
        
        auto tree = std::make_shared<Tree>("archer_bt");
        tree->SetRoot(root);
        
        std::cout << "궁수 플레이어 Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> PlayerBTs::CreateMageBT()
    {
        auto root = std::make_shared<bt::Selector>("mage_root");
        
        // 공격 시퀀스 (마법사는 마법 공격)
        auto attack_sequence = std::make_shared<bt::Sequence>("mage_attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("mage_attack"));
        
        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("mage_patrol");
        
        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);
        
        auto tree = std::make_shared<Tree>("mage_bt");
        tree->SetRoot(root);
        
        std::cout << "마법사 플레이어 Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

} // namespace bt
