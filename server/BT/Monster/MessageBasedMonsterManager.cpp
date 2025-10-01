#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

#include "../../../shared/Game/PacketProtocol.h"
#include "../../BT/Engine.h"
#include "../../Network/WebSocket/BeastHttpWebSocketServer.h"
#include "../PlayerManager.h"
#include "MessageBasedMonsterManager.h"
#include "MonsterBTExecutor.h"
#include "MonsterFactory.h"

namespace bt
{

    MessageBasedMonsterManager::MessageBasedMonsterManager()
        : next_monster_id_(1), auto_spawn_enabled_(false), running_(false), update_enabled_(true)
    {
        std::cout << "MessageBasedMonsterManager 생성됨" << std::endl;
    }

    MessageBasedMonsterManager::~MessageBasedMonsterManager()
    {
        Stop();
        std::cout << "MessageBasedMonsterManager 소멸됨" << std::endl;
    }

    void MessageBasedMonsterManager::HandleMessage(std::shared_ptr<GameMessage> message)
    {
        if (!message)
            return;

        switch (message->GetType())
        {
            case MessageType::MONSTER_SPAWN:
                if (auto spawn_msg = std::dynamic_pointer_cast<MonsterSpawnMessage>(message))
                {
                    HandleMonsterSpawn(spawn_msg);
                }
                break;

            case MessageType::MONSTER_MOVE:
                if (auto move_msg = std::dynamic_pointer_cast<MonsterMoveMessage>(message))
                {
                    HandleMonsterMove(move_msg);
                }
                break;

            case MessageType::MONSTER_DEATH:
                if (auto death_msg = std::dynamic_pointer_cast<MonsterDeathMessage>(message))
                {
                    HandleMonsterDeath(death_msg);
                }
                break;

            case MessageType::GAME_STATE_UPDATE:
                if (auto state_msg = std::dynamic_pointer_cast<GameStateUpdateMessage>(message))
                {
                    HandleGameStateUpdate(state_msg);
                }
                break;

            case MessageType::SYSTEM_SHUTDOWN:
                if (auto shutdown_msg = std::dynamic_pointer_cast<SystemShutdownMessage>(message))
                {
                    HandleSystemShutdown(shutdown_msg);
                }
                break;

            default:
                // 다른 메시지 타입은 무시
                break;
        }
    }

    void MessageBasedMonsterManager::AddMonster(std::shared_ptr<Monster> monster)
    {
        if (!monster)
            return;

        uint32_t id   = monster->GetID();
        monsters_[id] = monster;

        // 스폰 메시지 전송
        auto position = monster->GetPosition();
        SendMonsterSpawnMessage(id,
                                monster->GetName(),
                                MonsterFactory::MonsterTypeToString(monster->GetType()),
                                position,
                                monster->GetStats().health,
                                monster->GetStats().max_health);

        std::cout << "몬스터 추가됨: " << monster->GetName() << " (ID: " << id << ")" << std::endl;
    }

    void MessageBasedMonsterManager::RemoveMonster(uint32_t monster_id)
    {
        auto it = monsters_.find(monster_id);
        if (it != monsters_.end())
        {
            auto monster  = it->second;
            auto position = monster->GetPosition();

            // 데스 메시지 전송
            SendMonsterDeathMessage(monster_id, monster->GetName(), position);

            monsters_.erase(it);
            std::cout << "몬스터 제거됨: " << monster->GetName() << " (ID: " << monster_id << ")" << std::endl;
        }
    }

