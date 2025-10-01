#include <sstream>

#include <cstring>

#include "PacketProtocol.h"
#include "generated/packets.pb.h"

namespace bt
{

    namespace PacketUtils
    {

        // CONNECT_REQUEST 패킷 생성 (protobuf 기반)
        Packet create_connect_request(const std::string& client_name, uint32_t version)
        {
            bt::ConnectReq request;
            request.set_client_name(client_name);
            request.set_version(version);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::CONNECT_REQ), request);
        }

        // CONNECT_RESPONSE 패킷 생성 (protobuf 기반)
        Packet create_connect_response(bool success, const std::string& message, uint32_t client_id)
        {
            bt::ConnectRes response;
            response.set_success(success);
            response.set_message(message);
            response.set_client_id(client_id);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::CONNECT_RES), response);
        }

        // ERROR_MESSAGE 패킷 생성 (protobuf 기반)
        Packet create_error_message(const std::string& error, uint32_t error_code)
        {
            bt::ErrorMessageEvt error_msg;
            error_msg.set_error(error);
            error_msg.set_error_code(error_code);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::ERROR_MESSAGE_EVT), error_msg);
        }

        // PLAYER_MOVE 패킷 생성 (protobuf 기반)
        Packet create_player_move(uint32_t player_id, float x, float y, float z, float rotation)
        {
            bt::PlayerMoveReq move;
            move.set_player_id(player_id);
            move.set_x(x);
            move.set_y(y);
            move.set_z(z);
            move.set_rotation(rotation);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::PLAYER_MOVE_REQ), move);
        }

        // PLAYER_ATTACK 패킷 생성 (protobuf 기반)
        Packet create_player_attack(uint32_t attacker_id, uint32_t target_id, uint32_t damage)
        {
            bt::PlayerAttackReq attack;
            attack.set_attacker_id(attacker_id);
            attack.set_target_id(target_id);
            attack.set_damage(damage);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::PLAYER_ATTACK_REQ), attack);
        }

        // MONSTER_UPDATE 패킷 생성 (protobuf 기반)
        Packet create_monster_update(uint32_t           monster_id,
                                     const std::string& name,
                                     float              x,
                                     float              y,
                                     float              z,
                                     float              rotation,
                                     uint32_t           health,
                                     uint32_t           max_health,
                                     uint32_t           level,
                                     uint32_t           type)
        {
            bt::MonsterUpdateEvt update;
            update.set_monster_id(monster_id);
            update.set_name(name);
            update.set_x(x);
            update.set_y(y);
            update.set_z(z);
            update.set_rotation(rotation);
            update.set_health(health);
            update.set_max_health(max_health);
            update.set_level(level);
            update.set_type(type);

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::MONSTER_UPDATE_EVT), update);
        }

        // BT_EXECUTE 패킷 생성 (protobuf 기반)
        Packet create_bt_execute(uint32_t                                  monster_id,
                                 const std::string&                        bt_name,
                                 const std::map<std::string, std::string>& parameters)
        {
            bt::BTExecuteReq execute;
            execute.set_monster_id(monster_id);
            execute.set_bt_name(bt_name);

            // 파라미터 추가
            for (const auto& param : parameters)
            {
                execute.mutable_parameters()->insert({param.first, param.second});
            }

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::BT_EXECUTE_REQ), execute);
        }

        // BT_RESULT 패킷 생성 (protobuf 기반)
        Packet create_bt_result(uint32_t                                  monster_id,
                                const std::string&                        bt_name,
                                bool                                      success,
                                const std::string&                        result_message,
                                const std::map<std::string, std::string>& state_changes)
        {
            bt::BTResultEvt result;
            result.set_monster_id(monster_id);
            result.set_bt_name(bt_name);
            result.set_success(success);
            result.set_result_message(result_message);

            // 상태 변화 추가
            for (const auto& change : state_changes)
            {
                result.mutable_state_changes()->insert({change.first, change.second});
            }

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::BT_RESULT_EVT), result);
        }

        // WORLD_STATE_BROADCAST 패킷 생성 (protobuf 기반)
        Packet create_world_state_broadcast(uint64_t                             timestamp,
                                            uint32_t                             player_count,
                                            uint32_t                             monster_count,
                                            const std::vector<bt::PlayerState>&  players,
                                            const std::vector<bt::MonsterState>& monsters)
        {
            bt::WorldStateBroadcastEvt broadcast;
            broadcast.set_timestamp(timestamp);
            broadcast.set_player_count(player_count);
            broadcast.set_monster_count(monster_count);

            // 플레이어 상태 추가
            for (const auto& player : players)
            {
                *broadcast.add_players() = player;
            }

            // 몬스터 상태 추가
            for (const auto& monster : monsters)
            {
                *broadcast.add_monsters() = monster;
            }

            return Packet::FromProtobuf(static_cast<uint16_t>(PacketType::WORLD_STATE_BROADCAST_EVT), broadcast);
        }

        template <typename T>
        T read_from_packet(const Packet& packet, size_t offset)
        {
            if (offset + sizeof(T) > packet.data.size())
            {
        throw std::runtime_error("Packet read out of bounds");
    }
    
    T value;
    std::memcpy(&value, packet.data.data() + offset, sizeof(T));
    return value;
}

        template <typename T>
        void write_to_packet(Packet& packet, const T& value)
        {
    const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
    packet.data.insert(packet.data.end(), value_ptr, value_ptr + sizeof(T));
}

// 템플릿 특수화 선언
template uint32_t read_from_packet<uint32_t>(const Packet& packet, size_t offset);
template uint16_t read_from_packet<uint16_t>(const Packet& packet, size_t offset);
        template uint8_t  read_from_packet<uint8_t>(const Packet& packet, size_t offset);
        template float    read_from_packet<float>(const Packet& packet, size_t offset);

template void write_to_packet<uint32_t>(Packet& packet, const uint32_t& value);
template void write_to_packet<uint16_t>(Packet& packet, const uint16_t& value);
template void write_to_packet<uint8_t>(Packet& packet, const uint8_t& value);
template void write_to_packet<float>(Packet& packet, const float& value);

} // namespace PacketUtils

} // namespace bt
