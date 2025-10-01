#include <iostream>

#include "Map.h"

namespace bt
{

    Map::Map(uint32_t id) : id_(id), width_(1000.0f), height_(1000.0f)
    {
        name_ = "Map_" + std::to_string(id);
        std::cout << "맵 생성: " << name_ << " (ID: " << id_ << ")" << std::endl;
    }

    Map::~Map()
    {
        std::cout << "맵 소멸: " << name_ << " (ID: " << id_ << ")" << std::endl;
    }

    void Map::SetSize(float width, float height)
    {
        width_  = width;
        height_ = height;
        std::cout << "맵 크기 설정: " << name_ << " (" << width_ << " x " << height_ << ")" << std::endl;
    }

    bool Map::IsValidPosition(float x, float y, float z) const
    {
        return x >= 0.0f && x <= width_ && z >= 0.0f && z <= height_;
    }

} // namespace bt
