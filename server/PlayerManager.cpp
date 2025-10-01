#include <chrono>
#include <iostream>
#include <random>

#include "PlayerManager.h"
#include "Network/WebSocket/BeastHttpWebSocketServer.h"
#include "Common/LockOrder.h"

namespace bt
{

    PlayerManager::PlayerManager() : next_player_id_(1) {}

    PlayerManager::~PlayerManager() {}

    void PlayerManager::AddPlayer(std::shared_ptr<Player> player)
    {
        LOCK_PLAYERS(players_mutex_);
        players_[player->GetID()] = player;
        std::cout << "플레이어 추가: " << player->GetName() << " (ID: " << player->GetID() << ")" << std::endl;
    }

    void PlayerManager::RemovePlayer(uint32_t player_id)
    {
        LOCK_PLAYERS(players_mutex_);
        auto                        it = players_.find(player_id);
        if (it != players_.end())
        {
            std::cout << "플레이어 제거: " << it->second->GetName() << " (ID: " << player_id << ")" << std::endl;
            players_.erase(it);
        }
    }

    std::shared_ptr<Player> PlayerManager::GetPlayer(uint32_t player_id)
    {
        LOCK_PLAYERS(players_mutex_);
        auto                        it = players_.find(player_id);
        if (it != players_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Player>> PlayerManager::GetAllPlayers()
    {
        LOCK_PLAYERS(players_mutex_);
        std::vector<std::shared_ptr<Player>> result;
        for (const auto& [id, player] : players_)
        {
            result.push_back(player);
        }
        return result;
    }

    std::vector<std::shared_ptr<Player>> PlayerManager::GetPlayersInRange(const MonsterPosition& position,
                                                                             float                  range)
    {
        std::lock_guard<std::mutex>          lock(players_mutex_);
        std::vector<std::shared_ptr<Player>> result;

        for (const auto& [id, player] : players_)
        {
            if (!player->IsAlive())
                continue;

            auto  player_pos = player->GetPosition();
            float distance = std::sqrt(std::pow(player_pos.x - position.x, 2) + std::pow(player_pos.y - position.y, 2) +
                                       std::pow(player_pos.z - position.z, 2));

            if (distance <= range)
            {
                result.push_back(player);
            }
        }
        return result;
    }

    std::shared_ptr<Player> PlayerManager::CreatePlayer(const std::string& name, const MonsterPosition& position)
    {
        uint32_t player_id = next_player_id_.fetch_add(1);
        auto     player    = std::make_shared<Player>(player_id, name);

        // 위치 설정
        player->SetPosition(position.x, position.y, position.z, position.rotation);

        // 기본 스탯 설정
        PlayerStats stats;
        stats.health     = 100;
        stats.max_health = 100;
        stats.mana       = 50;
        stats.max_mana   = 50;
        stats.level      = 1;
        stats.experience = 0;
        player->SetStats(stats);

        // 플레이어 추가
        AddPlayer(player);

        std::cout << "플레이어 생성: " << name << " (ID: " << player_id << ")" << std::endl;
        return player;
    }

    std::shared_ptr<Player> PlayerManager::CreatePlayerForClient(uint32_t               client_id,
                                                                    const std::string&     name,
                                                                    const MonsterPosition& position)
    {
        uint32_t player_id = next_player_id_.fetch_add(1);
        auto     player    = std::make_shared<Player>(player_id, name);

        // 위치 설정
        player->SetPosition(position.x, position.y, position.z, position.rotation);

        // 기본 스탯 설정
        PlayerStats stats;
        stats.health     = 100;
        stats.max_health = 100;
        stats.mana       = 50;
        stats.max_mana   = 50;
        stats.level      = 1;
        stats.experience = 0;
        player->SetStats(stats);

        // 클라이언트 ID와 플레이어 ID 매핑
        {
            std::lock_guard<std::mutex> lock(players_mutex_);
            client_to_player_id_[client_id] = player_id;
            player_to_client_id_[player_id] = client_id;
            players_[player_id]             = player;
        }

        std::cout << "클라이언트용 플레이어 생성: " << name << " (ID: " << player_id << ", Client ID: " << client_id
                  << ")" << std::endl;
        return player;
    }

    void PlayerManager::RemovePlayerByClientID(uint32_t client_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto client_it = client_to_player_id_.find(client_id);
        if (client_it != client_to_player_id_.end())
        {
            uint32_t player_id = client_it->second;

            // 플레이어 제거
            auto player_it = players_.find(player_id);
            if (player_it != players_.end())
            {
                std::cout << "클라이언트용 플레이어 제거: " << player_it->second->GetName() << " (ID: " << player_id
                          << ", Client ID: " << client_id << ")" << std::endl;
                players_.erase(player_it);
            }

            // 매핑 제거
            client_to_player_id_.erase(client_it);
            player_to_client_id_.erase(player_id);
        }
    }

    std::shared_ptr<Player> PlayerManager::GetPlayerByClientID(uint32_t client_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto client_it = client_to_player_id_.find(client_id);
        if (client_it != client_to_player_id_.end())
        {
            uint32_t player_id = client_it->second;
            auto     player_it = players_.find(player_id);
            if (player_it != players_.end())
            {
                return player_it->second;
            }
        }
        return nullptr;
    }

    uint32_t PlayerManager::GetClientIDByPlayerID(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto player_it = player_to_client_id_.find(player_id);
        if (player_it != player_to_client_id_.end())
        {
            return player_it->second;
        }
        return 0; // 0은 유효하지 않은 클라이언트 ID
    }

    void PlayerManager::ProcessCombat(float delta_time)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        for (const auto& [id, player] : players_)
        {
            if (!player->IsAlive())
            {
                // 플레이어가 사망한 경우 처리 (필요시 리스폰 로직 추가)
                continue;
            }

            // 전투 로직 처리 (필요시 구현)
        }
    }

    void PlayerManager::AttackPlayer(uint32_t attacker_id, uint32_t tarGetID, uint32_t damage)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto target = GetPlayer(tarGetID);
        if (!target || !target->IsAlive())
            return;

        target->TakeDamage(damage);
        std::cout << "플레이어 " << attacker_id << "가 플레이어 " << tarGetID << "에게 " << damage << " 데미지"
                  << std::endl;
    }

