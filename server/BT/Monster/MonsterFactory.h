#pragma once

#include <memory>
#include <string>

#include "MonsterTypes.h"
#include "Monster.h"

namespace bt
{

    // 몬스터 팩토리 클래스
    class MonsterFactory
    {
    public:
        static std::shared_ptr<Monster> CreateMonster(MonsterType            type,
                                                       const std::string&     name,
                                                       const MonsterPosition& position);
        static std::shared_ptr<Monster> CreateMonster(const MonsterSpawnConfig& config);

        // 몬스터별 기본 통계 설정
        static MonsterStats GetDefaultStats(MonsterType type);

        // 몬스터별 Behavior Tree 이름 반환
        static std::string GetBTName(MonsterType type);

        // 문자열을 MonsterType으로 변환
        static MonsterType StringToMonsterType(const std::string& type_str);

        // MonsterType을 문자열로 변환
        static std::string MonsterTypeToString(MonsterType type);
    };

} // namespace bt