    std::shared_ptr<Monster> MessageBasedMonsterManager::GetMonster(uint32_t monster_id)
    {
        auto it = monsters_.find(monster_id);
        return (it != monsters_.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<Monster>> MessageBasedMonsterManager::GetAllMonsters()
    {
        std::vector<std::shared_ptr<Monster>> result;
        result.reserve(monsters_.size());

        for (const auto& pair : monsters_)
        {
            result.push_back(pair.second);
        }

        return result;
    }

    std::vector<std::shared_ptr<Monster>> MessageBasedMonsterManager::GetMonstersInRange(
        const MonsterPosition& position, float range)
    {
        std::vector<std::shared_ptr<Monster>> result;

        for (const auto& pair : monsters_)
        {
            auto monster = pair.second;
            if (!monster)
                continue;

            auto  monster_pos = monster->GetPosition();
            float distance =
                std::sqrt(std::pow(monster_pos.x - position.x, 2) + std::pow(monster_pos.z - position.z, 2));

            if (distance <= range)
            {
                result.push_back(monster);
            }
        }

        return result;
    }

    std::shared_ptr<Monster> MessageBasedMonsterManager::SpawnMonster(MonsterType            type,
                                                                      const std::string&     name,
                                                                      const MonsterPosition& position)
    {
        uint32_t id = next_monster_id_++;

        // MonsterFactory를 사용하여 AI가 포함된 몬스터 생성
        auto monster = MonsterFactory::CreateMonster(type, name, position);
        if (!monster)
        {
            std::cerr << "몬스터 생성 실패: " << name << std::endl;
            return nullptr;
        }

        // Behavior Tree 설정
        if (bt_engine_ && monster->GetAI())
        {
            std::string bt_name = monster->GetBTName();
            auto        tree    = bt_engine_->GetTree(bt_name);
            if (tree)
            {
                monster->GetAI()->SetBehaviorTree(tree);
                std::cout << "몬스터 생성: " << name << " (타입: " << static_cast<int>(type) << ", BT: " << bt_name
                          << ")" << std::endl;
            }
            else
            {
                std::cerr << "Behavior Tree를 찾을 수 없음: " << bt_name << std::endl;
            }
        }
        else
        {
            std::cerr << "BT 엔진 또는 AI가 없음: " << name << std::endl;
        }

        AddMonster(monster);
        return monster;
    }

    void MessageBasedMonsterManager::AddSpawnConfig(const MonsterSpawnConfig& config)
    {
        spawn_configs_.push_back(config);
        std::cout << "스폰 설정 추가됨: " << config.name << " (" << MonsterFactory::MonsterTypeToString(config.type)
                  << ")" << std::endl;
    }

    void MessageBasedMonsterManager::RemoveSpawnConfig(MonsterType type, const std::string& name)
    {
        auto it = std::find_if(spawn_configs_.begin(),
                               spawn_configs_.end(),
                               [type, &name](const MonsterSpawnConfig& config)
                               {
                                   return config.type == type && config.name == name;
                               });

        if (it != spawn_configs_.end())
        {
            spawn_configs_.erase(it);
            std::cout << "스폰 설정 제거됨: " << name << std::endl;
        }
    }

    void MessageBasedMonsterManager::StartAutoSpawn()
    {
        auto_spawn_enabled_ = true;
        std::cout << "자동 스폰 시작됨" << std::endl;
    }

    void MessageBasedMonsterManager::StopAutoSpawn()
    {
        auto_spawn_enabled_ = false;
        std::cout << "자동 스폰 중지됨" << std::endl;
    }

    bool MessageBasedMonsterManager::LoadSpawnConfigsFromFile(const std::string& file_path)
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            std::cerr << "스폰 설정 파일을 열 수 없습니다: " << file_path << std::endl;
            return false;
        }

        try
        {
            nlohmann::json config;
            file >> config;

            std::cout << "JSON 파일 로드 성공, 키들: ";
            for (auto& [key, value] : config.items())
            {
                std::cout << key << " ";
            }
            std::cout << std::endl;

            ClearAllSpawnConfigs();

            if (!config.contains("monster_spawns"))
            {
                std::cerr << "JSON에 'monster_spawns' 키가 없습니다!" << std::endl;
                return false;
            }

            int config_count = 0;
            for (const auto& spawn_config : config["monster_spawns"])
            {
                try
                {
                    std::cout << "스폰 설정 " << config_count << " 파싱 중..." << std::endl;

                    MonsterSpawnConfig config_obj;
                    std::cout << "  - type 파싱 중..." << std::endl;
                    config_obj.type = MonsterFactory::StringToMonsterType(spawn_config["type"]);
                    std::cout << "  - name 파싱 중..." << std::endl;
                    config_obj.name = spawn_config["name"];
                    std::cout << "  - position 파싱 중..." << std::endl;
                    config_obj.position.x        = spawn_config["position"]["x"];
                    config_obj.position.y        = spawn_config["position"]["y"];
                    config_obj.position.z        = spawn_config["position"]["z"];
                    config_obj.position.rotation = spawn_config["position"]["rotation"];
                    std::cout << "  - respawn_time 파싱 중..." << std::endl;
                    config_obj.respawn_time = spawn_config["respawn_time"];
                    std::cout << "  - max_count 파싱 중..." << std::endl;
                    config_obj.max_count = spawn_config["max_count"];
                    std::cout << "  - spawn_radius 파싱 중..." << std::endl;
                    config_obj.spawn_radius = spawn_config["spawn_radius"];
                    std::cout << "  - auto_spawn 파싱 중..." << std::endl;
                    config_obj.auto_spawn = spawn_config["auto_spawn"];

                    // 순찰 경로 파싱
                    if (spawn_config.contains("patrol_points"))
                    {
                        std::cout << "  - patrol_points 파싱 중..." << std::endl;
                        ParsePatrolPoints(spawn_config["patrol_points"].dump(), config_obj.patrol_points);
                    }

                    std::cout << "  - AddSpawnConfig 호출 중..." << std::endl;
                    AddSpawnConfig(config_obj);
                    std::cout << "스폰 설정 " << config_count << " 추가 완료: " << config_obj.name << std::endl;
                    config_count++;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "스폰 설정 " << config_count << " 파싱 오류: " << e.what() << std::endl;
                    config_count++;
                }
            }

            std::cout << "스폰 설정 로드 완료: " << spawn_configs_.size() << "개 설정" << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "스폰 설정 파일 파싱 오류: " << e.what() << std::endl;
            return false;
        }
    }

