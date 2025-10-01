#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>

namespace bt
{

    // 클라이언트 메시지 타입
    enum class ClientMessageType
    {
        NETWORK_PACKET_RECEIVED,
        NETWORK_CONNECTION_LOST,
        NETWORK_CONNECTION_ESTABLISHED,
        AI_UPDATE_REQUEST,
        AI_STATE_CHANGE,
        GAME_STATE_UPDATE,
        PLAYER_ACTION_REQUEST,
        MONSTER_UPDATE,
        COMBAT_RESULT,
        SYSTEM_SHUTDOWN
    };

    // 기본 클라이언트 메시지 클래스
    class ClientMessage
    {
    public:
        ClientMessage(ClientMessageType type) : type_(type), timestamp_(std::chrono::steady_clock::now()) {}
        virtual ~ClientMessage() = default;

        ClientMessageType GetType() const { return type_; }
        std::chrono::steady_clock::time_point GetTimestamp() const { return timestamp_; }

    private:
        ClientMessageType type_;
        std::chrono::steady_clock::time_point timestamp_;
    };

    // 네트워크 패킷 메시지
    class NetworkPacketMessage : public ClientMessage
    {
    public:
        NetworkPacketMessage(const std::vector<uint8_t>& data, uint16_t packet_type)
            : ClientMessage(ClientMessageType::NETWORK_PACKET_RECEIVED), data_(data), packet_type_(packet_type) {}

        const std::vector<uint8_t>& GetData() const { return data_; }
        uint16_t GetPacketType() const { return packet_type_; }

    private:
        std::vector<uint8_t> data_;
        uint16_t packet_type_;
    };

    // AI 업데이트 요청 메시지
    class AIUpdateMessage : public ClientMessage
    {
    public:
        AIUpdateMessage(float delta_time) : ClientMessage(ClientMessageType::AI_UPDATE_REQUEST), delta_time_(delta_time) {}
        float GetDeltaTime() const { return delta_time_; }

    private:
        float delta_time_;
    };

    // AI 상태 변경 메시지
    class AIStateChangeMessage : public ClientMessage
    {
    public:
        AIStateChangeMessage(bool active) : ClientMessage(ClientMessageType::AI_STATE_CHANGE), active_(active) {}
        bool IsActive() const { return active_; }

    private:
        bool active_;
    };

    // 플레이어 액션 요청 메시지
    class PlayerActionMessage : public ClientMessage
    {
    public:
        enum class ActionType
        {
            MOVE,
            ATTACK,
            RESPAWN
        };

        PlayerActionMessage(ActionType action, float x = 0, float y = 0, float z = 0, uint32_t target_id = 0)
            : ClientMessage(ClientMessageType::PLAYER_ACTION_REQUEST), action_(action), x_(x), y_(y), z_(z), target_id_(target_id) {}

        ActionType GetAction() const { return action_; }
        float GetX() const { return x_; }
        float GetY() const { return y_; }
        float GetZ() const { return z_; }
        uint32_t GetTargetId() const { return target_id_; }

    private:
        ActionType action_;
        float x_, y_, z_;
        uint32_t target_id_;
    };

    // 몬스터 업데이트 메시지
    class MonsterUpdateMessage : public ClientMessage
    {
    public:
        MonsterUpdateMessage(const std::unordered_map<uint32_t, std::tuple<float, float, float, float>>& monsters)
            : ClientMessage(ClientMessageType::MONSTER_UPDATE), monsters_(monsters) {}

        const std::unordered_map<uint32_t, std::tuple<float, float, float, float>>& GetMonsters() const { return monsters_; }

    private:
        std::unordered_map<uint32_t, std::tuple<float, float, float, float>> monsters_;
    };

    // 전투 결과 메시지
    class CombatResultMessage : public ClientMessage
    {
    public:
        CombatResultMessage(uint32_t attacker_id, uint32_t target_id, uint32_t damage, uint32_t remaining_health)
            : ClientMessage(ClientMessageType::COMBAT_RESULT), attacker_id_(attacker_id), target_id_(target_id), 
              damage_(damage), remaining_health_(remaining_health) {}

        uint32_t GetAttackerId() const { return attacker_id_; }
        uint32_t GetTargetId() const { return target_id_; }
        uint32_t GetDamage() const { return damage_; }
        uint32_t GetRemainingHealth() const { return remaining_health_; }

    private:
        uint32_t attacker_id_;
        uint32_t target_id_;
        uint32_t damage_;
        uint32_t remaining_health_;
    };

    // 클라이언트 메시지 핸들러 인터페이스
    class IClientMessageHandler
    {
    public:
        virtual ~IClientMessageHandler() = default;
        virtual void HandleMessage(std::shared_ptr<ClientMessage> message) = 0;
    };

    // 클라이언트 메시지 프로세서
    class ClientMessageProcessor
    {
    public:
        ClientMessageProcessor();
        ~ClientMessageProcessor();

        // 메시지 프로세서 시작/중지
        void Start();
        void Stop();

        // 메시지 전송
        void SendMessage(std::shared_ptr<ClientMessage> message);

        // 핸들러 등록
        void RegisterHandler(ClientMessageType type, std::shared_ptr<IClientMessageHandler> handler);

        // 상태 확인
        bool IsRunning() const { return running_.load(); }
        size_t GetQueueSize() const;

    private:
        void ProcessMessages();
        void HandleMessage(std::shared_ptr<ClientMessage> message);

        std::atomic<bool> running_;
        std::queue<std::shared_ptr<ClientMessage>> message_queue_;
        mutable std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        std::thread processor_thread_;

        std::unordered_map<ClientMessageType, std::shared_ptr<IClientMessageHandler>> handlers_;
        mutable std::mutex handlers_mutex_;
    };

} // namespace bt
