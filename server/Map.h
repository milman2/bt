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
        uint32_t get_id() const { return id_; }
        const std::string& get_name() const { return name_; }
        void set_name(const std::string& name) { name_ = name; }

        // 맵 크기
        float get_width() const { return width_; }
        float get_height() const { return height_; }
        void set_size(float width, float height);

        // 맵 데이터
        const std::string& get_map_data() const { return map_data_; }
        void set_map_data(const std::string& data) { map_data_ = data; }

        // 위치 검증
        bool is_valid_position(float x, float y, float z) const;

    private:
        uint32_t id_;
        std::string name_;
        float width_;
        float height_;
        std::string map_data_;
    };

} // namespace bt