    void MessageBasedMonsterManager::ClearAllSpawnConfigs()
    {
        spawn_configs_.clear();
        last_spawn_times_.clear();
        std::cout << "모든 스폰 설정 제거됨" << std::endl;
    }

    void MessageBasedMonsterManager::Update(float delta_time)
    {
        // 메시지 기반 시스템에서는 별도 스레드에서 처리
        // 이 메서드는 호환성을 위해 유지하지만 실제로는 아무것도 하지 않음
    }

    void MessageBasedMonsterManager::SetBTEngine(std::shared_ptr<Engine> engine)
    {
        bt_engine_ = engine;

        // 기존 몬스터들에게도 BT 엔진 설정은 Monster 클래스에서 직접 지원하지 않으므로 주석 처리
        // for (auto& pair : monsters_) {
        //     if (pair.second) {
        //         pair.second->SetBTEngine(engine);
        //     }
        // }
    }

    void MessageBasedMonsterManager::SetMessageProcessor(std::shared_ptr<GameMessageProcessor> processor)
    {
        message_processor_ = processor;
    }

    void MessageBasedMonsterManager::SetPlayerManager(std::shared_ptr<MessageBasedPlayerManager> manager)
    {
        player_manager_ = manager;
    }

    size_t MessageBasedMonsterManager::GetMonsterCount() const
    {
        return monsters_.size();
    }

    size_t MessageBasedMonsterManager::GetMonsterCountByType(MonsterType type) const
    {
        size_t count = 0;
        for (const auto& pair : monsters_)
        {
            if (pair.second && pair.second->GetType() == type)
            {
                count++;
            }
        }
        return count;
    }

    size_t MessageBasedMonsterManager::GetMonsterCountByName(const std::string& name) const
    {
        size_t count = 0;
        for (const auto& pair : monsters_)
        {
            if (pair.second && pair.second->GetName() == name)
            {
                count++;
            }
        }
        return count;
    }

