#include "MonsterFactory.h"

#include <iostream>

#include "BehaviorTree.h"

namespace bt
{

    std::shared_ptr<Monster> MonsterFactory::create_monster(MonsterType            type,
                                                           const std::string&     name,
                                                           const MonsterPosition& position)
    {
        auto monster = std::make_shared<Monster>(name, type, position);

        // AI 이름과 BT 이름 설정
        monster->set_ai_name(name);
        std::string bt_name = get_bt_name(type);
        monster->set_bt_name(bt_name);

        return monster;
    }

    std::shared_ptr<Monster> MonsterFactory::create_monster(const MonsterSpawnConfig& config)
    {
        auto monster = std::make_shared<Monster>(config.name, config.type, config.position);
        monster->set_position(config.position.x, config.position.y, config.position.z, config.position.rotation);
        monster->heal(config.health);                                          // 체력 설정
        monster->take_damage(monster->get_stats().max_health - config.health); // 현재 체력 설정

        // AI 이름과 BT 이름 설정
        monster->set_ai_name(config.name);
        std::string bt_name = get_bt_name(config.type);
        monster->set_bt_name(bt_name);

        return monster;
    }

    MonsterStats MonsterFactory::get_default_stats(MonsterType type)
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

    std::string MonsterFactory::get_bt_name(MonsterType type)
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

    std::string MonsterFactory::monster_type_to_string(MonsterType type)
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

    MonsterType MonsterFactory::string_to_monster_type(const std::string& str)
    {
        if (str == "goblin")
            return MonsterType::GOBLIN;
        if (str == "orc")
            return MonsterType::ORC;
        if (str == "dragon")
            return MonsterType::DRAGON;
        if (str == "skeleton")
            return MonsterType::SKELETON;
        if (str == "zombie")
            return MonsterType::ZOMBIE;
        if (str == "merchant")
            return MonsterType::NPC_MERCHANT;
        if (str == "guard")
            return MonsterType::NPC_GUARD;
        return MonsterType::GOBLIN; // 기본값
    }

} // namespace bt