    void PlayerManager::AttackMonster(uint32_t attacker_id, uint32_t tarGetID, uint32_t damage)
    {
        // 이 함수는 MonsterManager에서 호출되어야 함
        // 여기서는 플레이어의 공격력을 가져오는 역할만 함
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto attacker = GetPlayer(attacker_id);
        if (!attacker || !attacker->IsAlive())
            return;

        std::cout << "플레이어 " << attacker_id << "가 몬스터 " << tarGetID << "를 공격 (데미지: " << damage << ")"
                  << std::endl;
    }

    void PlayerManager::ProcessPlayerRespawn(float delta_time)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        for (const auto& [id, player] : players_)
        {
            if (!player->IsAlive())
            {
                // 플레이어가 사망한 경우 자동 리스폰 (필요시 시간 체크 로직 추가)
                RespawnPlayer(id);
            }
        }
    }

    void PlayerManager::SetHttpWebSocketServer(std::shared_ptr<bt::BeastHttpWebSocketServer> server)
    {
        http_websocket_server_ = server;
        std::cout << "PlayerManager에 통합 HTTP+WebSocket 서버 설정 완료" << std::endl;
    }

    void PlayerManager::Update(float delta_time)
    {
        static int update_count = 0;
        update_count++;
        if (update_count % 100 == 0)
        { // 10초마다 로그 출력
            std::cout << "PlayerManager::update 호출됨 (카운트: " << update_count
                      << ", 플레이어 수: " << players_.size() << ")" << std::endl;
        }

        ProcessCombat(delta_time);
        ProcessPlayerAI(delta_time);
        ProcessPlayerRespawn(delta_time);
    }

    void PlayerManager::ProcessPlayerAI(float delta_time)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        float current_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count() /
            1000.0f;

        for (const auto& [id, player] : players_)
        {
            if (!player->IsAlive())
                continue;

            // 랜덤 이동
            auto last_move_it = player_last_move_time_.find(id);
            if (last_move_it == player_last_move_time_.end() ||
                current_time - last_move_it->second >= player_move_interval_)
            {
                MovePlayerToRandomLocation(id);
                player_last_move_time_[id] = current_time;
            }

            // 근처 몬스터 공격
            auto last_attack_it = player_last_attack_time_.find(id);
            if (last_attack_it == player_last_attack_time_.end() ||
                current_time - last_attack_it->second >= player_attack_interval_)
            {
                AttackNearbyMonster(id);
                player_last_attack_time_[id] = current_time;
            }
        }
    }

    void PlayerManager::MovePlayerToRandomLocation(uint32_t player_id)
    {
        auto player = GetPlayer(player_id);
        if (!player)
        {
            return;
        }

        // 랜덤 위치 생성 (현재 위치에서 ±50 범위)
        static std::random_device                    rd;
        static std::mt19937                          gen(rd());
        static std::uniform_real_distribution<float> dis(-50.0f, 50.0f);

        auto  current_pos = player->GetPosition();
        float new_x       = current_pos.x + dis(gen);
        float new_y       = current_pos.y;
        float new_z       = current_pos.z + dis(gen);

        player->SetPosition(new_x, new_y, new_z, current_pos.rotation);

        std::cout << "플레이어 " << player_id << " 랜덤 이동: (" << new_x << ", " << new_y << ", " << new_z << ")"
                  << std::endl;
    }

    void PlayerManager::AttackNearbyMonster(uint32_t player_id)
    {
        auto player = GetPlayer(player_id);
        if (!player)
        {
            return;
        }

        // 근처 몬스터를 찾아서 공격하는 로직
        // 주의: MonsterManager 직접 호출 시 데드락 위험 있음
        // 대신 이벤트 시스템이나 비동기 메시지 사용 권장
        std::cout << "플레이어 " << player_id << " 근처 몬스터 공격 시도" << std::endl;
    }

    void PlayerManager::RespawnPlayer(uint32_t player_id)
    {
        auto player = GetPlayer(player_id);
        if (!player)
        {
            return;
        }

        // 랜덤 리스폰 포인트에서 부활
        MonsterPosition respawn_pos = GetRandomRespawnPoint();
        player->SetPosition(respawn_pos.x, respawn_pos.y, respawn_pos.z, respawn_pos.rotation);

        // 스탯 복구
        PlayerStats stats = player->GetStats();
        stats.health      = stats.max_health;
        stats.mana        = stats.max_mana;
        player->SetStats(stats);

        std::cout << "플레이어 " << player_id << " 리스폰: (" << respawn_pos.x << ", " << respawn_pos.y << ", "
                  << respawn_pos.z << ")" << std::endl;
    }

    void PlayerManager::SetPlayerRespawnPoints(const std::vector<MonsterPosition>& points)
    {
        player_respawn_points_ = points;
    }

    MonsterPosition PlayerManager::GetRandomRespawnPoint() const
    {
        if (player_respawn_points_.empty())
        {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        static std::random_device             rd;
        static std::mt19937                   gen(rd());
        std::uniform_int_distribution<size_t> dis(0, player_respawn_points_.size() - 1);

        return player_respawn_points_[dis(gen)];
    }

} // namespace bt
