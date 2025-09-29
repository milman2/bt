#pragma once

#include "server.h"
#include <functional>
#include <unordered_map>

namespace bt {

// 패킷 타입 정의
enum class PacketType : uint16_t {
    // 연결 관련
    CONNECT_REQUEST = 0x0001,
    CONNECT_RESPONSE = 0x0002,
    DISCONNECT = 0x0003,
    
    // 인증 관련
    LOGIN_REQUEST = 0x0100,
    LOGIN_RESPONSE = 0x0101,
    LOGOUT_REQUEST = 0x0102,
    
    // 플레이어 관련
    PLAYER_MOVE = 0x0200,
    PLAYER_ATTACK = 0x0201,
    PLAYER_CHAT = 0x0202,
    PLAYER_STATS = 0x0203,
    
    // 게임 월드 관련
    WORLD_UPDATE = 0x0300,
    MAP_CHANGE = 0x0301,
    NPC_SPAWN = 0x0302,
    NPC_UPDATE = 0x0303,
    
    // 아이템 관련
    ITEM_PICKUP = 0x0400,
    ITEM_DROP = 0x0401,
    INVENTORY_UPDATE = 0x0402,
    
    // 채팅 관련
    CHAT_MESSAGE = 0x0500,
    WHISPER_MESSAGE = 0x0501,
    
    // 에러 관련
    ERROR_MESSAGE = 0xFF00
};

// 패킷 핸들러 함수 타입
using PacketHandlerFunc = std::function<void(int socket_fd, const Packet& packet)>;

// 패킷 핸들러 클래스
class PacketHandler {
public:
    PacketHandler();
    ~PacketHandler();

    // 패킷 처리
    void handle_packet(int socket_fd, const Packet& packet);
    
    // 핸들러 등록
    void register_handler(PacketType type, PacketHandlerFunc handler);
    
    // 패킷 생성 유틸리티
    static Packet create_connect_response(bool success, const std::string& message);
    static Packet create_login_response(bool success, uint32_t player_id, const std::string& message);
    static Packet create_error_message(const std::string& error);
    static Packet create_chat_message(const std::string& sender, const std::string& message);
    static Packet create_player_move(uint32_t player_id, float x, float y, float z);
    static Packet create_world_update(const std::vector<uint8_t>& world_data);

private:
    // 기본 핸들러들
    void handle_connect_request(int socket_fd, const Packet& packet);
    void handle_login_request(int socket_fd, const Packet& packet);
    void handle_logout_request(int socket_fd, const Packet& packet);
    void handle_player_move(int socket_fd, const Packet& packet);
    void handle_player_chat(int socket_fd, const Packet& packet);
    void handle_disconnect(int socket_fd, const Packet& packet);
    
    // 패킷 파싱 유틸리티
    template<typename T>
    T read_from_packet(const Packet& packet, size_t offset) const;
    
    template<typename T>
    void write_to_packet(Packet& packet, const T& value) const;

private:
    std::unordered_map<PacketType, PacketHandlerFunc> handlers_;
};

} // namespace bt
