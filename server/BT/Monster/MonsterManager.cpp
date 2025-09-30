#include "MonsterManager.h"
#include "MonsterFactory.h"
#include "MonsterBTExecutor.h"
#include "../../Player.h"
#include "../../PlayerManager.h"
#include "../../Network/WebSocket/SimpleWebSocketServer.h"
#include "../BehaviorTreeEngine.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <cmath>
#include <nlohmann/json.hpp>

namespace bt
{

    // MonsterManager 구현
    MonsterManager::MonsterManager() : next_monster_id_(1), auto_spawn_enabled_(false) {}

    MonsterManager::~MonsterManager() {}

    void MonsterManager::add_monster(std::shared_ptr<Monster> monster)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        uint32_t                    id = next_monster_id_.fetch_add(1);
        monster->SetID(id); // 몬스터 객체에 ID 설정
        monsters_[id] = monster;
        std::cout << "몬스터 추가: " << monster->GetName() << " (ID: " << id << ")" << std::endl;
    }

    void MonsterManager::remove_monster(uint32_t monster_id)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        auto                        it = monsters_.find(monster_id);
        if (it != monsters_.end())
        {
            std::cout << "몬스터 제거: " << it->second->GetName() << " (ID: " << monster_id << ")" << std::endl;
            monsters_.erase(it);
        }
    }

    std::shared_ptr<Monster> MonsterManager::get_monster(uint32_t monster_id)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        auto                        it = monsters_.find(monster_id);
        if (it != monsters_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Monster>> MonsterManager::get_all_monsters()
    {
        std::lock_guard<std::mutex>           lock(monsters_mutex_);
        std::vector<std::shared_ptr<Monster>> result;
        for (const auto& [id, monster] : monsters_)
        {
            result.push_back(monster);
        }
        return result;
    }

    std::vector<std::shared_ptr<Monster>> MonsterManager::get_monsters_in_range(const MonsterPosition& position,
                                                                                float                  range)
    {
        std::lock_guard<std::mutex>           lock(monsters_mutex_);
        std::vector<std::shared_ptr<Monster>> result;

        for (const auto& [id, monster] : monsters_)
        {
            const auto& monster_pos = monster->GetPosition();
            float       distance =
                std::sqrt(std::pow(monster_pos.x - position.x, 2) + std::pow(monster_pos.y - position.y, 2) +
                          std::pow(monster_pos.z - position.z, 2));

            if (distance <= range)
            {
                result.push_back(monster);
            }
        }

        return result;
    }

    std::shared_ptr<Monster> MonsterManager::spawn_monster(MonsterType            type,
                                                           const std::string&     name,
                                                           const MonsterPosition& position)
    {
        auto monster = MonsterFactory::create_monster(type, name, position);
        add_monster(monster);

        // AI 이름과 BT 이름 설정
        monster->SetAIName(name);
        std::string bt_name = MonsterFactory::GetBTName(type);
        monster->SetBTName(bt_name);

        // Behavior Tree 엔진에 AI 등록
        if (bt_engine_ && monster->GetAI())
        {
            bt_engine_->register_monster_ai(monster->GetAI());

            // Behavior Tree 설정
            auto tree = bt_engine_->get_tree(bt_name);
            if (tree)
            {
                monster->GetAI()->set_behavior_tree(tree);
                std::cout << "몬스터 AI Behavior Tree 설정: " << name << " -> " << bt_name << std::endl;
            }
            else
            {
                std::cout << "몬스터 AI Behavior Tree 설정 실패: " << name << " -> " << bt_name
                          << " (트리를 찾을 수 없음)" << std::endl;
            }
        }

        return monster;
    }

    void MonsterManager::add_spawn_config(const MonsterSpawnConfig& config)
    {
        std::lock_guard<std::mutex> lock(spawn_mutex_);
        spawn_configs_.push_back(config);
        std::cout << "몬스터 스폰 설정 추가: " << config.name << " (타입: " << static_cast<int>(config.type) << ")"
                  << std::endl;
    }

    void MonsterManager::remove_spawn_config(MonsterType type, const std::string& name)
    {
        std::lock_guard<std::mutex> lock(spawn_mutex_);
        spawn_configs_.erase(std::remove_if(spawn_configs_.begin(),
                                            spawn_configs_.end(),
                                            [&](const MonsterSpawnConfig& config)
                                            {
                                                return config.type == type && config.name == name;
                                            }),
                             spawn_configs_.end());
    }

    void MonsterManager::start_auto_spawn()
    {
        auto_spawn_enabled_.store(true);
        std::cout << "몬스터 자동 스폰 시작" << std::endl;
    }

    void MonsterManager::stop_auto_spawn()
    {
        auto_spawn_enabled_.store(false);
        std::cout << "몬스터 자동 스폰 중지" << std::endl;
    }

    void MonsterManager::clear_all_spawn_configs()
    {
        std::lock_guard<std::mutex> lock(spawn_mutex_);
        spawn_configs_.clear();
        last_spawn_times_.clear();
        std::cout << "모든 몬스터 스폰 설정이 초기화되었습니다." << std::endl;
    }

    void MonsterManager::set_bt_engine(std::shared_ptr<BehaviorTreeEngine> engine)
    {
        bt_engine_ = engine;
        std::cout << "MonsterManager에 Behavior Tree 엔진 설정 완료" << std::endl;
    }

    void MonsterManager::set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server)
    {
        websocket_server_ = server;
        std::cout << "MonsterManager에 WebSocket 서버 설정 완료" << std::endl;
    }

    void MonsterManager::set_player_manager(std::shared_ptr<PlayerManager> manager)
    {
        player_manager_ = manager;
        std::cout << "MonsterManager에 PlayerManager 설정 완료" << std::endl;
    }

    bool MonsterManager::load_spawn_configs_from_file(const std::string& file_path)
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            std::cerr << "설정 파일을 열 수 없습니다: " << file_path << std::endl;
            return false;
        }

        // 기존 설정 초기화
        clear_all_spawn_configs();

        try
        {
            nlohmann::json json_data;
            file >> json_data;
            file.close();

            if (!json_data.contains("monster_spawns") || !json_data["monster_spawns"].is_array())
            {
                std::cerr << "monster_spawns 배열을 찾을 수 없습니다." << std::endl;
                return false;
            }

            int config_count = 0;
            for (const auto& monster_data : json_data["monster_spawns"])
            {
                MonsterSpawnConfig config;

                // 기본값 설정
                config.respawn_time    = 30.0f;
                config.max_count       = 1;
                config.spawn_radius    = 5.0f;
                config.auto_spawn      = true;
                config.position        = {0.0f, 0.0f, 0.0f, 0.0f};
                config.detection_range = 15.0f;
                config.attack_range    = 3.0f;
                config.chase_range     = 25.0f;
                config.health          = 100;
                config.damage          = 10;
                config.move_speed      = 2.0f;

                // 타입 파싱
                if (monster_data.contains("type") && monster_data["type"].is_string())
                {
                    config.type = MonsterFactory::string_to_monster_type(monster_data["type"]);
                }

                // 이름 파싱
                if (monster_data.contains("name") && monster_data["name"].is_string())
                {
                    config.name = monster_data["name"];
                }

                // 위치 파싱
                if (monster_data.contains("position") && monster_data["position"].is_object())
                {
                    const auto& pos          = monster_data["position"];
                    config.position.x        = pos.value("x", 0.0f);
                    config.position.y        = pos.value("y", 0.0f);
                    config.position.z        = pos.value("z", 0.0f);
                    config.position.rotation = pos.value("rotation", 0.0f);
                }

                // 기타 설정들 파싱
                config.respawn_time    = monster_data.value("respawn_time", 30.0f);
                config.max_count       = monster_data.value("max_count", 1);
                config.spawn_radius    = monster_data.value("spawn_radius", 5.0f);
                config.auto_spawn      = monster_data.value("auto_spawn", true);
                config.detection_range = monster_data.value("detection_range", 15.0f);
                config.attack_range    = monster_data.value("attack_range", 3.0f);
                config.chase_range     = monster_data.value("chase_range", 25.0f);
                config.health          = monster_data.value("health", 100);
                config.damage          = monster_data.value("damage", 10);
                config.move_speed      = monster_data.value("move_speed", 2.0f);

                // 순찰점 파싱
                if (monster_data.contains("patrol_points") && monster_data["patrol_points"].is_array())
                {
                    config.patrol_points.clear();
                    for (const auto& point : monster_data["patrol_points"])
                    {
                        MonsterPosition patrol_pos;
                        patrol_pos.x        = point.value("x", 0.0f);
                        patrol_pos.y        = point.value("y", 0.0f);
                        patrol_pos.z        = point.value("z", 0.0f);
                        patrol_pos.rotation = point.value("rotation", 0.0f);
                        config.patrol_points.push_back(patrol_pos);
                    }
                }

                add_spawn_config(config);
                config_count++;
            }

            std::cout << "설정 파일에서 " << config_count << "개의 몬스터 스폰 설정을 로드했습니다." << std::endl;
            return config_count > 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
            return false;
        }
    }

    void MonsterManager::process_auto_spawn(float /* delta_time */)
    {
        if (!auto_spawn_enabled_.load())
        {
            static int debug_count = 0;
            if (debug_count++ % 100 == 0)
            {
                std::cout << "자동 스폰이 비활성화되어 있습니다." << std::endl;
            }
            return;
        }

        std::lock_guard<std::mutex> lock(spawn_mutex_);
        auto                        now = std::chrono::steady_clock::now();

        static int debug_count = 0;
        if (debug_count++ % 100 == 0)
        {
            std::cout << "자동 스폰 처리 중... 설정된 스폰 설정 수: " << spawn_configs_.size() << std::endl;
        }

        // 10초마다 스폰 설정 수 로그 출력 (성능 개선)
        if (debug_count % 100 == 0)
        {
            std::cout << "현재 스폰 설정 수: " << spawn_configs_.size() << std::endl;
        }
        for (const auto& config : spawn_configs_)
        {
            if (!config.auto_spawn)
            {
                if (debug_count % 100 == 0)
                {
                    std::cout << "스폰 설정 " << config.name << "은 자동 스폰이 비활성화되어 있습니다." << std::endl;
                }
                continue;
            }

            std::string config_key = config.name + "_" + std::to_string(static_cast<int>(config.type));

            // 현재 이름의 몬스터 수 확인
            size_t current_count = get_monster_count_by_name(config.name);
            // 10초마다 스폰 상태 로그 출력 (성능 개선)
            if (debug_count % 100 == 0)
            {
                std::cout << "스폰 설정 " << config.name << " - 현재 몬스터 수: " << current_count
                          << ", 최대 수: " << config.max_count << std::endl;
            }
            if (current_count >= config.max_count)
            {
                if (debug_count % 100 == 0)
                {
                    std::cout << "스폰 설정 " << config.name << " - 최대 수에 도달했습니다." << std::endl;
                }
                continue;
            }

            // 마지막 스폰 시간 확인
            auto last_spawn_it = last_spawn_times_.find(config_key);
            if (last_spawn_it != last_spawn_times_.end())
            {
                auto time_since_last =
                    std::chrono::duration_cast<std::chrono::seconds>(now - last_spawn_it->second).count();
                if (time_since_last < config.respawn_time)
                    continue;
            }

            // 몬스터 스폰
            MonsterPosition spawn_pos = config.position;
            // 스폰 반경 내에서 랜덤 위치 조정
            if (config.spawn_radius > 0)
            {
                std::random_device                    rd;
                std::mt19937                          gen(rd());
                std::uniform_real_distribution<float> dis(-config.spawn_radius, config.spawn_radius);
                spawn_pos.x += dis(gen);
                spawn_pos.z += dis(gen);
            }

            auto monster = spawn_monster(config.type, config.name, spawn_pos);
            if (monster)
            {
                last_spawn_times_[config_key] = now;
                std::cout << "자동 스폰: " << config.name << " at (" << spawn_pos.x << ", " << spawn_pos.y << ", "
                          << spawn_pos.z << ")" << std::endl;
            }
        }
    }

    void MonsterManager::process_respawn(float /* delta_time */)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);

        std::vector<uint32_t> dead_monsters;
        for (const auto& [id, monster] : monsters_)
        {
            if (!monster->IsAlive())
            {
                dead_monsters.push_back(id);
            }
        }

        // 죽은 몬스터 제거
        for (uint32_t id : dead_monsters)
        {
            monsters_.erase(id);
            std::cout << "죽은 몬스터 제거: ID " << id << std::endl;
        }
    }

    void MonsterManager::update(float delta_time)
    {
        static int update_count = 0;
        update_count++;

        // 10초마다 로그 출력 (성능 개선)
        if (update_count % 100 == 0)
        {
            std::cout << "MonsterManager::update 호출됨 (카운트: " << update_count
                      << ", 몬스터 수: " << monsters_.size() << ")" << std::endl;
        }

        process_auto_spawn(delta_time);
        process_respawn(delta_time);

        std::lock_guard<std::mutex> lock(monsters_mutex_);

        // 모든 몬스터와 플레이어 정보 수집
        std::vector<std::shared_ptr<Monster>> all_monsters;
        std::vector<std::shared_ptr<Player>>  all_players;

        for (const auto& [id, monster] : monsters_)
        {
            all_monsters.push_back(monster);
        }

        // PlayerManager에서 플레이어 정보 가져오기
        if (player_manager_)
        {
            all_players = player_manager_->get_all_players();
        }

        // 각 몬스터의 환경 정보 업데이트
        for (const auto& [id, monster] : monsters_)
        {
            monster->update_environment_info(all_players, all_monsters);
            monster->update(delta_time);
        }

        // 주기적으로 WebSocket으로 몬스터 상태 업데이트 브로드캐스트
        static float broadcast_timer = 0.0f;
        broadcast_timer += delta_time;
        if (broadcast_timer >= 1.0f)
        { // 1초마다 브로드캐스트
            broadcast_timer = 0.0f;

            if (websocket_server_)
            {
                nlohmann::json event;
                event["type"]     = "monster_update";
                event["monsters"] = nlohmann::json::array();

                for (const auto& [id, monster] : monsters_)
                {
                    if (monster)
                    {
                        nlohmann::json monster_data;
                        monster_data["id"]       = monster->GetID();
                        monster_data["name"]     = monster->GetName();
                        monster_data["type"]     = MonsterFactory::monster_type_to_string(monster->GetType());
                        monster_data["position"] = {
                            {"x",        monster->GetPosition().x       },
                            {"y",        monster->GetPosition().y       },
                            {"z",        monster->GetPosition().z       },
                            {"rotation", monster->GetPosition().rotation}
                        };
                        monster_data["health"]     = monster->GetStats().health;
                        monster_data["max_health"] = monster->GetStats().max_health;
                        monster_data["level"]      = monster->GetStats().level;
                        monster_data["ai_name"]    = monster->GetAIName();
                        monster_data["bt_name"]    = monster->GetBTName();
                        event["monsters"].push_back(monster_data);
                    }
                }

                std::cout << "WebSocket 브로드캐스트: " << event["monsters"].size() << "마리 몬스터 전송" << std::endl;
                websocket_server_->broadcast(event.dump());

                // 서버 통계도 함께 브로드캐스트
                nlohmann::json stats_event;
                stats_event["type"]                   = "server_stats";
                stats_event["data"]["totalMonsters"]  = monsters_.size();
                stats_event["data"]["activeMonsters"] = monsters_.size();
                // PlayerManager에서 실제 플레이어 수 가져오기
                if (player_manager_)
                {
                    stats_event["data"]["totalPlayers"] = player_manager_->get_player_count();
                }
                else
                {
                    stats_event["data"]["totalPlayers"] = 0;
                }
                stats_event["data"]["registeredBTTrees"] = 7; // 등록된 BT 트리 수
                stats_event["data"]["connectedClients"]  = websocket_server_->get_connected_clients();
                websocket_server_->broadcast(stats_event.dump());
            }
        }
    }

    size_t MonsterManager::get_monster_count_by_type(MonsterType type) const
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        size_t                      count = 0;
        for (const auto& [id, monster] : monsters_)
        {
            if (monster->GetType() == type)
            {
                count++;
            }
        }
        return count;
    }

    size_t MonsterManager::get_monster_count_by_name(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        size_t                      count = 0;
        for (const auto& [id, monster] : monsters_)
        {
            if (monster->GetName() == name)
            {
                count++;
            }
        }
        return count;
    }

} // namespace bt
