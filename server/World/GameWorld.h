#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace bt
{

    // 전방 선언
    class Player;
    class Map;
    class NPC;

    // 게임 월드 클래스
    class GameWorld
    {
    public:
        GameWorld();
        ~GameWorld();

        // 플레이어 관리
        bool                 AddPlayer(uint32_t player_id, const std::string& name);
        void                 RemovePlayer(uint32_t player_id);
        Player*              GetPlayer(uint32_t player_id);
        std::vector<Player*> GetPlayersInMap(uint32_t map_id);

        // 맵 관리
        bool              LoadMap(uint32_t map_id, const std::string& map_data);
        Map*              GetMap(uint32_t map_id);
        std::vector<Map*> GetAllMaps();

        // NPC 관리
        void SpawnNPC(uint32_t npc_id, uint32_t map_id, float x, float y, float z);
        void RemoveNPC(uint32_t npc_id);
        NPC* GetNPC(uint32_t npc_id);

        // 게임 루프
        void Update(float delta_time);
        void StartGameLoop();
        void StopGameLoop();

        // 이벤트 시스템
        void RegisterEventHandler(std::function<void(const std::string&, const std::string&)> handler);
        void TriggerEvent(const std::string& event_type, const std::string& data);

    private:
        void GameLoopThread();
        void ProcessAI(float delta_time);
        void ProcessMovement(float delta_time);
        void ProcessCombat(float delta_time);

    private:
        std::atomic<bool> running_;
        std::thread       game_loop_thread_;

        // 플레이어 관리
        std::unordered_map<uint32_t, std::unique_ptr<Player>> players_;
        std::mutex                                            players_mutex_;

        // 맵 관리
        std::unordered_map<uint32_t, std::unique_ptr<Map>> maps_;
        std::mutex                                         maps_mutex_;

        // NPC 관리
        std::unordered_map<uint32_t, std::unique_ptr<NPC>> npcs_;
        std::mutex                                         npcs_mutex_;

        // 이벤트 시스템 (간단한 구현)
        std::vector<std::function<void(const std::string&, const std::string&)>> event_handlers_;
        std::mutex                                                               event_handlers_mutex_;

        // 게임 상태
        std::chrono::steady_clock::time_point last_update_time_;
        float                                 target_fps_ = 60.0f;
    };

} // namespace bt
