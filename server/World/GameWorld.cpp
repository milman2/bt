#include <chrono>
#include <iostream>
#include <thread>

#include "GameWorld.h"
#include "Map.h"
#include "BT/NPC/NPC.h"
#include "Player.h"

namespace bt
{

    // Map과 NPC 클래스는 이제 별도 파일에 정의됨

    GameWorld::GameWorld() : running_(false)
    {
        std::cout << "게임 월드 초기화 중..." << std::endl;
    }

    GameWorld::~GameWorld()
    {
        StopGameLoop();
        std::cout << "게임 월드 소멸" << std::endl;
    }

    bool GameWorld::AddPlayer(uint32_t player_id, const std::string& name)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        if (players_.find(player_id) != players_.end())
        {
            return false; // 플레이어가 이미 존재함
        }

        players_[player_id] = std::make_unique<Player>(player_id, name);
        std::cout << "플레이어 추가: " << name << " (ID: " << player_id << ")" << std::endl;
        return true;
    }

    void GameWorld::RemovePlayer(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto it = players_.find(player_id);
        if (it != players_.end())
        {
            std::cout << "플레이어 제거: " << it->second->GetName() << " (ID: " << player_id << ")" << std::endl;
            players_.erase(it);
        }
    }

    Player* GameWorld::GetPlayer(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto it = players_.find(player_id);
        return (it != players_.end()) ? it->second.get() : nullptr;
    }

    std::vector<Player*> GameWorld::GetPlayersInMap(uint32_t map_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);
        std::vector<Player*>        players_in_map;

        for (auto& [id, player] : players_)
        {
            if (player->GetCurrentMapID() == map_id)
            {
                players_in_map.push_back(player.get());
            }
        }

        return players_in_map;
    }

    bool GameWorld::LoadMap(uint32_t map_id, const std::string& map_data)
    {
        std::lock_guard<std::mutex> lock(maps_mutex_);

        if (maps_.find(map_id) != maps_.end())
        {
            return false; // 맵이 이미 로드됨
        }

        maps_[map_id] = std::make_unique<Map>(map_id);
        std::cout << "맵 로드: ID " << map_id << std::endl;
        return true;
    }

    Map* GameWorld::GetMap(uint32_t map_id)
    {
        std::lock_guard<std::mutex> lock(maps_mutex_);

        auto it = maps_.find(map_id);
        return (it != maps_.end()) ? it->second.get() : nullptr;
    }

    std::vector<Map*> GameWorld::GetAllMaps()
    {
        std::lock_guard<std::mutex> lock(maps_mutex_);
        std::vector<Map*>           all_maps;

        for (auto& [id, map] : maps_)
        {
            all_maps.push_back(map.get());
        }

        return all_maps;
    }

    void GameWorld::SpawnNPC(uint32_t npc_id, uint32_t map_id, float x, float y, float z)
    {
        std::lock_guard<std::mutex> lock(npcs_mutex_);

        if (npcs_.find(npc_id) == npcs_.end())
        {
            npcs_[npc_id] = std::make_unique<NPC>(npc_id);
            std::cout << "NPC 스폰: ID " << npc_id << " 맵 " << map_id << " 위치 (" << x << ", " << y << ", " << z
                      << ")" << std::endl;
        }
    }

    void GameWorld::RemoveNPC(uint32_t npc_id)
    {
        std::lock_guard<std::mutex> lock(npcs_mutex_);

        auto it = npcs_.find(npc_id);
        if (it != npcs_.end())
        {
            std::cout << "NPC 제거: ID " << npc_id << std::endl;
            npcs_.erase(it);
        }
    }

    NPC* GameWorld::GetNPC(uint32_t npc_id)
    {
        std::lock_guard<std::mutex> lock(npcs_mutex_);

        auto it = npcs_.find(npc_id);
        return (it != npcs_.end()) ? it->second.get() : nullptr;
    }

    void GameWorld::Update(float delta_time)
    {
        // 플레이어 업데이트
        {
            std::lock_guard<std::mutex> lock(players_mutex_);
            for (auto& [id, player] : players_)
            {
                player->Update(delta_time);
            }
        }

        // AI 처리
        ProcessAI(delta_time);

        // 이동 처리
        ProcessMovement(delta_time);

        // 전투 처리
        ProcessCombat(delta_time);
    }

    void GameWorld::StartGameLoop()
    {
        if (running_.load())
        {
            return;
        }

        running_.store(true);
        game_loop_thread_ = std::thread(&GameWorld::GameLoopThread, this);
        std::cout << "게임 루프 시작" << std::endl;
    }

    void GameWorld::StopGameLoop()
    {
        if (!running_.load())
        {
            return;
        }

        running_.store(false);

        if (game_loop_thread_.joinable())
        {
            game_loop_thread_.join();
        }

        std::cout << "게임 루프 중지" << std::endl;
    }

    void GameWorld::RegisterEventHandler(std::function<void(const std::string&, const std::string&)> handler)
    {
        std::lock_guard<std::mutex> lock(event_handlers_mutex_);
        event_handlers_.push_back(handler);
    }

    void GameWorld::TriggerEvent(const std::string& event_type, const std::string& data)
    {
        std::lock_guard<std::mutex> lock(event_handlers_mutex_);

        for (auto& handler : event_handlers_)
        {
            handler(event_type, data);
        }
    }

    void GameWorld::GameLoopThread()
    {
        std::cout << "게임 루프 스레드 시작" << std::endl;

        last_update_time_             = std::chrono::steady_clock::now();
        const float target_frame_time = 1.0f / target_fps_;

        while (running_.load())
        {
            auto current_time = std::chrono::steady_clock::now();
            auto delta_time   = std::chrono::duration<float>(current_time - last_update_time_).count();

            if (delta_time >= target_frame_time)
            {
                Update(delta_time);
                last_update_time_ = current_time;
            }
            else
            {
                // 프레임 시간 조절
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        std::cout << "게임 루프 스레드 종료" << std::endl;
    }

    void GameWorld::ProcessAI(float delta_time)
    {
        // NPC AI 처리 (기본 구현)
        std::lock_guard<std::mutex> lock(npcs_mutex_);
        // 실제로는 각 NPC의 AI 로직을 실행
    }

    void GameWorld::ProcessMovement(float delta_time)
    {
        // 플레이어 이동 처리 (기본 구현)
        std::lock_guard<std::mutex> lock(players_mutex_);
        // 실제로는 각 플레이어의 이동을 처리
    }

    void GameWorld::ProcessCombat(float delta_time)
    {
        // 전투 시스템 처리 (기본 구현)
        std::lock_guard<std::mutex> lock(players_mutex_);
        // 실제로는 전투 로직을 실행
    }

} // namespace bt
