#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include <cmath>
#include <nlohmann/json.hpp>

#include "monster_ai.h"
#include "player.h"
#include "player_manager.h"
#include "simple_websocket_server.h"

namespace bt
{

    // Monster 구현
    Monster::Monster(const std::string& name, MonsterType type, const MonsterPosition& position)
        : id_(0)
        , name_(name)
        , type_(type)
        , state_(MonsterState::IDLE)
        , position_(position)
        , ai_name_("")
        , bt_name_("")
        , target_id_(0)
        , current_patrol_index_(0)
        , spawn_position_(position)
    {
        // 기본 통계 설정
        stats_            = MonsterFactory::get_default_stats(type);
        last_update_time_ = std::chrono::steady_clock::now();

        // 기본 순찰점 설정 (스폰 위치 주변)
        set_default_patrol_points();

        std::cout << "몬스터 생성: " << name_ << " (타입: " << static_cast<int>(type_) << ")" << std::endl;
    }

    Monster::~Monster()
    {
        std::cout << "몬스터 소멸: " << name_ << std::endl;
    }

    void Monster::set_position(float x, float y, float z, float rotation)
    {
        position_.x        = x;
        position_.y        = y;
        position_.z        = z;
        position_.rotation = rotation;
    }

    void Monster::move_to(float x, float y, float z, float rotation)
    {
        set_position(x, y, z, rotation);
        std::cout << "몬스터 " << name_ << " 이동: (" << x << ", " << y << ", " << z << ")" << std::endl;
    }

    void Monster::take_damage(uint32_t damage)
    {
        if (damage >= stats_.health)
        {
            stats_.health = 0;
            state_        = MonsterState::DEAD;
            death_time_   = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count() /
                          1000.0f;
            std::cout << "몬스터 " << name_ << " 사망!" << std::endl;
        }
        else
        {
            stats_.health -= damage;
            std::cout << "몬스터 " << name_ << " 데미지 받음: " << damage << " (남은 체력: " << stats_.health << ")"
                      << std::endl;
        }
    }

    void Monster::heal(uint32_t amount)
    {
        if (state_ == MonsterState::DEAD)
        {
            return;
        }

        uint32_t old_health = stats_.health;
        stats_.health += amount;

        if (stats_.health > stats_.max_health)
        {
            stats_.health = stats_.max_health;
        }

        uint32_t actual_heal = stats_.health - old_health;
        if (actual_heal > 0)
        {
            std::cout << "몬스터 " << name_ << " 치료됨: " << actual_heal << " (현재 체력: " << stats_.health << ")"
                      << std::endl;
        }
    }

    void Monster::update_environment_info(const std::vector<std::shared_ptr<Player>>&  players,
                                          const std::vector<std::shared_ptr<Monster>>& monsters)
    {
        environment_info_ = EnvironmentInfo();

        // 주변 플레이어 검색
        for (const auto& player : players)
        {
            if (!player || !player->is_alive())
                continue;

            const auto& player_pos = player->get_position();
            float       distance =
                std::sqrt(std::pow(player_pos.x - position_.x, 2) + std::pow(player_pos.y - position_.y, 2) +
                          std::pow(player_pos.z - position_.z, 2));

            // 디버깅: 플레이어와의 거리 로그 (더 자주 출력)
            static int debug_count = 0;
            if (debug_count++ % 20 == 0)
            { // 2초마다 로그 출력
                std::cout << "Monster " << name_ << " - Player " << player->get_id() << " 거리: " << distance
                          << " (탐지범위: " << stats_.detection_range << ")" << std::endl;
            }

            if (distance <= stats_.detection_range)
            {
                environment_info_.nearby_players.push_back(player->get_id());

                if (environment_info_.nearest_enemy_distance < 0 || distance < environment_info_.nearest_enemy_distance)
                {
                    environment_info_.nearest_enemy_distance = distance;
                    environment_info_.nearest_enemy_id       = player->get_id();

                    std::cout << "Monster " << name_ << " - Player " << player->get_id()
                              << " 탐지됨! 거리: " << distance << std::endl;
                }
            }
        }

        // 주변 몬스터 검색
        for (const auto& monster : monsters)
        {
            if (!monster || monster.get() == this || !monster->is_alive())
                continue;

            const auto& monster_pos = monster->get_position();
            float       distance =
                std::sqrt(std::pow(monster_pos.x - position_.x, 2) + std::pow(monster_pos.y - position_.y, 2) +
                          std::pow(monster_pos.z - position_.z, 2));

            if (distance <= stats_.detection_range)
            {
                environment_info_.nearby_monsters.push_back(monster->get_target_id());
            }
        }

        // 시야 확보 여부 (간단한 구현)
        environment_info_.has_line_of_sight = true; // TODO: 실제 시야 계산 구현
    }

