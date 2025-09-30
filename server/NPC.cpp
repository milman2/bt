#include "NPC.h"

#include <iostream>

namespace bt
{

    NPC::NPC(uint32_t id) : id_(id), x_(0.0f), y_(0.0f), z_(0.0f), map_id_(0), type_(NPCType::MERCHANT)
    {
        name_ = "NPC_" + std::to_string(id);
        std::cout << "NPC 생성: " << name_ << " (ID: " << id_ << ")" << std::endl;
    }

    NPC::~NPC()
    {
        std::cout << "NPC 소멸: " << name_ << " (ID: " << id_ << ")" << std::endl;
    }

    void NPC::set_position(float x, float y, float z)
    {
        x_ = x;
        y_ = y;
        z_ = z;
        std::cout << "NPC 위치 설정: " << name_ << " (" << x_ << ", " << y_ << ", " << z_ << ")" << std::endl;
    }

} // namespace bt
