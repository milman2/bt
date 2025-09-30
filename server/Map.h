#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bt
{

    // 맵 클래스
    class Map
    {
    public:
        Map(uint32_t id);
        ~Map();

        // 기본 정보
        uint32_t GetID() const { return id_; }
        const std::string& GetName() const { return name_; }
        void SetName(const std::string& name) { name_ = name; }

        // 맵 크기
        float GetWidth() const { return width_; }
        float GetHeight() const { return height_; }
        void SetSize(float width, float height);

        // 맵 데이터
        const std::string& GetMapData() const { return map_data_; }
        void SetMapData(const std::string& data) { map_data_ = data; }

        // 위치 검증
        bool IsValidPosition(float x, float y, float z) const;

    private:
        uint32_t id_;
        std::string name_;
        float width_;
        float height_;
        std::string map_data_;
    };

} // namespace bt