    // 환경 인지 헬퍼 메서드들 구현
    bool Monster::has_enemy_in_range(float range) const
    {
        return environment_info_.nearest_enemy_distance >= 0 && environment_info_.nearest_enemy_distance <= range;
    }

    bool Monster::has_enemy_in_attack_range() const
    {
        return has_enemy_in_range(stats_.attack_range);
    }

    bool Monster::has_enemy_in_detection_range() const
    {
        return has_enemy_in_range(stats_.detection_range);
    }

    bool Monster::has_enemy_in_chase_range() const
    {
        return has_enemy_in_range(chase_range_);
    }

    float Monster::get_distance_to_nearest_enemy() const
    {
        return environment_info_.nearest_enemy_distance;
    }

    uint32_t Monster::get_nearest_enemy_id() const
    {
        return environment_info_.nearest_enemy_id;
    }

    bool Monster::can_see_target(uint32_t target_id) const
    {
        // 간단한 시야 구현: 거리와 시야 확보 여부 확인
        if (target_id == 0)
            return false;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.has_line_of_sight &&
                   environment_info_.nearest_enemy_distance <= stats_.detection_range;
        }

        return false;
    }

    bool Monster::is_target_in_range(uint32_t target_id, float range) const
    {
        if (target_id == 0)
            return false;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.nearest_enemy_distance <= range;
        }

        return false;
    }

    float Monster::get_distance_to_target(uint32_t target_id) const
    {
        if (target_id == 0)
            return -1.0f;

        // 현재 타겟이 가장 가까운 적인지 확인
        if (target_id == environment_info_.nearest_enemy_id)
        {
            return environment_info_.nearest_enemy_distance;
        }

        return -1.0f;
    }

    void Monster::attack_target()
    {
        if (!has_target())
            return;

        // TODO: 실제 타겟에게 데미지 주기 (PlayerManager를 통해)
        std::cout << "Monster " << name_ << " attacks target " << target_id_ << " for " << damage_ << " damage!"
                  << std::endl;

        // 공격 상태로 변경
        set_state(MonsterState::ATTACK);
    }

    void Monster::update(float delta_time)
    {
        static std::map<std::string, int> update_counts;
        update_counts[name_]++;

        if (update_counts[name_] % 50 == 0)
        { // 5초마다 로그 출력
            std::cout << "Monster::update 호출됨: " << name_ << " (카운트: " << update_counts[name_] << ")"
                      << std::endl;
        }

        // AI 업데이트
        if (ai_)
        {
            ai_->update(delta_time);
            if (update_counts[name_] % 50 == 0)
            {
                std::cout << "Monster::update - AI 업데이트 완료: " << name_ << std::endl;
            }
        }
        else
        {
            if (update_counts[name_] % 50 == 0)
            {
                std::cout << "Monster::update - AI가 없음: " << name_ << std::endl;
            }
        }

        last_update_time_ = std::chrono::steady_clock::now();
    }

    void Monster::set_default_patrol_points()
    {
        patrol_points_.clear();

        // 스폰 위치를 중심으로 4방향 순찰점 생성
        float patrol_radius = 15.0f; // 순찰 반경

        patrol_points_.push_back(spawn_position_); // 스폰 위치
        patrol_points_.push_back({spawn_position_.x + patrol_radius, spawn_position_.y, spawn_position_.z, 0.0f});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z + patrol_radius, 0.0f});
        patrol_points_.push_back({spawn_position_.x - patrol_radius, spawn_position_.y, spawn_position_.z, 0.0f});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z - patrol_radius, 0.0f});

        current_patrol_index_ = 0;
    }

    void Monster::set_patrol_points(const std::vector<MonsterPosition>& points)
    {
        patrol_points_        = points;
        current_patrol_index_ = 0;
    }

    void Monster::add_patrol_point(const MonsterPosition& point)
    {
        patrol_points_.push_back(point);
    }

    MonsterPosition Monster::get_next_patrol_point()
    {
        if (patrol_points_.empty())
        {
            return spawn_position_;
        }

        MonsterPosition point = patrol_points_[current_patrol_index_];
        current_patrol_index_ = (current_patrol_index_ + 1) % patrol_points_.size();
        return point;
    }

    // MonsterFactory 구현
    std::shared_ptr<Monster> MonsterFactory::create_monster(MonsterType            type,
                                                            const std::string&     name,
                                                            const MonsterPosition& position)
    {
        auto monster = std::make_shared<Monster>(name, type, position);

        // 몬스터 타입에 따른 AI 생성 및 연결
        std::string bt_name = get_bt_name(type);
        auto        ai      = std::make_shared<MonsterAI>(name, bt_name);
        monster->set_ai(ai);
        ai->set_monster(monster); // AI에서 몬스터 참조 설정

        std::cout << "몬스터 AI 연결: " << name << " -> " << bt_name << std::endl;

        return monster;
    }

    MonsterStats MonsterFactory::get_default_stats(MonsterType type)
    {
        MonsterStats stats;

        switch (type)
        {
            case MonsterType::GOBLIN:
                stats.level           = 1;
                stats.health          = 50;
                stats.max_health      = 50;
                stats.mana            = 20;
                stats.max_mana        = 20;
                stats.attack_power    = 8;
                stats.defense         = 3;
                stats.move_speed      = 1.2f;
                stats.attack_range    = 1.5f;
                stats.detection_range = 50.0f; // 탐지 범위 증가
                break;

            case MonsterType::ORC:
                stats.level           = 3;
                stats.health          = 120;
                stats.max_health      = 120;
                stats.mana            = 40;
                stats.max_mana        = 40;
                stats.attack_power    = 15;
                stats.defense         = 8;
                stats.move_speed      = 1.0f;
                stats.attack_range    = 2.0f;
                stats.detection_range = 60.0f; // 탐지 범위 증가
                break;

            case MonsterType::DRAGON:
                stats.level           = 20;
                stats.health          = 2000;
                stats.max_health      = 2000;
                stats.mana            = 500;
                stats.max_mana        = 500;
                stats.attack_power    = 100;
                stats.defense         = 50;
                stats.move_speed      = 2.0f;
                stats.attack_range    = 5.0f;
                stats.detection_range = 20.0f;
                break;

            case MonsterType::SKELETON:
                stats.level           = 2;
                stats.health          = 80;
                stats.max_health      = 80;
                stats.mana            = 30;
                stats.max_mana        = 30;
                stats.attack_power    = 12;
                stats.defense         = 5;
                stats.move_speed      = 0.8f;
                stats.attack_range    = 1.8f;
                stats.detection_range = 9.0f;
                break;

            case MonsterType::ZOMBIE:
                stats.level           = 1;
                stats.health          = 100;
                stats.max_health      = 100;
                stats.mana            = 10;
                stats.max_mana        = 10;
                stats.attack_power    = 10;
                stats.defense         = 4;
                stats.move_speed      = 0.5f;
                stats.attack_range    = 1.2f;
                stats.detection_range = 6.0f;
                break;

            case MonsterType::NPC_MERCHANT:
                stats.level           = 1;
                stats.health          = 50;
                stats.max_health      = 50;
                stats.mana            = 100;
                stats.max_mana        = 100;
                stats.attack_power    = 5;
                stats.defense         = 2;
                stats.move_speed      = 1.0f;
                stats.attack_range    = 0.0f;
                stats.detection_range = 5.0f;
                break;

            case MonsterType::NPC_GUARD:
                stats.level           = 5;
                stats.health          = 200;
                stats.max_health      = 200;
                stats.mana            = 80;
                stats.max_mana        = 80;
                stats.attack_power    = 25;
                stats.defense         = 15;
                stats.move_speed      = 1.5f;
                stats.attack_range    = 3.0f;
                stats.detection_range = 15.0f;
                break;
        }

        return stats;
    }

    std::string MonsterFactory::get_bt_name(MonsterType type)
    {
        switch (type)
        {
            case MonsterType::GOBLIN:
                return "goblin_bt";
            case MonsterType::ORC:
                return "orc_bt";
            case MonsterType::DRAGON:
                return "dragon_bt";
            case MonsterType::SKELETON:
                return "skeleton_bt";
            case MonsterType::ZOMBIE:
                return "zombie_bt";
            case MonsterType::NPC_MERCHANT:
                return "merchant_bt";
            case MonsterType::NPC_GUARD:
                return "guard_bt";
            default:
                return "default_bt";
        }
    }

    // MonsterManager 구현
    MonsterManager::MonsterManager() : next_monster_id_(1), auto_spawn_enabled_(false) {}

    MonsterManager::~MonsterManager() {}

    void MonsterManager::add_monster(std::shared_ptr<Monster> monster)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        uint32_t                    id = next_monster_id_.fetch_add(1);
        monster->set_id(id); // 몬스터 객체에 ID 설정
        monsters_[id] = monster;
        std::cout << "몬스터 추가: " << monster->get_name() << " (ID: " << id << ")" << std::endl;
    }

    void MonsterManager::remove_monster(uint32_t monster_id)
    {
        std::lock_guard<std::mutex> lock(monsters_mutex_);
        auto                        it = monsters_.find(monster_id);
        if (it != monsters_.end())
        {
            std::cout << "몬스터 제거: " << it->second->get_name() << " (ID: " << monster_id << ")" << std::endl;
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
            const auto& monster_pos = monster->get_position();
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
        monster->set_ai_name(name);
        std::string bt_name = MonsterFactory::get_bt_name(type);
        monster->set_bt_name(bt_name);

        // Behavior Tree 엔진에 AI 등록
        if (bt_engine_ && monster->get_ai())
        {
            bt_engine_->register_monster_ai(monster->get_ai());

            // Behavior Tree 설정
            auto tree = bt_engine_->get_tree(bt_name);
            if (tree)
            {
                monster->get_ai()->set_behavior_tree(tree);
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
            if (!monster->is_alive())
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
                        monster_data["id"]       = monster->get_id();
                        monster_data["name"]     = monster->get_name();
                        monster_data["type"]     = MonsterFactory::monster_type_to_string(monster->get_type());
                        monster_data["position"] = {
                            {"x",        monster->get_position().x       },
                            {"y",        monster->get_position().y       },
                            {"z",        monster->get_position().z       },
                            {"rotation", monster->get_position().rotation}
                        };
                        monster_data["health"]     = monster->get_stats().health;
                        monster_data["max_health"] = monster->get_stats().max_health;
                        monster_data["level"]      = monster->get_stats().level;
                        monster_data["ai_name"]    = monster->get_ai_name();
                        monster_data["bt_name"]    = monster->get_bt_name();
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
            if (monster->get_type() == type)
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
            if (monster->get_name() == name)
            {
                count++;
            }
        }
        return count;
    }

    // MonsterBTs 네임스페이스 구현
    namespace MonsterBTs
    {

        std::shared_ptr<BehaviorTree> create_goblin_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("goblin_bt");
            auto root = std::make_shared<BTSelector>("goblin_root");

            // 적 발견 및 공격 시퀀스
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            // 적 추적 시퀀스
            auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
            chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            chase_sequence->add_child(std::make_shared<BTCondition>("path_clear", MonsterConditions::is_path_clear));
            chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));

            // 적 탐지 시퀀스
            auto detection_sequence = std::make_shared<BTSequence>("detection_sequence");
            detection_sequence->add_child(
                std::make_shared<BTCondition>("enemy_in_range", MonsterConditions::is_enemy_in_detection_range));
            detection_sequence->add_child(std::make_shared<BTAction>("set_target", MonsterActions::set_target));

            // 순찰
            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(chase_sequence);
            root->add_child(detection_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_orc_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("orc_bt");
            auto root = std::make_shared<BTSelector>("orc_root");

            // 고블린과 유사하지만 더 공격적
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
            chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));

            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(chase_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_skeleton_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("skeleton_bt");
            auto root = std::make_shared<BTSelector>("skeleton_root");

            // 스켈레톤은 느리지만 지속적으로 추적
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
            chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));

            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(chase_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_zombie_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("zombie_bt");
            auto root = std::make_shared<BTSelector>("zombie_root");

            // 좀비는 느리고 단순한 AI
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_dragon_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("dragon_bt");
            auto root = std::make_shared<BTSelector>("dragon_root");

            // 드래곤은 복잡한 AI (보스급)
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
            chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));

            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(chase_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_merchant_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("merchant_bt");
            auto root = std::make_shared<BTSelector>("merchant_root");

            // 상인은 공격하지 않고 대화만
            auto idle_action = std::make_shared<BTAction>("idle", MonsterActions::idle);

            root->add_child(idle_action);

            tree->set_root(root);
            return tree;
        }

        std::shared_ptr<BehaviorTree> create_guard_bt()
        {
            auto tree = std::make_shared<BehaviorTree>("guard_bt");
            auto root = std::make_shared<BTSelector>("guard_root");

            // 경비병은 특정 영역을 순찰하고 적을 공격
            auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
            attack_sequence->add_child(
                std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
            attack_sequence->add_child(
                std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
            attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));

            auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);

            root->add_child(attack_sequence);
            root->add_child(patrol_action);

            tree->set_root(root);
            return tree;
        }

    } // namespace MonsterBTs

    // MonsterActions 네임스페이스 구현
    namespace MonsterActions
    {

        BTNodeStatus attack(BTContext& context)
        {
            // 공격 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return BTNodeStatus::FAILURE;

            // 공격 실행
            monster->attack_target();
            std::cout << "MonsterActions::attack - " << monster->get_name() << "이(가) 타겟 "
                      << monster->get_target_id() << "을(를) 공격했습니다." << std::endl;

            return BTNodeStatus::SUCCESS;
        }

        BTNodeStatus move_to_target(BTContext& context)
        {
            // 타겟으로 이동 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return BTNodeStatus::FAILURE;

            // 타겟 방향으로 이동 (간단한 구현)
            uint32_t target_id = monster->get_nearest_enemy_id();
            if (target_id != 0)
            {
                // TODO: 실제 타겟 위치로 이동 구현
                std::cout << "MonsterActions::move_to_target - " << monster->get_name() << "이(가) 타겟 " << target_id
                          << "을(를) 향해 이동합니다." << std::endl;
                return BTNodeStatus::SUCCESS;
            }

            return BTNodeStatus::FAILURE;
        }

        BTNodeStatus patrol(BTContext& context)
        {
            // 순찰 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::FAILURE;

            if (monster->has_patrol_points())
            {
                MonsterPosition next_point = monster->get_next_patrol_point();
                monster->move_to(next_point.x, next_point.y, next_point.z, next_point.rotation);
                std::cout << "MonsterActions::patrol - " << monster->get_name() << "이(가) 순찰 중입니다." << std::endl;
                return BTNodeStatus::SUCCESS;
            }

            return BTNodeStatus::FAILURE;
        }

        BTNodeStatus idle(BTContext& context)
        {
            // 대기 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::SUCCESS;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::SUCCESS;

            // 대기 상태 (아무것도 하지 않음)
            std::cout << "MonsterActions::idle - " << monster->get_name() << "이(가) 대기 중입니다." << std::endl;

            return BTNodeStatus::SUCCESS;
        }

        BTNodeStatus flee(BTContext& context)
        {
            // 도망 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::FAILURE;

            // 타겟과 반대 방향으로 이동
            std::cout << "MonsterActions::flee - " << monster->get_name() << "이(가) 도망치고 있습니다." << std::endl;

            return BTNodeStatus::SUCCESS;
        }

        BTNodeStatus die(BTContext& context)
        {
            // 사망 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::SUCCESS;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::SUCCESS;

            // 사망 처리
            monster->set_state(MonsterState::DEAD);
            std::cout << "MonsterActions::die - " << monster->get_name() << "이(가) 사망했습니다." << std::endl;

            return BTNodeStatus::SUCCESS;
        }

        BTNodeStatus return_to_patrol(BTContext& context)
        {
            // 순찰 복귀 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::FAILURE;

            // 타겟 초기화하고 순찰 상태로 복귀
            monster->clear_target();
            monster->set_state(MonsterState::PATROL);
            monster->reset_patrol_index();

            std::cout << "MonsterActions::return_to_patrol - " << monster->get_name() << "이(가) 순찰로 복귀했습니다."
                      << std::endl;

            return BTNodeStatus::SUCCESS;
        }

        BTNodeStatus set_target(BTContext& context)
        {
            // 타겟 설정 로직 구현
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return BTNodeStatus::FAILURE;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return BTNodeStatus::FAILURE;

            // 가장 가까운 적을 타겟으로 설정
            uint32_t nearest_enemy_id = monster->get_nearest_enemy_id();
            if (nearest_enemy_id != 0)
            {
                monster->set_target_id(nearest_enemy_id);
                monster->set_state(MonsterState::CHASE);
                std::cout << "MonsterActions::set_target - " << monster->get_name() << "이(가) 타겟 "
                          << nearest_enemy_id << "을(를) 설정했습니다." << std::endl;
                return BTNodeStatus::SUCCESS;
            }

            return BTNodeStatus::FAILURE;
        }

    } // namespace MonsterActions

    // MonsterConditions 네임스페이스 구현
    namespace MonsterConditions
    {

        bool has_target(BTContext& context)
        {
            // 타겟 존재 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_target();
        }

        bool is_target_in_range(BTContext& context)
        {
            // 타겟이 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            uint32_t target_id = monster->get_target_id();
            return monster->is_target_in_range(target_id, monster->get_stats().attack_range);
        }

        bool is_target_visible(BTContext& context)
        {
            // 타겟이 보이는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            uint32_t target_id = monster->get_target_id();
            return monster->can_see_target(target_id);
        }

        bool is_health_low(BTContext& context)
        {
            // 체력이 낮은지 확인 (30% 이하)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            const auto& stats = monster->get_stats();
            return stats.health <= (stats.max_health * 0.3f);
        }

        bool is_health_critical(BTContext& context)
        {
            // 체력이 위험한지 확인 (10% 이하)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            const auto& stats = monster->get_stats();
            return stats.health <= (stats.max_health * 0.1f);
        }

        bool is_mana_low(BTContext& context)
        {
            // 마나가 낮은지 확인 (20% 이하)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            const auto& stats = monster->get_stats();
            return stats.mana <= (stats.max_mana * 0.2f);
        }

        bool is_in_combat(BTContext& context)
        {
            // 전투 중인지 확인 (타겟이 있고 공격 범위 내에 있음)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_target() && monster->has_enemy_in_attack_range();
        }

        bool is_dead(BTContext& context)
        {
            // 사망했는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return !monster->is_alive();
        }

        bool can_see_enemy(BTContext& context)
        {
            // 적을 볼 수 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            // 가장 가까운 적이 탐지 범위 내에 있고 시야가 확보되어 있는지 확인
            return monster->has_enemy_in_detection_range() && monster->get_environment_info().has_line_of_sight;
        }

        bool is_enemy_in_detection_range(BTContext& context)
        {
            // 적이 탐지 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_enemy_in_detection_range();
        }

        bool is_enemy_in_attack_range(BTContext& context)
        {
            // 적이 공격 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_enemy_in_attack_range();
        }

        bool is_enemy_in_chase_range(BTContext& context)
        {
            // 적이 추적 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_enemy_in_chase_range();
        }

        bool has_line_of_sight(BTContext& context)
        {
            // 시야가 확보되어 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->get_environment_info().has_line_of_sight;
        }

        bool is_path_clear(BTContext& context)
        {
            // 경로가 막혀있지 않은지 확인 (간단한 구현)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return true; // 기본적으로 경로가 막혀있지 않음

            auto monster = monster_ai->get_monster();
            if (!monster)
                return true;

            // TODO: 실제 경로 탐색 구현
            return true;
        }

        bool has_enemy_target(BTContext& context)
        {
            // 적 타겟이 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            return monster->has_target() && monster->get_nearest_enemy_id() != 0;
        }

        bool is_target_alive(BTContext& context)
        {
            // 타겟이 살아있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            // TODO: 실제 타겟 생존 여부 확인 (PlayerManager를 통해)
            return true; // 임시로 항상 살아있다고 가정
        }

        bool is_target_in_attack_range(BTContext& context)
        {
            // 타겟이 공격 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            uint32_t target_id = monster->get_target_id();
            return monster->is_target_in_range(target_id, monster->get_stats().attack_range);
        }

        bool is_target_in_detection_range(BTContext& context)
        {
            // 타겟이 탐지 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            uint32_t target_id = monster->get_target_id();
            return monster->is_target_in_range(target_id, monster->get_stats().detection_range);
        }

        bool is_target_in_chase_range(BTContext& context)
        {
            // 타겟이 추적 범위 내에 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            uint32_t target_id = monster->get_target_id();
            return monster->is_target_in_range(target_id, monster->get_chase_range());
        }

        bool should_return_to_patrol(BTContext& context)
        {
            // 순찰로 돌아가야 하는지 확인 (타겟이 없거나 너무 멀리 떨어져 있을 때)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return true;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return true;

            // 타겟이 없거나 추적 범위를 벗어났을 때
            return !monster->has_target() || !monster->has_enemy_in_chase_range();
        }

        bool can_cast_spell(BTContext& context)
        {
            // 스킬을 사용할 수 있는지 확인 (마나가 충분하고 타겟이 있을 때)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            const auto& stats = monster->get_stats();
            return stats.mana >= 10 && monster->has_target(); // 최소 마나 10 필요
        }

        bool can_summon_minion(BTContext& context)
        {
            // 미니언을 소환할 수 있는지 확인 (마나가 충분하고 특정 조건 만족)
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster)
                return false;

            const auto& stats = monster->get_stats();
            return stats.mana >= 50 && monster->get_type() == MonsterType::DRAGON; // 드래곤만 소환 가능
        }

        bool has_usable_item(BTContext& context)
        {
            // 사용 가능한 아이템이 있는지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            // TODO: 실제 아이템 시스템 구현
            return false; // 현재는 아이템 시스템이 없음
        }

        bool is_target_stronger(BTContext& context)
        {
            // 타겟이 더 강한지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            auto monster = monster_ai->get_monster();
            if (!monster || !monster->has_target())
                return false;

            // TODO: 실제 타겟 강도 비교 구현
            return false; // 임시로 항상 약하다고 가정
        }

        bool is_night_time(BTContext& context)
        {
            // 밤 시간인지 확인
            auto monster_ai = context.get_monster_ai();
            if (!monster_ai)
                return false;

            // TODO: 실제 시간 시스템 구현
            auto now    = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm     = *std::localtime(&time_t);

            // 18시부터 6시까지를 밤으로 간주
            return tm.tm_hour >= 18 || tm.tm_hour < 6;
        }

    } // namespace MonsterConditions

    // MonsterFactory 클래스 구현
    std::shared_ptr<Monster> MonsterFactory::create_monster(const MonsterSpawnConfig& config)
    {
        auto monster = std::make_shared<Monster>(config.name, config.type, config.position);
        monster->set_position(config.position.x, config.position.y, config.position.z, config.position.rotation);
        monster->heal(config.health);                                          // 체력 설정
        monster->take_damage(monster->get_stats().max_health - config.health); // 현재 체력 설정
        // TODO: 데미지, 이동 속도 등 다른 스탯 설정
        monster->set_patrol_points(config.patrol_points);
        // TODO: detection_range, attack_range, chase_range 설정

        return monster;
    }

    std::string MonsterFactory::monster_type_to_string(MonsterType type)
    {
        switch (type)
        {
            case MonsterType::GOBLIN:
                return "goblin";
            case MonsterType::ORC:
                return "orc";
            case MonsterType::DRAGON:
                return "dragon";
            case MonsterType::SKELETON:
                return "skeleton";
            case MonsterType::ZOMBIE:
                return "zombie";
            case MonsterType::NPC_MERCHANT:
                return "merchant";
            case MonsterType::NPC_GUARD:
                return "guard";
            default:
                return "unknown";
        }
    }

    MonsterType MonsterFactory::string_to_monster_type(const std::string& str)
    {
        if (str == "goblin")
            return MonsterType::GOBLIN;
        if (str == "orc")
            return MonsterType::ORC;
        if (str == "dragon")
            return MonsterType::DRAGON;
        if (str == "skeleton")
            return MonsterType::SKELETON;
        if (str == "zombie")
            return MonsterType::ZOMBIE;
        if (str == "merchant")
            return MonsterType::NPC_MERCHANT;
        if (str == "guard")
            return MonsterType::NPC_GUARD;
        return MonsterType::GOBLIN; // 기본값
    }

} // namespace bt
