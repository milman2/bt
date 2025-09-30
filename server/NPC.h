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
        uint32_t get_id() const { return id_; }
        const std::string& get_name() const { return name_; }
        void set_name(const std::string& name) { name_ = name; }

        // 위치
        float get_x() const { return x_; }
        float get_y() const { return y_; }
        float get_z() const { return z_; }
        void set_position(float x, float y, float z);

        // 맵 ID
        uint32_t get_map_id() const { return map_id_; }
        void set_map_id(uint32_t map_id) { map_id_ = map_id; }

        // NPC 타입
        enum class NPCType
        {
            MERCHANT,
            GUARD,
            QUEST_GIVER,
            TRAINER
        };

        NPCType get_type() const { return type_; }
        void set_type(NPCType type) { type_ = type; }

    private:
        uint32_t id_;
        std::string name_;
        float x_, y_, z_;
        uint32_t map_id_;
        NPCType type_;
    };

} // namespace bt
