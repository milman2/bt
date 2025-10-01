#pragma once

#include <string>
#include <vector>

#include <any>
#include <nlohmann/json.hpp>

#include "MessageQueue.h"

namespace bt
{

    // 기본 게임 메시지
    class GameMessage : public IMessage
    {
    protected:
        MessageType type_;
        std::string source_;
        uint64_t    timestamp_;

    public:
        GameMessage(MessageType type, const std::string& source = "")
            : type_(type)
            , source_(source)
            , timestamp_(std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now().time_since_epoch())
                             .count())
        {
        }

        MessageType        GetType() const override { return type_; }
        const std::string& GetSource() const { return source_; }
        uint64_t           GetTimestamp() const { return timestamp_; }

        virtual nlohmann::json ToJson() const
        {
            return {
                {"type",      static_cast<int>(type_)},
                {"source",    source_                },
                {"timestamp", timestamp_             }
            };
        }

        std::string ToString() const override { return ToJson().dump(); }
    };

    // 몬스터 관련 메시지들
    class MonsterSpawnMessage : public GameMessage
    {
    public:
        uint32_t    monster_id;
        std::string monster_name;
        std::string monster_type;
        float       x, y, z, rotation;
        float       health, max_health;

        MonsterSpawnMessage(uint32_t           id,
                            const std::string& name,
                            const std::string& type,
                            float              x,
                            float              y,
                            float              z,
                            float              rotation,
                            float              health,
                            float              max_health)
            : GameMessage(MessageType::MONSTER_SPAWN, "MonsterManager")
            , monster_id(id)
            , monster_name(name)
            , monster_type(type)
            , x(x)
            , y(y)
            , z(z)
            , rotation(rotation)
            , health(health)
            , max_health(max_health)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json            = GameMessage::ToJson();
            json["monster_id"]   = monster_id;
            json["monster_name"] = monster_name;
            json["monster_type"] = monster_type;
            json["position"]     = {
                {"x",        x       },
                {"y",        y       },
                {"z",        z       },
                {"rotation", rotation}
            };
            json["health"]     = health;
            json["max_health"] = max_health;
            return json;
        }
    };

    class MonsterMoveMessage : public GameMessage
    {
    public:
        uint32_t monster_id;
        float    from_x, from_y, from_z;
        float    to_x, to_y, to_z;

        MonsterMoveMessage(uint32_t id, float from_x, float from_y, float from_z, float to_x, float to_y, float to_z)
            : GameMessage(MessageType::MONSTER_MOVE, "MonsterManager")
            , monster_id(id)
            , from_x(from_x)
            , from_y(from_y)
            , from_z(from_z)
            , to_x(to_x)
            , to_y(to_y)
            , to_z(to_z)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json          = GameMessage::ToJson();
            json["monster_id"] = monster_id;
            json["from"]       = {
                {"x", from_x},
                {"y", from_y},
                {"z", from_z}
            };
            json["to"] = {
                {"x", to_x},
                {"y", to_y},
                {"z", to_z}
            };
            return json;
        }
    };

    class MonsterDeathMessage : public GameMessage
    {
    public:
        uint32_t    monster_id;
        std::string monster_name;
        float       x, y, z;

        MonsterDeathMessage(uint32_t id, const std::string& name, float x, float y, float z)
            : GameMessage(MessageType::MONSTER_DEATH, "MonsterManager")
            , monster_id(id)
            , monster_name(name)
            , x(x)
            , y(y)
            , z(z)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json            = GameMessage::ToJson();
            json["monster_id"]   = monster_id;
            json["monster_name"] = monster_name;
            json["position"]     = {
                {"x", x},
                {"y", y},
                {"z", z}
            };
            return json;
        }
    };

    // 플레이어 관련 메시지들
    class PlayerJoinMessage : public GameMessage
    {
    public:
        uint32_t    player_id;
        std::string player_name;
        float       x, y, z, rotation;

        PlayerJoinMessage(uint32_t id, const std::string& name, float x, float y, float z, float rotation)
            : GameMessage(MessageType::PLAYER_JOIN, "PlayerManager")
            , player_id(id)
            , player_name(name)
            , x(x)
            , y(y)
            , z(z)
            , rotation(rotation)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json           = GameMessage::ToJson();
            json["player_id"]   = player_id;
            json["player_name"] = player_name;
            json["position"]    = {
                {"x",        x       },
                {"y",        y       },
                {"z",        z       },
                {"rotation", rotation}
            };
            return json;
        }
    };

    class PlayerMoveMessage : public GameMessage
    {
    public:
        uint32_t player_id;
        float    from_x, from_y, from_z;
        float    to_x, to_y, to_z;

        PlayerMoveMessage(uint32_t id, float from_x, float from_y, float from_z, float to_x, float to_y, float to_z)
            : GameMessage(MessageType::PLAYER_MOVE, "PlayerManager")
            , player_id(id)
            , from_x(from_x)
            , from_y(from_y)
            , from_z(from_z)
            , to_x(to_x)
            , to_y(to_y)
            , to_z(to_z)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json         = GameMessage::ToJson();
            json["player_id"] = player_id;
            json["from"]      = {
                {"x", from_x},
                {"y", from_y},
                {"z", from_z}
            };
            json["to"] = {
                {"x", to_x},
                {"y", to_y},
                {"z", to_z}
            };
            return json;
        }
    };

    // 게임 상태 업데이트 메시지
    class GameStateUpdateMessage : public GameMessage
    {
    public:
        nlohmann::json game_state;

        GameStateUpdateMessage(const nlohmann::json& state)
            : GameMessage(MessageType::GAME_STATE_UPDATE, "GameEngine"), game_state(state)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json          = GameMessage::ToJson();
            json["game_state"] = game_state;
            return json;
        }
    };

    // 네트워크 브로드캐스트 메시지
    class NetworkBroadcastMessage : public GameMessage
    {
    public:
        std::string    event_type;
        nlohmann::json data;

        NetworkBroadcastMessage(const std::string& event_type, const nlohmann::json& data)
            : GameMessage(MessageType::NETWORK_BROADCAST, "NetworkManager"), event_type(event_type), data(data)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json          = GameMessage::ToJson();
            json["event_type"] = event_type;
            json["data"]       = data;
            return json;
        }
    };

    // 시스템 셧다운 메시지
    class SystemShutdownMessage : public GameMessage
    {
    public:
        std::string reason;

        SystemShutdownMessage(const std::string& reason = "")
            : GameMessage(MessageType::SYSTEM_SHUTDOWN, "System"), reason(reason)
        {
        }

        nlohmann::json ToJson() const override
        {
            auto json      = GameMessage::ToJson();
            json["reason"] = reason;
            return json;
        }
    };

} // namespace bt
