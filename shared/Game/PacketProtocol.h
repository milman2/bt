#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <cstdint>

#include "generated/packets.pb.h"

namespace bt
{

    // 공통 패킷 구조체 (protobuf 기반)
    struct Packet
    {
        uint32_t             size;
        uint16_t             type;
        std::vector<uint8_t> data;

        Packet() : size(0), type(0) {}
        Packet(uint16_t packet_type, const std::vector<uint8_t>& packet_data)
            : size(packet_data.size() + sizeof(uint16_t)), type(packet_type), data(packet_data)
        {
        }

        // protobuf 메시지로부터 패킷 생성
        template <typename T>
        static Packet FromProtobuf(uint16_t packet_type, const T& message)
        {
            std::string serialized;
            message.SerializeToString(&serialized);
            std::vector<uint8_t> data(serialized.begin(), serialized.end());
            return Packet(packet_type, data);
        }

        // protobuf 메시지로 역직렬화
        template <typename T>
        bool ToProtobuf(T& message) const
        {
            std::string serialized(data.begin(), data.end());
            return message.ParseFromString(serialized);
        }
    };

    // 공통 패킷 타입 정의
    enum class PacketType : uint16_t
    {
        // 연결 관련
        CONNECT_REQ     = 0x0001, // 클라이언트 → 서버: 연결 요청
        CONNECT_RES     = 0x0002, // 서버 → 클라이언트: 연결 응답
        DISCONNECT_EVT  = 0x0003, // 서버 → 클라이언트: 연결 종료 알림

        // 플레이어 관련
        PLAYER_JOIN_REQ = 0x1000, // 클라이언트 → 서버: 게임 참여 요청
        PLAYER_JOIN_RES = 0x1001, // 서버 → 클라이언트: 게임 참여 응답
        PLAYER_MOVE_REQ = 0x2000, // 클라이언트 → 서버: 플레이어 이동 요청
        PLAYER_ATTACK_REQ = 0x2001, // 클라이언트 → 서버: 플레이어 공격 요청
        PLAYER_STATS_EVT = 0x0203, // 서버 → 클라이언트: 플레이어 상태 업데이트

        // 몬스터 관련
        MONSTER_UPDATE_EVT = 0x3000, // 서버 → 클라이언트: 몬스터 상태 업데이트

        // Behavior Tree 관련
        BT_EXECUTE_REQ = 0x4000, // 서버 → 클라이언트: BT 실행 요청
        BT_RESULT_EVT  = 0x4001, // 클라이언트 → 서버: BT 실행 결과

        // 게임 월드 관련
        WORLD_STATE_BROADCAST_EVT = 0x0301, // 서버 → 클라이언트: 월드 상태 브로드캐스팅

        // 에러 관련
        ERROR_MESSAGE_EVT = 0xFF00 // 서버 → 클라이언트: 오류 메시지
    };

    // 공통 패킷 생성 유틸리티 함수들
    namespace PacketUtils
    {
        // 연결 요청 패킷 생성 (protobuf 기반)
        Packet create_connect_request(const std::string& client_name, uint32_t version = 1);

        // 연결 응답 패킷 생성 (protobuf 기반)
        Packet create_connect_response(bool success, const std::string& message, uint32_t client_id = 0);

        // 로그인 응답 패킷 생성
        Packet create_login_response(bool success, uint32_t player_id, const std::string& message);

        // 에러 메시지 패킷 생성 (protobuf 기반)
        Packet create_error_message(const std::string& error, uint32_t error_code = 0);

        // 채팅 메시지 패킷 생성
        Packet create_chat_message(const std::string& sender, const std::string& message);

        // 플레이어 이동 패킷 생성 (protobuf 기반)
        Packet create_player_move(uint32_t player_id, float x, float y, float z, float rotation = 0.0f);

        // 플레이어 공격 패킷 생성 (protobuf 기반)
        Packet create_player_attack(uint32_t attacker_id, uint32_t target_id, uint32_t damage);

        // 몬스터 업데이트 패킷 생성 (protobuf 기반)
        Packet create_monster_update(uint32_t           monster_id,
                                     const std::string& name,
                                     float              x,
                                     float              y,
                                     float              z,
                                     float              rotation,
                                     uint32_t           health,
                                     uint32_t           max_health,
                                     uint32_t           level,
                                     uint32_t           type);

        // 월드 업데이트 패킷 생성
        Packet create_world_update(const std::vector<uint8_t>& world_data);

        // WORLD_STATE_BROADCAST 패킷 생성 (protobuf 기반)
        Packet create_world_state_broadcast(uint64_t                             timestamp,
                                            uint32_t                             player_count,
                                            uint32_t                             monster_count,
                                            const std::vector<bt::PlayerState>&  players,
                                            const std::vector<bt::MonsterState>& monsters);

        // BT_EXECUTE 패킷 생성 (protobuf 기반)
        Packet create_bt_execute(uint32_t                                  monster_id,
                                 const std::string&                        bt_name,
                                 const std::map<std::string, std::string>& parameters = {});

        // BT_RESULT 패킷 생성 (protobuf 기반)
        Packet create_bt_result(uint32_t                                  monster_id,
                                const std::string&                        bt_name,
                                bool                                      success,
                                const std::string&                        result_message = "",
                                const std::map<std::string, std::string>& state_changes  = {});

        // 패킷에서 데이터 읽기
        template <typename T>
        T read_from_packet(const Packet& packet, size_t offset);

        // 패킷에 데이터 쓰기
        template <typename T>
        void write_to_packet(Packet& packet, const T& value);
    } // namespace PacketUtils

    // 공통 클라이언트 설정 구조체
    struct ClientConfig
    {
        std::string server_host            = "127.0.0.1";
        uint16_t    server_port            = 8080;
        uint32_t    connection_timeout_ms  = 5000;
        bool        auto_reconnect         = false;
        int         max_reconnect_attempts = 3;
        uint32_t    max_packet_size        = 4096;
    };

    // 공통 서버 설정 구조체
    struct ServerConfig
    {
        std::string host                  = "0.0.0.0";
        uint16_t    port                  = 8080;
        size_t      max_clients           = 1000;
        size_t      worker_threads        = 4;
        bool        debug_mode            = false;
        uint32_t    max_packet_size       = 4096;
        uint32_t    connection_timeout_ms = 30000; // 30초
        uint32_t    broadcast_fps         = 10;    // 브로드캐스팅 FPS
    };

    // protobuf 메시지를 사용하므로 기존 구조체들은 제거됨
    // 대신 packets.pb.h의 bt::PlayerState, bt::MonsterState, bt::WorldStateBroadcast 사용

} // namespace bt
