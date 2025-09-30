#include <chrono>
#include <iostream>
#include <random>

#include "PlayerManager.h"
#include "SimpleWebSocketServer.h"

namespace bt
{

    PlayerManager::PlayerManager() : next_player_id_(1) {}

    PlayerManager::~PlayerManager() {}

    void PlayerManager::add_player(std::shared_ptr<Player> player)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);
        players_[player->GetID()] = player;
        std::cout << "플레이어 추가: " << player->GetName() << " (ID: " << player->GetID() << ")" << std::endl;
    }

    void PlayerManager::remove_player(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);
        auto                        it = players_.find(player_id);
        if (it != players_.end())
        {
            std::cout << "플레이어 제거: " << it->second->GetName() << " (ID: " << player_id << ")" << std::endl;
            players_.erase(it);
        }
    }

    std::shared_ptr<Player> PlayerManager::get_player(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);
        auto                        it = players_.find(player_id);
        if (it != players_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Player>> PlayerManager::get_all_players()
    {
        std::lock_guard<std::mutex>          lock(players_mutex_);
        std::vector<std::shared_ptr<Player>> result;
        for (const auto& [id, player] : players_)
        {
            result.push_back(player);
        }
        return result;
    }

    std::vector<std::shared_ptr<Player>> PlayerManager::get_players_in_range(const MonsterPosition& position,
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

    std::shared_ptr<Player> PlayerManager::create_player(const std::string& name, const MonsterPosition& position)
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
        add_player(player);

        std::cout << "플레이어 생성: " << name << " (ID: " << player_id << ")" << std::endl;
        return player;
    }

    std::shared_ptr<Player> PlayerManager::create_player_for_client(uint32_t               client_id,
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

    void PlayerManager::remove_player_by_client_id(uint32_t client_id)
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

    std::shared_ptr<Player> PlayerManager::get_player_by_client_id(uint32_t client_id)
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

    uint32_t PlayerManager::get_client_id_by_player_id(uint32_t player_id)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto player_it = player_to_client_id_.find(player_id);
        if (player_it != player_to_client_id_.end())
        {
            return player_it->second;
        }
        return 0; // 0은 유효하지 않은 클라이언트 ID
    }

    void PlayerManager::process_combat(float delta_time)
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

    void PlayerManager::attack_player(uint32_t attacker_id, uint32_t tarGetID, uint32_t damage)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto target = get_player(tarGetID);
        if (!target || !target->IsAlive())
            return;

        target->TakeDamage(damage);
        std::cout << "플레이어 " << attacker_id << "가 플레이어 " << tarGetID << "에게 " << damage << " 데미지"
                  << std::endl;
    }

    void PlayerManager::attack_monster(uint32_t attacker_id, uint32_t tarGetID, uint32_t damage)
    {
        // 이 함수는 MonsterManager에서 호출되어야 함
        // 여기서는 플레이어의 공격력을 가져오는 역할만 함
        std::lock_guard<std::mutex> lock(players_mutex_);

        auto attacker = get_player(attacker_id);
        if (!attacker || !attacker->IsAlive())
            return;

        std::cout << "플레이어 " << attacker_id << "가 몬스터 " << tarGetID << "를 공격 (데미지: " << damage << ")"
                  << std::endl;
    }

    void PlayerManager::process_player_respawn(float delta_time)
    {
        std::lock_guard<std::mutex> lock(players_mutex_);

        for (const auto& [id, player] : players_)
        {
            if (!player->IsAlive())
            {
                // 플레이어가 사망한 경우 자동 리스폰 (필요시 시간 체크 로직 추가)
                respawn_player(id);
            }
        }
    }

    void PlayerManager::set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server)
    {
        websocket_server_ = server;
        std::cout << "PlayerManager에 WebSocket 서버 설정 완료" << std::endl;
    }

    void PlayerManager::update(float delta_time)
    {
        static int update_count = 0;
        update_count++;
        if (update_count % 100 == 0)
        { // 10초마다 로그 출력
            std::cout << "PlayerManager::update 호출됨 (카운트: " << update_count
                      << ", 플레이어 수: " << players_.size() << ")" << std::endl;
        }

        process_combat(delta_time);
        process_player_ai(delta_time);
        process_player_respawn(delta_time);
    }

    void PlayerManager::process_player_ai(float delta_time)
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
                move_player_to_random_location(id);
                player_last_move_time_[id] = current_time;
            }

            // 근처 몬스터 공격
            auto last_attack_it = player_last_attack_time_.find(id);
            if (last_attack_it == player_last_attack_time_.end() ||
                current_time - last_attack_it->second >= player_attack_interval_)
            {
                attack_nearby_monster(id);
                player_last_attack_time_[id] = current_time;
            }
        }
    }

    void PlayerManager::move_player_to_random_location(uint32_t player_id)
    {
        auto player = get_player(player_id);
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

    void PlayerManager::attack_nearby_monster(uint32_t player_id)
    {
        auto player = get_player(player_id);
        if (!player)
        {
            return;
        }

        // 근처 몬스터를 찾아서 공격하는 로직 (MonsterManager와 연동 필요)
        std::cout << "플레이어 " << player_id << " 근처 몬스터 공격 시도" << std::endl;
    }

    void PlayerManager::respawn_player(uint32_t player_id)
    {
        auto player = get_player(player_id);
        if (!player)
        {
            return;
        }

        // 랜덤 리스폰 포인트에서 부활
        MonsterPosition respawn_pos = get_random_respawn_point();
        player->SetPosition(respawn_pos.x, respawn_pos.y, respawn_pos.z, respawn_pos.rotation);

        // 스탯 복구
        PlayerStats stats = player->GetStats();
        stats.health      = stats.max_health;
        stats.mana        = stats.max_mana;
        player->SetStats(stats);

        std::cout << "플레이어 " << player_id << " 리스폰: (" << respawn_pos.x << ", " << respawn_pos.y << ", "
                  << respawn_pos.z << ")" << std::endl;
    }

    void PlayerManager::set_player_respawn_points(const std::vector<MonsterPosition>& points)
    {
        player_respawn_points_ = points;
    }

    MonsterPosition PlayerManager::get_random_respawn_point() const
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
