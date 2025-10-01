#include "PacketProtocol.h"
#include <cstring>
#include <sstream>

namespace bt {

namespace PacketUtils {

Packet create_connect_response(bool success, const std::string& message) {
    std::vector<uint8_t> data;
    
    // 성공 여부 (1바이트)
    data.push_back(success ? 1 : 0);
    
    // 메시지 길이 (4바이트)
    uint32_t message_len = static_cast<uint32_t>(message.length());
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&message_len), 
                reinterpret_cast<const uint8_t*>(&message_len) + sizeof(message_len));
    
    // 메시지 내용
    data.insert(data.end(), message.begin(), message.end());
    
    return Packet(static_cast<uint16_t>(PacketType::CONNECT_RESPONSE), data);
}


Packet create_error_message(const std::string& error) {
    std::vector<uint8_t> data;
    
    // 에러 메시지 길이 (4바이트)
    uint32_t error_len = static_cast<uint32_t>(error.length());
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&error_len), 
                reinterpret_cast<const uint8_t*>(&error_len) + sizeof(error_len));
    
    // 에러 메시지 내용
    data.insert(data.end(), error.begin(), error.end());
    
    return Packet(static_cast<uint16_t>(PacketType::ERROR_MESSAGE), data);
}


Packet create_player_move(uint32_t player_id, float x, float y, float z) {
    std::vector<uint8_t> data;
    
    // 플레이어 ID (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&player_id), 
                reinterpret_cast<const uint8_t*>(&player_id) + sizeof(player_id));
    
    // X 좌표 (4바이트)
    uint32_t x_bits = *reinterpret_cast<const uint32_t*>(&x);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&x_bits), 
                reinterpret_cast<const uint8_t*>(&x_bits) + sizeof(x_bits));
    
    // Y 좌표 (4바이트)
    uint32_t y_bits = *reinterpret_cast<const uint32_t*>(&y);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&y_bits), 
                reinterpret_cast<const uint8_t*>(&y_bits) + sizeof(y_bits));
    
    // Z 좌표 (4바이트)
    uint32_t z_bits = *reinterpret_cast<const uint32_t*>(&z);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&z_bits), 
                reinterpret_cast<const uint8_t*>(&z_bits) + sizeof(z_bits));
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_MOVE), data);
}

Packet create_player_attack(uint32_t attacker_id, uint32_t target_id, uint32_t damage) {
    std::vector<uint8_t> data;
    
    // 공격자 ID (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&attacker_id), 
                reinterpret_cast<const uint8_t*>(&attacker_id) + sizeof(attacker_id));
    
    // 대상 ID (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&target_id), 
                reinterpret_cast<const uint8_t*>(&target_id) + sizeof(target_id));
    
    // 데미지 (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&damage), 
                reinterpret_cast<const uint8_t*>(&damage) + sizeof(damage));
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_ATTACK), data);
}

Packet create_monster_update(uint32_t monster_id, float x, float y, float z, uint32_t health) {
    std::vector<uint8_t> data;
    
    // 몬스터 ID (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&monster_id), 
                reinterpret_cast<const uint8_t*>(&monster_id) + sizeof(monster_id));
    
    // X 좌표 (4바이트)
    uint32_t x_bits = *reinterpret_cast<const uint32_t*>(&x);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&x_bits), 
                reinterpret_cast<const uint8_t*>(&x_bits) + sizeof(x_bits));
    
    // Y 좌표 (4바이트)
    uint32_t y_bits = *reinterpret_cast<const uint32_t*>(&y);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&y_bits), 
                reinterpret_cast<const uint8_t*>(&y_bits) + sizeof(y_bits));
    
    // Z 좌표 (4바이트)
    uint32_t z_bits = *reinterpret_cast<const uint32_t*>(&z);
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&z_bits), 
                reinterpret_cast<const uint8_t*>(&z_bits) + sizeof(z_bits));
    
    // 체력 (4바이트)
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&health), 
                reinterpret_cast<const uint8_t*>(&health) + sizeof(health));
    
    return Packet(static_cast<uint16_t>(PacketType::MONSTER_UPDATE), data);
}


template<typename T>
T read_from_packet(const Packet& packet, size_t offset) {
    if (offset + sizeof(T) > packet.data.size()) {
        throw std::runtime_error("Packet read out of bounds");
    }
    
    T value;
    std::memcpy(&value, packet.data.data() + offset, sizeof(T));
    return value;
}

template<typename T>
void write_to_packet(Packet& packet, const T& value) {
    const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
    packet.data.insert(packet.data.end(), value_ptr, value_ptr + sizeof(T));
}

// 템플릿 특수화 선언
template uint32_t read_from_packet<uint32_t>(const Packet& packet, size_t offset);
template uint16_t read_from_packet<uint16_t>(const Packet& packet, size_t offset);
template uint8_t read_from_packet<uint8_t>(const Packet& packet, size_t offset);
template float read_from_packet<float>(const Packet& packet, size_t offset);

template void write_to_packet<uint32_t>(Packet& packet, const uint32_t& value);
template void write_to_packet<uint16_t>(Packet& packet, const uint16_t& value);
template void write_to_packet<uint8_t>(Packet& packet, const uint8_t& value);
template void write_to_packet<float>(Packet& packet, const float& value);

} // namespace PacketUtils

} // namespace bt
