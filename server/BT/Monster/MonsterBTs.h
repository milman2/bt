#pragma once

#include <memory>
#include <string>

// 전방 선언
namespace bt
{
    class Tree;
    class Node;
} // namespace bt

namespace bt
{

    // 몬스터별 Behavior Tree 생성 클래스
    class MonsterBTs
    {
    public:
        // 각 몬스터별 BT 생성 함수
        static std::shared_ptr<Tree> CreateGoblinBT();
        static std::shared_ptr<Tree> CreateOrcBT();
        static std::shared_ptr<Tree> CreateDragonBT();
        static std::shared_ptr<Tree> CreateSkeletonBT();
        static std::shared_ptr<Tree> CreateZombieBT();
        static std::shared_ptr<Tree> CreateMerchantBT();
        static std::shared_ptr<Tree> CreateGuardBT();
    };

} // namespace bt
