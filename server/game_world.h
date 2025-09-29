#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

namespace bt {

// 전방 선언
class Player;
class Map;
class NPC;

// 게임 월드 클래스
class GameWorld {
public:
    GameWorld();
    ~GameWorld();

    // 플레이어 관리
    bool add_player(uint32_t player_id, const std::string& name);
    void remove_player(uint32_t player_id);
    Player* get_player(uint32_t player_id);
    std::vector<Player*> get_players_in_map(uint32_t map_id);

    // 맵 관리
    bool load_map(uint32_t map_id, const std::string& map_data);
    Map* get_map(uint32_t map_id);
    std::vector<Map*> get_all_maps();

    // NPC 관리
    void spawn_npc(uint32_t npc_id, uint32_t map_id, float x, float y, float z);
    void remove_npc(uint32_t npc_id);
    NPC* get_npc(uint32_t npc_id);

    // 게임 루프
    void update(float delta_time);
    void start_game_loop();
    void stop_game_loop();

    // 이벤트 시스템
    void register_event_handler(std::function<void(const std::string&, const std::string&)> handler);
    void trigger_event(const std::string& event_type, const std::string& data);

private:
    void game_loop_thread();
    void process_ai(float delta_time);
    void process_movement(float delta_time);
    void process_combat(float delta_time);

private:
    std::atomic<bool> running_;
    std::thread game_loop_thread_;
    
    // 플레이어 관리
    std::unordered_map<uint32_t, std::unique_ptr<Player>> players_;
    std::mutex players_mutex_;
    
    // 맵 관리
    std::unordered_map<uint32_t, std::unique_ptr<Map>> maps_;
    std::mutex maps_mutex_;
    
    // NPC 관리
    std::unordered_map<uint32_t, std::unique_ptr<NPC>> npcs_;
    std::mutex npcs_mutex_;
    
    // 이벤트 시스템 (간단한 구현)
    std::vector<std::function<void(const std::string&, const std::string&)>> event_handlers_;
    std::mutex event_handlers_mutex_;
    
    // 게임 상태
    std::chrono::steady_clock::time_point last_update_time_;
    float target_fps_ = 60.0f;
};

} // namespace bt
