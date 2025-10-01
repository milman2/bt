#include <iostream>

#include "../../BT/Control/Selector.h"
#include "../../BT/Control/Sequence.h"
#include "../../BT/Tree.h"
#include "../Action/Attack.h"
#include "../Action/Patrol.h"
#include "../Condition/HasTarget.h"
#include "../Condition/InAttackRange.h"
#include "MonsterBTs.h"

namespace bt
{

    std::shared_ptr<Tree> MonsterBTs::CreateGoblinBT()
    {
        auto tree = std::make_shared<Tree>("goblin_bt");

        // 루트 노드: Selector (하나라도 성공하면 성공)
        auto root = std::make_shared<Selector>("goblin_root");

        // 공격 시퀀스: 타겟이 있고 공격 범위 내에 있으면 공격
        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        // 루트에 자식 노드들 추가
        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Goblin Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateOrcBT()
    {
        auto tree = std::make_shared<Tree>("orc_bt");

        // 오크는 고블린과 비슷하지만 더 공격적
        auto root = std::make_shared<Selector>("orc_root");

        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Orc Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateDragonBT()
    {
        auto tree = std::make_shared<Tree>("dragon_bt");

        // 드래곤은 더 복잡한 AI
        auto root = std::make_shared<Selector>("dragon_root");

        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Dragon Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateSkeletonBT()
    {
        auto tree = std::make_shared<Tree>("skeleton_bt");

        auto root = std::make_shared<Selector>("skeleton_root");

        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Skeleton Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateZombieBT()
    {
        auto tree = std::make_shared<Tree>("zombie_bt");

        auto root = std::make_shared<Selector>("zombie_root");

        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Zombie Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateMerchantBT()
    {
        auto tree = std::make_shared<Tree>("merchant_bt");

        // 상인은 순찰만 함
        auto root = std::make_shared<bt::action::Patrol>("merchant_patrol");

        tree->SetRoot(root);

        std::cout << "Merchant Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

    std::shared_ptr<Tree> MonsterBTs::CreateGuardBT()
    {
        auto tree = std::make_shared<Tree>("guard_bt");

        // 경비병은 공격과 순찰
        auto root = std::make_shared<Selector>("guard_root");

        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        attack_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        attack_sequence->AddChild(std::make_shared<bt::condition::InAttackRange>("in_attack_range"));
        attack_sequence->AddChild(std::make_shared<bt::action::Attack>("attack"));

        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(patrol_action);

        tree->SetRoot(root);

        std::cout << "Guard Behavior Tree 생성 완료" << std::endl;
        return tree;
    }

} // namespace bt
