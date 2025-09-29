#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace bt {

// 플레이어 상태 열거형
enum class PlayerState {
    OFFLINE,
    ONLINE,
    IN_GAME,
    IN_COMBAT,
    DEAD
};

// 플레이어 통계 구조체
struct PlayerStats {
    uint32_t level = 1;
    uint32_t experience = 0;
    uint32_t health = 100;
    uint32_t max_health = 100;
    uint32_t mana = 50;
    uint32_t max_mana = 50;
    uint32_t strength = 10;
    uint32_t agility = 10;
    uint32_t intelligence = 10;
    uint32_t vitality = 10;
};

// 플레이어 위치 구조체
struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
};

// 플레이어 클래스
class Player {
public:
    Player(uint32_t id, const std::string& name);
    ~Player();

    // 기본 정보
    uint32_t get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    PlayerState get_state() const { return state_; }
    void set_state(PlayerState state) { state_ = state; }

    // 위치 관련
    const Position& get_position() const { return position_; }
    void set_position(const Position& pos) { position_ = pos; }
    void set_position(float x, float y, float z, float rotation = 0.0f);
    void move_to(float x, float y, float z, float rotation = 0.0f);

    // 통계 관련
    const PlayerStats& get_stats() const { return stats_; }
    void set_stats(const PlayerStats& stats) { stats_ = stats; }
    void add_experience(uint32_t exp);
    void level_up();
    void take_damage(uint32_t damage);
    void heal(uint32_t amount);
    bool is_alive() const { return stats_.health > 0; }

    // 맵 관련
    uint32_t get_current_map_id() const { return current_map_id_; }
    void set_current_map_id(uint32_t map_id) { current_map_id_ = map_id; }

    // 소켓 관련
    int get_socket_fd() const { return socket_fd_; }
    void set_socket_fd(int fd) { socket_fd_ = fd; }

    // 시간 관련
    std::chrono::steady_clock::time_point get_last_activity() const { return last_activity_; }
    void update_activity() { last_activity_ = std::chrono::steady_clock::now(); }

    // 게임 로직
    void update(float delta_time);
    void respawn();

private:
    uint32_t id_;
    std::string name_;
    PlayerState state_;
    Position position_;
    PlayerStats stats_;
    uint32_t current_map_id_;
    int socket_fd_;
    std::chrono::steady_clock::time_point last_activity_;
    std::chrono::steady_clock::time_point last_update_time_;
};

} // namespace bt
