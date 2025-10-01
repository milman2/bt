#include <iostream>

#include "../../BT/Control/Selector.h"
#include "../../BT/Control/Sequence.h"
#include "../../BT/Tree.h"
#include "Action/Attack.h"
#include "Action/Chase.h"
#include "Action/Patrol.h"
#include "Action/TeleportToNearest.h"
#include "Condition/HasTarget.h"
#include "Condition/InAttackRange.h"
#include "Condition/InDetectionRange.h"
#include "Condition/TeleportTimer.h"
#include "PlayerBTs.h"

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

        // 추적 시퀀스
        auto chase_sequence = std::make_shared<bt::Sequence>("chase_sequence");
        chase_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        chase_sequence->AddChild(std::make_shared<bt::condition::InDetectionRange>("in_detection_range"));
        chase_sequence->AddChild(std::make_shared<bt::action::Chase>("chase"));

        // 텔레포트 시퀀스 (3초 동안 몬스터를 찾지 못하면 텔레포트)
        auto teleport_sequence = std::make_shared<bt::Sequence>("teleport_sequence");
        teleport_sequence->AddChild(std::make_shared<bt::condition::TeleportTimer>("teleport_timer"));
        teleport_sequence->AddChild(std::make_shared<bt::action::TeleportToNearest>("teleport_to_nearest"));

        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("patrol");

        root->AddChild(attack_sequence);
        root->AddChild(chase_sequence);
        root->AddChild(teleport_sequence);
        root->AddChild(patrol_action);

        auto tree = std::make_shared<Tree>("player_bt");
        tree->SetRoot(root);

        std::cout << "기본 플레이어 Behavior Tree 생성 완료 (텔레포트 기능 포함)" << std::endl;
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

        // 추적 시퀀스
        auto chase_sequence = std::make_shared<bt::Sequence>("warrior_chase_sequence");
        chase_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        chase_sequence->AddChild(std::make_shared<bt::condition::InDetectionRange>("in_detection_range"));
        chase_sequence->AddChild(std::make_shared<bt::action::Chase>("warrior_chase"));

        // 텔레포트 시퀀스 (3초 동안 몬스터를 찾지 못하면 텔레포트)
        auto teleport_sequence = std::make_shared<bt::Sequence>("warrior_teleport_sequence");
        teleport_sequence->AddChild(std::make_shared<bt::condition::TeleportTimer>("warrior_teleport_timer"));
        teleport_sequence->AddChild(std::make_shared<bt::action::TeleportToNearest>("warrior_teleport_to_nearest"));

        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("warrior_patrol");

        root->AddChild(attack_sequence);
        root->AddChild(chase_sequence);
        root->AddChild(teleport_sequence);
        root->AddChild(patrol_action);

        auto tree = std::make_shared<Tree>("warrior_bt");
        tree->SetRoot(root);

        std::cout << "전사 플레이어 Behavior Tree 생성 완료 (텔레포트 기능 포함)" << std::endl;
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

        // 추적 시퀀스
        auto chase_sequence = std::make_shared<bt::Sequence>("archer_chase_sequence");
        chase_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        chase_sequence->AddChild(std::make_shared<bt::condition::InDetectionRange>("in_detection_range"));
        chase_sequence->AddChild(std::make_shared<bt::action::Chase>("archer_chase"));

        // 텔레포트 시퀀스 (3초 동안 몬스터를 찾지 못하면 텔레포트)
        auto teleport_sequence = std::make_shared<bt::Sequence>("archer_teleport_sequence");
        teleport_sequence->AddChild(std::make_shared<bt::condition::TeleportTimer>("archer_teleport_timer"));
        teleport_sequence->AddChild(std::make_shared<bt::action::TeleportToNearest>("archer_teleport_to_nearest"));

        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("archer_patrol");

        root->AddChild(attack_sequence);
        root->AddChild(chase_sequence);
        root->AddChild(teleport_sequence);
        root->AddChild(patrol_action);

        auto tree = std::make_shared<Tree>("archer_bt");
        tree->SetRoot(root);

        std::cout << "궁수 플레이어 Behavior Tree 생성 완료 (텔레포트 기능 포함)" << std::endl;
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

        // 추적 시퀀스
        auto chase_sequence = std::make_shared<bt::Sequence>("mage_chase_sequence");
        chase_sequence->AddChild(std::make_shared<bt::condition::HasTarget>("has_target"));
        chase_sequence->AddChild(std::make_shared<bt::condition::InDetectionRange>("in_detection_range"));
        chase_sequence->AddChild(std::make_shared<bt::action::Chase>("mage_chase"));

        // 텔레포트 시퀀스 (3초 동안 몬스터를 찾지 못하면 텔레포트)
        auto teleport_sequence = std::make_shared<bt::Sequence>("mage_teleport_sequence");
        teleport_sequence->AddChild(std::make_shared<bt::condition::TeleportTimer>("mage_teleport_timer"));
        teleport_sequence->AddChild(std::make_shared<bt::action::TeleportToNearest>("mage_teleport_to_nearest"));

        // 순찰 액션
        auto patrol_action = std::make_shared<bt::action::Patrol>("mage_patrol");

        root->AddChild(attack_sequence);
        root->AddChild(chase_sequence);
        root->AddChild(teleport_sequence);
        root->AddChild(patrol_action);

        auto tree = std::make_shared<Tree>("mage_bt");
        tree->SetRoot(root);

        std::cout << "마법사 플레이어 Behavior Tree 생성 완료 (텔레포트 기능 포함)" << std::endl;
        return tree;
    }

} // namespace bt
