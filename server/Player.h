#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace bt
{

    // 플레이어 상태 열거형
    enum class PlayerStateType
    {
        OFFLINE,
        ONLINE,
        IN_GAME,
        IN_COMBAT,
        DEAD
    };

    // 플레이어 통계 구조체 (protobuf PlayerStats와 구분)
    struct PlayerGameStats
    {
        uint32_t level        = 1;
        uint32_t experience   = 0;
        uint32_t health       = 100;
        uint32_t max_health   = 100;
        uint32_t mana         = 50;
        uint32_t max_mana     = 50;
        uint32_t strength     = 10;
        uint32_t agility      = 10;
        uint32_t intelligence = 10;
        uint32_t vitality     = 10;
    };

    // 플레이어 위치 구조체
    struct Position
    {
        float x        = 0.0f;
        float y        = 0.0f;
        float z        = 0.0f;
        float rotation = 0.0f;
    };

    // 플레이어 클래스
    class Player
    {
    public:
        Player(uint32_t id, const std::string& name);
        ~Player();

        // 기본 정보
        uint32_t           GetID() const { return id_; }
        const std::string& GetName() const { return name_; }
        PlayerStateType    GetState() const { return state_; }
        void               SetState(PlayerStateType state) { state_ = state; }

        // 위치 관련
        const Position& GetPosition() const { return position_; }
        void            SetPosition(const Position& pos) { position_ = pos; }
        void            SetPosition(float x, float y, float z, float rotation = 0.0f);
        void            MoveTo(float x, float y, float z, float rotation = 0.0f);

        // 통계 관련
        const PlayerGameStats& GetStats() const { return stats_; }
        void               SetStats(const PlayerGameStats& stats) { stats_ = stats; }
        void               AddExperience(uint32_t exp);
        void               LevelUp();
        void               TakeDamage(uint32_t damage);
        void               Heal(uint32_t amount);
        bool               IsAlive() const { return stats_.health > 0; }

        // 맵 관련
        uint32_t GetCurrentMapID() const { return current_map_id_; }
        void     SetCurrentMapID(uint32_t map_id) { current_map_id_ = map_id; }

        // 소켓 관련
        int  GetSocketFD() const { return socket_fd_; }
        void SetSocketFD(int fd) { socket_fd_ = fd; }

        // 시간 관련
        std::chrono::steady_clock::time_point GetLastActivity() const { return last_activity_; }
        void                                  UpdateActivity() { last_activity_ = std::chrono::steady_clock::now(); }

        // 게임 로직
        void Update(float delta_time);
        void Respawn();

    private:
        uint32_t                              id_;
        std::string                           name_;
        PlayerStateType                       state_;
        Position                              position_;
        PlayerGameStats                       stats_;
        uint32_t                              current_map_id_;
        int                                   socket_fd_;
        std::chrono::steady_clock::time_point last_activity_;
        std::chrono::steady_clock::time_point last_update_time_;
    };

} // namespace bt
