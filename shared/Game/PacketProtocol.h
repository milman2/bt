#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace bt {

// 공통 패킷 구조체
struct Packet {
    uint32_t size;
    uint16_t type;
    std::vector<uint8_t> data;
    
    Packet() : size(0), type(0) {}
    Packet(uint16_t packet_type, const std::vector<uint8_t>& packet_data) 
        : size(packet_data.size() + sizeof(uint16_t)), type(packet_type), data(packet_data) {}
};

// 공통 패킷 타입 정의
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
    PLAYER_JOIN = 0x1000,
    PLAYER_JOIN_RESPONSE = 0x1001,
    PLAYER_MOVE = 0x2000,
    PLAYER_ATTACK = 0x2001,
    PLAYER_CHAT = 0x2002,
    PLAYER_STATS = 0x0203,
    
    // 몬스터 관련
    MONSTER_UPDATE = 0x3000,
    MONSTER_ACTION = 0x3001,
    MONSTER_DEATH = 0x3002,
    MONSTER_SPAWN = 0x3003,
    
    // Behavior Tree 관련
    BT_EXECUTE = 0x4000,
    BT_RESULT = 0x4001,
    BT_DEBUG = 0x4002,
    
    // 게임 월드 관련
    WORLD_UPDATE = 0x0300,
    WORLD_STATE_BROADCAST = 0x0301,  // 서버에서 클라이언트로 월드 상태 브로드캐스팅
    MAP_CHANGE = 0x0302,
    NPC_SPAWN = 0x0303,
    NPC_UPDATE = 0x0304,
    
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

// 공통 패킷 생성 유틸리티 함수들
namespace PacketUtils {
    // 연결 응답 패킷 생성
    Packet create_connect_response(bool success, const std::string& message);
    
    // 로그인 응답 패킷 생성
    Packet create_login_response(bool success, uint32_t player_id, const std::string& message);
    
    // 에러 메시지 패킷 생성
    Packet create_error_message(const std::string& error);
    
    // 채팅 메시지 패킷 생성
    Packet create_chat_message(const std::string& sender, const std::string& message);
    
    // 플레이어 이동 패킷 생성
    Packet create_player_move(uint32_t player_id, float x, float y, float z);
    
    // 플레이어 공격 패킷 생성
    Packet create_player_attack(uint32_t attacker_id, uint32_t target_id, uint32_t damage);
    
    // 몬스터 업데이트 패킷 생성
    Packet create_monster_update(uint32_t monster_id, float x, float y, float z, uint32_t health);
    
    // 월드 업데이트 패킷 생성
    Packet create_world_update(const std::vector<uint8_t>& world_data);
    
    // 패킷에서 데이터 읽기
    template<typename T>
    T read_from_packet(const Packet& packet, size_t offset);
    
    // 패킷에 데이터 쓰기
    template<typename T>
    void write_to_packet(Packet& packet, const T& value);
}

// 공통 클라이언트 설정 구조체
struct ClientConfig {
    std::string server_host = "127.0.0.1";
    uint16_t server_port = 8080;
    uint32_t connection_timeout_ms = 5000;
    bool auto_reconnect = false;
    int max_reconnect_attempts = 3;
    uint32_t max_packet_size = 4096;
};

// 공통 서버 설정 구조체
struct ServerConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    size_t max_clients = 1000;
    size_t worker_threads = 4;
    bool debug_mode = false;
    uint32_t max_packet_size = 4096;
    uint32_t connection_timeout_ms = 30000; // 30초
    uint32_t broadcast_fps = 10; // 브로드캐스팅 FPS
};

// 게임 월드 상태 구조체
struct WorldState {
    uint64_t timestamp; // 서버 시간
    uint32_t player_count;
    uint32_t monster_count;
    std::vector<uint8_t> serialized_data; // 직렬화된 월드 데이터
    
    WorldState() : timestamp(0), player_count(0), monster_count(0) {}
};

// 플레이어 상태 구조체
struct PlayerState {
    uint32_t id;
    std::string name;
    float x, y, z;
    float rotation;
    uint32_t health;
    uint32_t max_health;
    uint32_t level;
    
    PlayerState() : id(0), x(0), y(0), z(0), rotation(0), health(0), max_health(0), level(0) {}
};

// 몬스터 상태 구조체
struct MonsterState {
    uint32_t id;
    std::string name;
    float x, y, z;
    float rotation;
    uint32_t health;
    uint32_t max_health;
    uint32_t level;
    uint32_t type; // 몬스터 타입
    
    MonsterState() : id(0), x(0), y(0), z(0), rotation(0), health(0), max_health(0), level(0), type(0) {}
};

} // namespace bt