    void MessageBasedMonsterManager::Start()
    {
        if (running_)
            return;

        running_        = true;
        update_enabled_ = true;

        // 업데이트 스레드 시작
        update_thread_ = std::thread(
            [this]
            {
                UpdateThread();
            });

        std::cout << "MessageBasedMonsterManager 시작됨" << std::endl;
    }

    void MessageBasedMonsterManager::Stop()
    {
        if (!running_)
            return;

        running_        = false;
        update_enabled_ = false;

        if (update_thread_.joinable())
        {
            update_thread_.join();
        }

        std::cout << "MessageBasedMonsterManager 중지됨" << std::endl;
    }

    void MessageBasedMonsterManager::ProcessAutoSpawn(float delta_time)
    {
        if (!auto_spawn_enabled_)
            return;

        auto now = std::chrono::steady_clock::now();

        for (const auto& config : spawn_configs_)
        {
            if (!config.auto_spawn)
                continue;

            // 현재 타입의 몬스터 수 확인
            size_t current_count = GetMonsterCountByType(config.type);
            if (current_count >= config.max_count)
                continue;

            // 마지막 스폰 시간 확인
            auto last_spawn_it = last_spawn_times_.find(config.name);
            if (last_spawn_it != last_spawn_times_.end())
            {
                auto time_since_last =
                    std::chrono::duration_cast<std::chrono::seconds>(now - last_spawn_it->second).count();
                if (time_since_last < config.respawn_time)
                    continue;
            }

            // 몬스터 스폰
            auto spawn_pos = config.position;
            if (config.spawn_radius > 0)
            {
                std::random_device                    rd;
                std::mt19937                          gen(rd());
                std::uniform_real_distribution<float> dis(-config.spawn_radius, config.spawn_radius);
                spawn_pos.x += dis(gen);
                spawn_pos.z += dis(gen);
            }

            SpawnMonster(config.type, config.name, spawn_pos);
            last_spawn_times_[config.name] = now;

            std::cout << "자동 스폰: " << config.name << " at (" << spawn_pos.x << ", " << spawn_pos.z << ")"
                      << std::endl;
        }
    }

    void MessageBasedMonsterManager::ProcessRespawn(float delta_time)
    {
        // 리스폰 로직은 기존과 동일하게 구현
        // 필요시 추가 구현
    }

    void MessageBasedMonsterManager::ProcessMonsterUpdates(float delta_time)
    {
        for (auto& pair : monsters_)
        {
            auto monster = pair.second;
            if (!monster)
                continue;

            // 몬스터 업데이트
            monster->Update(delta_time);

            // 위치 변경 감지 및 메시지 전송
            // (실제 구현에서는 이전 위치와 비교)
        }
    }

