#pragma once

#include <memory>

#include "../../BT/Tree.h"

namespace bt
{

    // 플레이어용 Behavior Tree 생성 클래스
    class PlayerBTs
    {
    public:
        // 기본 플레이어 BT 생성
        static std::shared_ptr<Tree> CreatePlayerBT();

        // 전사 플레이어 BT 생성
        static std::shared_ptr<Tree> CreateWarriorBT();

        // 궁수 플레이어 BT 생성
        static std::shared_ptr<Tree> CreateArcherBT();

        // 마법사 플레이어 BT 생성
        static std::shared_ptr<Tree> CreateMageBT();
    };

} // namespace bt
