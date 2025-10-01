#include "MonsterFactory.h"
#include "MonsterBTExecutor.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../../BT/Tree.h"

namespace bt
{

    std::shared_ptr<Monster> MonsterFactory::CreateMonster(MonsterType            type,
                                                           const std::string&     name,
                                                           const MonsterPosition& position)
    {
        auto monster = std::make_shared<Monster>(name, type, position);

        // AI 이름과 BT 이름 설정
        monster->SetAIName(name);
        std::string bt_name = GetBTName(type);
        monster->SetBTName(bt_name);

        // AI 생성 및 설정
        auto ai = std::make_shared<MonsterBTExecutor>(name, bt_name);
        ai->SetMonster(monster);  // AI에 몬스터 참조 설정
        monster->SetAI(ai);

        return monster;
    }

    std::shared_ptr<Monster> MonsterFactory::CreateMonster(const MonsterSpawnConfig& config)
    {
        auto monster = std::make_shared<Monster>(config.name, config.type, config.position);
        monster->SetPosition(config.position.x, config.position.y, config.position.z, config.position.rotation);
        monster->Heal(config.health);                                          // 체력 설정
        monster->TakeDamage(monster->GetStats().max_health - config.health); // 현재 체력 설정

        // AI 이름과 BT 이름 설정
        monster->SetAIName(config.name);
        std::string bt_name = GetBTName(config.type);
        monster->SetBTName(bt_name);

        // AI 생성 및 설정
        auto ai = std::make_shared<MonsterBTExecutor>(config.name, bt_name);
        ai->SetMonster(monster);  // AI에 몬스터 참조 설정
        monster->SetAI(ai);

        return monster;
    }

    MonsterStats MonsterFactory::GetDefaultStats(MonsterType type)
    {
        MonsterStats stats;

        switch (type)
        {
            case MonsterType::GOBLIN:
                stats.level           = 1;
                stats.health          = 50;
                stats.max_health      = 50;
                stats.mana            = 20;
                stats.max_mana        = 20;
                stats.attack_power    = 15;
                stats.defense         = 5;
                stats.move_speed      = 2.0f;
                stats.attack_range    = 1.5f;
                stats.detection_range = 50.0f; // 탐지 범위 증가
                break;

            case MonsterType::ORC:
                stats.level           = 3;
                stats.health          = 100;
                stats.max_health      = 100;
                stats.mana            = 30;
                stats.max_mana        = 30;
                stats.attack_power    = 25;
                stats.defense         = 10;
                stats.move_speed      = 1.5f;
                stats.attack_range    = 2.0f;
                stats.detection_range = 60.0f; // 탐지 범위 증가
                break;

            case MonsterType::DRAGON:
                stats.level           = 10;
                stats.health          = 500;
                stats.max_health      = 500;
                stats.mana            = 200;
                stats.max_mana        = 200;
                stats.attack_power    = 80;
                stats.defense         = 30;
                stats.move_speed      = 3.0f;
                stats.attack_range    = 5.0f;
                stats.detection_range = 100.0f;
                break;

            case MonsterType::SKELETON:
                stats.level           = 2;
                stats.health          = 80;
                stats.max_health      = 80;
                stats.mana            = 0;
                stats.max_mana        = 0;
                stats.attack_power    = 20;
                stats.defense         = 8;
                stats.move_speed      = 1.8f;
                stats.attack_range    = 1.8f;
                stats.detection_range = 40.0f;
                break;

            case MonsterType::ZOMBIE:
                stats.level           = 1;
                stats.health          = 100;
                stats.max_health      = 100;
                stats.mana            = 10;
                stats.max_mana        = 10;
                stats.attack_power    = 10;
                stats.defense         = 4;
                stats.move_speed      = 0.5f;
                stats.attack_range    = 1.2f;
                stats.detection_range = 6.0f;
                break;

            case MonsterType::NPC_MERCHANT:
                stats.level           = 1;
                stats.health          = 50;
                stats.max_health      = 50;
                stats.mana            = 100;
                stats.max_mana        = 100;
                stats.attack_power    = 5;
                stats.defense         = 2;
                stats.move_speed      = 1.0f;
                stats.attack_range    = 0.0f;
                stats.detection_range = 5.0f;
                break;

            case MonsterType::NPC_GUARD:
                stats.level           = 5;
                stats.health          = 200;
                stats.max_health      = 200;
                stats.mana            = 80;
                stats.max_mana        = 80;
                stats.attack_power    = 25;
                stats.defense         = 15;
                stats.move_speed      = 1.5f;
                stats.attack_range    = 3.0f;
                stats.detection_range = 15.0f;
                break;
        }

        return stats;
    }

    std::string MonsterFactory::GetBTName(MonsterType type)
    {
        switch (type)
        {
            case MonsterType::GOBLIN:
                return "goblin_bt";
            case MonsterType::ORC:
                return "orc_bt";
            case MonsterType::DRAGON:
                return "dragon_bt";
            case MonsterType::SKELETON:
                return "skeleton_bt";
            case MonsterType::ZOMBIE:
                return "zombie_bt";
            case MonsterType::NPC_MERCHANT:
                return "merchant_bt";
            case MonsterType::NPC_GUARD:
                return "guard_bt";
            default:
                return "default_bt";
        }
    }

    std::string MonsterFactory::MonsterTypeToString(MonsterType type)
    {
        switch (type)
        {
            case MonsterType::GOBLIN:
                return "goblin";
            case MonsterType::ORC:
                return "orc";
            case MonsterType::DRAGON:
                return "dragon";
            case MonsterType::SKELETON:
                return "skeleton";
            case MonsterType::ZOMBIE:
                return "zombie";
            case MonsterType::NPC_MERCHANT:
                return "merchant";
            case MonsterType::NPC_GUARD:
                return "guard";
            default:
                return "unknown";
        }
    }

    MonsterType MonsterFactory::StringToMonsterType(const std::string& str)
    {
        // 대소문자 구분 없이 처리
        std::string lower_str = str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
        
        if (lower_str == "goblin")
            return MonsterType::GOBLIN;
        if (lower_str == "orc")
            return MonsterType::ORC;
        if (lower_str == "dragon")
            return MonsterType::DRAGON;
        if (lower_str == "skeleton")
            return MonsterType::SKELETON;
        if (lower_str == "zombie")
            return MonsterType::ZOMBIE;
        if (lower_str == "merchant" || lower_str == "npc_merchant")
            return MonsterType::NPC_MERCHANT;
        if (lower_str == "guard" || lower_str == "npc_guard")
            return MonsterType::NPC_GUARD;
        return MonsterType::GOBLIN; // 기본값
    }

} // namespace bt