    void MessageBasedMonsterManager::UpdateThread()
    {
        auto last_time = std::chrono::steady_clock::now();

        while (running_)
        {
            auto  current_time = std::chrono::steady_clock::now();
            float delta_time   = std::chrono::duration<float>(current_time - last_time).count();
            last_time          = current_time;

            if (update_enabled_)
            {
                ProcessAutoSpawn(delta_time);
                ProcessRespawn(delta_time);
                ProcessMonsterUpdates(delta_time);
            }

            // 60 FPS로 제한
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    MonsterPosition MessageBasedMonsterManager::GetRandomRespawnPoint() const
    {
        if (player_respawn_points_.empty())
        {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        std::random_device                    rd;
        std::mt19937                          gen(rd());
        std::uniform_int_distribution<size_t> dis(0, player_respawn_points_.size() - 1);

        return player_respawn_points_[dis(gen)];
    }

    void MessageBasedMonsterManager::ParsePatrolPoints(const std::string&            points_content,
                                                       std::vector<MonsterPosition>& points)
    {
        try
        {
            nlohmann::json points_json = nlohmann::json::parse(points_content);

            for (const auto& point : points_json)
            {
                MonsterPosition pos;
                pos.x = point["x"];
                pos.y = point["y"];
                pos.z = point["z"];
                // rotation은 기본값 0.0으로 설정 (JSON에 없음)
                pos.rotation = point.contains("rotation") ? point["rotation"].get<float>() : 0.0f;
                points.push_back(pos);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "순찰 경로 파싱 오류: " << e.what() << std::endl;
        }
    }

    void MessageBasedMonsterManager::HandleMonsterSpawn(std::shared_ptr<MonsterSpawnMessage> message)
    {
        // 스폰 메시지 처리 로직
        std::cout << "몬스터 스폰 메시지 처리: " << message->monster_name << std::endl;
    }

    void MessageBasedMonsterManager::HandleMonsterMove(std::shared_ptr<MonsterMoveMessage> message)
    {
        // 이동 메시지 처리 로직
        std::cout << "몬스터 이동 메시지 처리: ID " << message->monster_id << std::endl;
    }

    void MessageBasedMonsterManager::HandleMonsterDeath(std::shared_ptr<MonsterDeathMessage> message)
    {
        // 데스 메시지 처리 로직
        std::cout << "몬스터 데스 메시지 처리: " << message->monster_name << std::endl;
    }

    void MessageBasedMonsterManager::HandleGameStateUpdate(std::shared_ptr<GameStateUpdateMessage> message)
    {
        // 게임 상태 업데이트 처리 로직
        std::cout << "게임 상태 업데이트 메시지 처리" << std::endl;
    }

    void MessageBasedMonsterManager::HandleSystemShutdown(std::shared_ptr<SystemShutdownMessage> message)
    {
        std::cout << "시스템 셧다운 메시지 수신: " << message->reason << std::endl;
        Stop();
    }

    void MessageBasedMonsterManager::SendMonsterSpawnMessage(uint32_t               monster_id,
                                                             const std::string&     name,
                                                             const std::string&     type,
                                                             const MonsterPosition& position,
                                                             float                  health,
                                                             float                  max_health)
    {
        if (!message_processor_)
            return;

        auto message = std::make_shared<MonsterSpawnMessage>(
            monster_id, name, type, position.x, position.y, position.z, position.rotation, health, max_health);

        message_processor_->SendToNetwork(message);
    }

    void MessageBasedMonsterManager::SendMonsterMoveMessage(uint32_t               monster_id,
                                                            const MonsterPosition& from,
                                                            const MonsterPosition& to)
    {
        if (!message_processor_)
            return;

        auto message = std::make_shared<MonsterMoveMessage>(monster_id, from.x, from.y, from.z, to.x, to.y, to.z);

        message_processor_->SendToNetwork(message);
    }

    void MessageBasedMonsterManager::SendMonsterDeathMessage(uint32_t               monster_id,
                                                             const std::string&     name,
                                                             const MonsterPosition& position)
    {
        if (!message_processor_)
            return;

        auto message = std::make_shared<MonsterDeathMessage>(monster_id, name, position.x, position.y, position.z);

        message_processor_->SendToNetwork(message);
    }

    std::vector<MonsterState> MessageBasedMonsterManager::GetMonsterStates() const
    {
        std::vector<MonsterState> states;

        for (const auto& [id, monster] : monsters_)
        {
            if (!monster)
                continue;

            MonsterState state;
            state.set_id(id);
            state.set_name(monster->GetName());

            MonsterPosition pos = monster->GetPosition();
            state.set_x(pos.x);
            state.set_y(pos.y);
            state.set_z(pos.z);
            state.set_rotation(pos.rotation);

            state.set_health(monster->GetStats().health);
            state.set_max_health(monster->GetMaxHealth());
            state.set_level(monster->GetStats().level);
            state.set_type(static_cast<uint32_t>(monster->GetType()));

            states.push_back(state);
        }

        return states;
    }

} // namespace bt
