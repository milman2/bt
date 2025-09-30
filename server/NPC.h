#pragma once

#include <cstdint>
#include <string>

namespace bt
{

    // NPC 클래스
    class NPC
    {
    public:
        NPC(uint32_t id);
        ~NPC();

        // 기본 정보
        uint32_t GetID() const { return id_; }
        const std::string& GetName() const { return name_; }
        void SetName(const std::string& name) { name_ = name; }

        // 위치
        float GetX() const { return x_; }
        float GetY() const { return y_; }
        float GetZ() const { return z_; }
        void SetPosition(float x, float y, float z);

        // 맵 ID
        uint32_t GetMapID() const { return map_id_; }
        void SetMapID(uint32_t map_id) { map_id_ = map_id; }

        // NPC 타입
        enum class NPCType
        {
            MERCHANT,
            GUARD,
            QUEST_GIVER,
            TRAINER
        };

        NPCType GetType() const { return type_; }
        void SetType(NPCType type) { type_ = type; }

    private:
        uint32_t id_;
        std::string name_;
        float x_, y_, z_;
        uint32_t map_id_;
        NPCType type_;
    };

} // namespace bt
