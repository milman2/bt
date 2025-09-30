#pragma once

#include <memory>
#include <string>

#include "MonsterAI.h"

namespace bt
{

    // 몬스터 팩토리 클래스
    class MonsterFactory
    {
    public:
        static std::shared_ptr<Monster> create_monster(MonsterType            type,
                                                       const std::string&     name,
                                                       const MonsterPosition& position);
        static std::shared_ptr<Monster> create_monster(const MonsterSpawnConfig& config);

        // 몬스터별 기본 통계 설정
        static MonsterStats get_default_stats(MonsterType type);

        // 몬스터별 Behavior Tree 이름 반환
        static std::string get_bt_name(MonsterType type);

        // 문자열을 MonsterType으로 변환
        static MonsterType string_to_monster_type(const std::string& type_str);

        // MonsterType을 문자열로 변환
        static std::string monster_type_to_string(MonsterType type);
    };

} // namespace bt
