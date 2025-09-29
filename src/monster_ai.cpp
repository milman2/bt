#include "monster_ai.h"
#include "simple_websocket_server.h"
#include "player.h"
#include <iostream>
#include <random>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace bt {

// Monster 구현
Monster::Monster(const std::string& name, MonsterType type, const MonsterPosition& position)
    : id_(0), name_(name), type_(type), state_(MonsterState::IDLE), position_(position), target_id_(0),
      ai_name_(""), bt_name_(""), current_patrol_index_(0), spawn_position_(position) {
    
    // 기본 통계 설정
    stats_ = MonsterFactory::get_default_stats(type);
    last_update_time_ = std::chrono::steady_clock::now();
    
    // 기본 순찰점 설정 (스폰 위치 주변)
    set_default_patrol_points();
    
    std::cout << "몬스터 생성: " << name_ << " (타입: " << static_cast<int>(type_) << ")" << std::endl;
}

Monster::~Monster() {
    std::cout << "몬스터 소멸: " << name_ << std::endl;
}

void Monster::set_position(float x, float y, float z, float rotation) {
    position_.x = x;
    position_.y = y;
    position_.z = z;
    position_.rotation = rotation;
}

void Monster::move_to(float x, float y, float z, float rotation) {
    set_position(x, y, z, rotation);
    std::cout << "몬스터 " << name_ << " 이동: (" << x << ", " << y << ", " << z << ")" << std::endl;
}

void Monster::take_damage(uint32_t damage) {
    if (damage >= stats_.health) {
        stats_.health = 0;
        state_ = MonsterState::DEAD;
        death_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;
        std::cout << "몬스터 " << name_ << " 사망!" << std::endl;
    } else {
        stats_.health -= damage;
        std::cout << "몬스터 " << name_ << " 데미지 받음: " << damage 
                  << " (남은 체력: " << stats_.health << ")" << std::endl;
    }
}

void Monster::heal(uint32_t amount) {
    if (state_ == MonsterState::DEAD) {
        return;
    }
    
    uint32_t old_health = stats_.health;
    stats_.health += amount;
    
    if (stats_.health > stats_.max_health) {
        stats_.health = stats_.max_health;
    }
    
    uint32_t actual_heal = stats_.health - old_health;
    if (actual_heal > 0) {
        std::cout << "몬스터 " << name_ << " 치료됨: " << actual_heal 
                  << " (현재 체력: " << stats_.health << ")" << std::endl;
    }
}

void Monster::update_environment_info(const std::vector<std::shared_ptr<Player>>& players, 
                                     const std::vector<std::shared_ptr<Monster>>& monsters) {
    environment_info_ = EnvironmentInfo();
    
    // 주변 플레이어 검색
    for (const auto& player : players) {
        if (!player || !player->is_alive()) continue;
        
        const auto& player_pos = player->get_position();
        float distance = std::sqrt(
            std::pow(player_pos.x - position_.x, 2) +
            std::pow(player_pos.y - position_.y, 2) +
            std::pow(player_pos.z - position_.z, 2)
        );
        
        if (distance <= stats_.detection_range) {
            environment_info_.nearby_players.push_back(player->get_id());
            
            if (environment_info_.nearest_enemy_distance < 0 || distance < environment_info_.nearest_enemy_distance) {
                environment_info_.nearest_enemy_distance = distance;
                environment_info_.nearest_enemy_id = player->get_id();
            }
        }
    }
    
    // 주변 몬스터 검색
    for (const auto& monster : monsters) {
        if (!monster || monster.get() == this || !monster->is_alive()) continue;
        
        const auto& monster_pos = monster->get_position();
        float distance = std::sqrt(
            std::pow(monster_pos.x - position_.x, 2) +
            std::pow(monster_pos.y - position_.y, 2) +
            std::pow(monster_pos.z - position_.z, 2)
        );
        
        if (distance <= stats_.detection_range) {
            environment_info_.nearby_monsters.push_back(monster->get_target_id());
        }
    }
    
    // 시야 확보 여부 (간단한 구현)
    environment_info_.has_line_of_sight = true; // TODO: 실제 시야 계산 구현
}

void Monster::update(float delta_time) {
    static std::map<std::string, int> update_counts;
    update_counts[name_]++;
    
    if (update_counts[name_] % 100 == 0) { // 10초마다 로그 출력
        std::cout << "Monster::update 호출됨: " << name_ << " (카운트: " << update_counts[name_] << ")" << std::endl;
    }
    
    // AI 업데이트
    if (ai_) {
        ai_->update(delta_time);
    } else {
        if (update_counts[name_] % 100 == 0) {
            std::cout << "Monster::update - AI가 없음: " << name_ << std::endl;
        }
    }
    
    last_update_time_ = std::chrono::steady_clock::now();
}

void Monster::set_default_patrol_points() {
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

void Monster::set_patrol_points(const std::vector<MonsterPosition>& points) {
    patrol_points_ = points;
    current_patrol_index_ = 0;
}

void Monster::add_patrol_point(const MonsterPosition& point) {
    patrol_points_.push_back(point);
}

MonsterPosition Monster::get_next_patrol_point() {
    if (patrol_points_.empty()) {
        return spawn_position_;
    }
    
    MonsterPosition point = patrol_points_[current_patrol_index_];
    current_patrol_index_ = (current_patrol_index_ + 1) % patrol_points_.size();
    return point;
}

// MonsterFactory 구현
std::shared_ptr<Monster> MonsterFactory::create_monster(MonsterType type, const std::string& name, 
                                                        const MonsterPosition& position) {
    auto monster = std::make_shared<Monster>(name, type, position);
    
    // 몬스터 타입에 따른 AI 생성 및 연결
    std::string bt_name = get_bt_name(type);
    auto ai = std::make_shared<MonsterAI>(name, bt_name);
    monster->set_ai(ai);
    ai->set_monster(monster); // AI에서 몬스터 참조 설정
    
    std::cout << "몬스터 AI 연결: " << name << " -> " << bt_name << std::endl;
    
    return monster;
}

MonsterStats MonsterFactory::get_default_stats(MonsterType type) {
    MonsterStats stats;
    
    switch (type) {
        case MonsterType::GOBLIN:
            stats.level = 1;
            stats.health = 50;
            stats.max_health = 50;
            stats.mana = 20;
            stats.max_mana = 20;
            stats.attack_power = 8;
            stats.defense = 3;
            stats.move_speed = 1.2f;
            stats.attack_range = 1.5f;
            stats.detection_range = 8.0f;
            break;
            
        case MonsterType::ORC:
            stats.level = 3;
            stats.health = 120;
            stats.max_health = 120;
            stats.mana = 40;
            stats.max_mana = 40;
            stats.attack_power = 15;
            stats.defense = 8;
            stats.move_speed = 1.0f;
            stats.attack_range = 2.0f;
            stats.detection_range = 10.0f;
            break;
            
        case MonsterType::DRAGON:
            stats.level = 20;
            stats.health = 2000;
            stats.max_health = 2000;
            stats.mana = 500;
            stats.max_mana = 500;
            stats.attack_power = 100;
            stats.defense = 50;
            stats.move_speed = 2.0f;
            stats.attack_range = 5.0f;
            stats.detection_range = 20.0f;
            break;
            
        case MonsterType::SKELETON:
            stats.level = 2;
            stats.health = 80;
            stats.max_health = 80;
            stats.mana = 30;
            stats.max_mana = 30;
            stats.attack_power = 12;
            stats.defense = 5;
            stats.move_speed = 0.8f;
            stats.attack_range = 1.8f;
            stats.detection_range = 9.0f;
            break;
            
        case MonsterType::ZOMBIE:
            stats.level = 1;
            stats.health = 100;
            stats.max_health = 100;
            stats.mana = 10;
            stats.max_mana = 10;
            stats.attack_power = 10;
            stats.defense = 4;
            stats.move_speed = 0.5f;
            stats.attack_range = 1.2f;
            stats.detection_range = 6.0f;
            break;
            
        case MonsterType::NPC_MERCHANT:
            stats.level = 1;
            stats.health = 50;
            stats.max_health = 50;
            stats.mana = 100;
            stats.max_mana = 100;
            stats.attack_power = 5;
            stats.defense = 2;
            stats.move_speed = 1.0f;
            stats.attack_range = 0.0f;
            stats.detection_range = 5.0f;
            break;
            
        case MonsterType::NPC_GUARD:
            stats.level = 5;
            stats.health = 200;
            stats.max_health = 200;
            stats.mana = 80;
            stats.max_mana = 80;
            stats.attack_power = 25;
            stats.defense = 15;
            stats.move_speed = 1.5f;
            stats.attack_range = 3.0f;
            stats.detection_range = 15.0f;
            break;
    }
    
    return stats;
}

std::string MonsterFactory::get_bt_name(MonsterType type) {
    switch (type) {
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
MonsterManager::MonsterManager() : next_monster_id_(1), auto_spawn_enabled_(false) {
}

MonsterManager::~MonsterManager() {
}

void MonsterManager::add_monster(std::shared_ptr<Monster> monster) {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    uint32_t id = next_monster_id_.fetch_add(1);
    monster->set_id(id);  // 몬스터 객체에 ID 설정
    monsters_[id] = monster;
    std::cout << "몬스터 추가: " << monster->get_name() << " (ID: " << id << ")" << std::endl;
}

void MonsterManager::remove_monster(uint32_t monster_id) {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    auto it = monsters_.find(monster_id);
    if (it != monsters_.end()) {
        std::cout << "몬스터 제거: " << it->second->get_name() << " (ID: " << monster_id << ")" << std::endl;
        monsters_.erase(it);
    }
}

std::shared_ptr<Monster> MonsterManager::get_monster(uint32_t monster_id) {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    auto it = monsters_.find(monster_id);
    if (it != monsters_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Monster>> MonsterManager::get_all_monsters() {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    std::vector<std::shared_ptr<Monster>> result;
    for (const auto& [id, monster] : monsters_) {
        result.push_back(monster);
    }
    return result;
}

std::vector<std::shared_ptr<Monster>> MonsterManager::get_monsters_in_range(const MonsterPosition& position, float range) {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    std::vector<std::shared_ptr<Monster>> result;
    
    for (const auto& [id, monster] : monsters_) {
        const auto& monster_pos = monster->get_position();
        float distance = std::sqrt(
            std::pow(monster_pos.x - position.x, 2) +
            std::pow(monster_pos.y - position.y, 2) +
            std::pow(monster_pos.z - position.z, 2)
        );
        
        if (distance <= range) {
            result.push_back(monster);
        }
    }
    
    return result;
}

std::shared_ptr<Monster> MonsterManager::spawn_monster(MonsterType type, const std::string& name, 
                                                      const MonsterPosition& position) {
    auto monster = MonsterFactory::create_monster(type, name, position);
    add_monster(monster);
    
    // AI 이름과 BT 이름 설정
    monster->set_ai_name(name);
    std::string bt_name = MonsterFactory::get_bt_name(type);
    monster->set_bt_name(bt_name);
    
    // Behavior Tree 엔진에 AI 등록
    if (bt_engine_ && monster->get_ai()) {
        bt_engine_->register_monster_ai(monster->get_ai());
        
        // Behavior Tree 설정
        auto tree = bt_engine_->get_tree(bt_name);
        if (tree) {
            monster->get_ai()->set_behavior_tree(tree);
            std::cout << "몬스터 AI Behavior Tree 설정: " << name << " -> " << bt_name << std::endl;
        } else {
            std::cout << "몬스터 AI Behavior Tree 설정 실패: " << name << " -> " << bt_name << " (트리를 찾을 수 없음)" << std::endl;
        }
    }
    
    return monster;
}

void MonsterManager::add_spawn_config(const MonsterSpawnConfig& config) {
    std::lock_guard<std::mutex> lock(spawn_mutex_);
    spawn_configs_.push_back(config);
    std::cout << "몬스터 스폰 설정 추가: " << config.name << " (타입: " << static_cast<int>(config.type) << ")" << std::endl;
}

void MonsterManager::remove_spawn_config(MonsterType type, const std::string& name) {
    std::lock_guard<std::mutex> lock(spawn_mutex_);
    spawn_configs_.erase(
        std::remove_if(spawn_configs_.begin(), spawn_configs_.end(),
            [&](const MonsterSpawnConfig& config) {
                return config.type == type && config.name == name;
            }),
        spawn_configs_.end()
    );
}

void MonsterManager::start_auto_spawn() {
    auto_spawn_enabled_.store(true);
    std::cout << "몬스터 자동 스폰 시작" << std::endl;
}

void MonsterManager::stop_auto_spawn() {
    auto_spawn_enabled_.store(false);
    std::cout << "몬스터 자동 스폰 중지" << std::endl;
}

void MonsterManager::clear_all_spawn_configs() {
    std::lock_guard<std::mutex> lock(spawn_mutex_);
    spawn_configs_.clear();
    last_spawn_times_.clear();
    std::cout << "모든 몬스터 스폰 설정이 초기화되었습니다." << std::endl;
}

void MonsterManager::set_bt_engine(std::shared_ptr<BehaviorTreeEngine> engine) {
    bt_engine_ = engine;
    std::cout << "MonsterManager에 Behavior Tree 엔진 설정 완료" << std::endl;
}

void MonsterManager::set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server) {
    websocket_server_ = server;
    std::cout << "MonsterManager에 WebSocket 서버 설정 완료" << std::endl;
}

void MonsterManager::set_player_manager(std::shared_ptr<PlayerManager> manager) {
    player_manager_ = manager;
    std::cout << "MonsterManager에 PlayerManager 설정 완료" << std::endl;
}

bool MonsterManager::load_spawn_configs_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "설정 파일을 열 수 없습니다: " << file_path << std::endl;
        return false;
    }
    
    // 기존 설정 초기화
    clear_all_spawn_configs();
    
    try {
        nlohmann::json json_data;
        file >> json_data;
        file.close();
        
        if (!json_data.contains("monster_spawns") || !json_data["monster_spawns"].is_array()) {
            std::cerr << "monster_spawns 배열을 찾을 수 없습니다." << std::endl;
            return false;
        }
        
        int config_count = 0;
        for (const auto& monster_data : json_data["monster_spawns"]) {
            MonsterSpawnConfig config;
            
            // 기본값 설정
            config.respawn_time = 30.0f;
            config.max_count = 1;
            config.spawn_radius = 5.0f;
            config.auto_spawn = true;
            config.position = {0.0f, 0.0f, 0.0f, 0.0f};
            config.detection_range = 15.0f;
            config.attack_range = 3.0f;
            config.chase_range = 25.0f;
            config.health = 100;
            config.damage = 10;
            config.move_speed = 2.0f;
            
            // 타입 파싱
            if (monster_data.contains("type") && monster_data["type"].is_string()) {
                config.type = MonsterFactory::string_to_monster_type(monster_data["type"]);
            }
            
            // 이름 파싱
            if (monster_data.contains("name") && monster_data["name"].is_string()) {
                config.name = monster_data["name"];
            }
            
            // 위치 파싱
            if (monster_data.contains("position") && monster_data["position"].is_object()) {
                const auto& pos = monster_data["position"];
                config.position.x = pos.value("x", 0.0f);
                config.position.y = pos.value("y", 0.0f);
                config.position.z = pos.value("z", 0.0f);
                config.position.rotation = pos.value("rotation", 0.0f);
            }
            
            // 기타 설정들 파싱
            config.respawn_time = monster_data.value("respawn_time", 30.0f);
            config.max_count = monster_data.value("max_count", 1);
            config.spawn_radius = monster_data.value("spawn_radius", 5.0f);
            config.auto_spawn = monster_data.value("auto_spawn", true);
            config.detection_range = monster_data.value("detection_range", 15.0f);
            config.attack_range = monster_data.value("attack_range", 3.0f);
            config.chase_range = monster_data.value("chase_range", 25.0f);
            config.health = monster_data.value("health", 100);
            config.damage = monster_data.value("damage", 10);
            config.move_speed = monster_data.value("move_speed", 2.0f);
            
            // 순찰점 파싱
            if (monster_data.contains("patrol_points") && monster_data["patrol_points"].is_array()) {
                config.patrol_points.clear();
                for (const auto& point : monster_data["patrol_points"]) {
                    MonsterPosition patrol_pos;
                    patrol_pos.x = point.value("x", 0.0f);
                    patrol_pos.y = point.value("y", 0.0f);
                    patrol_pos.z = point.value("z", 0.0f);
                    patrol_pos.rotation = point.value("rotation", 0.0f);
                    config.patrol_points.push_back(patrol_pos);
                }
            }
            
            add_spawn_config(config);
            config_count++;
        }
        
        std::cout << "설정 파일에서 " << config_count << "개의 몬스터 스폰 설정을 로드했습니다." << std::endl;
        return config_count > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
        return false;
    }
}


void MonsterManager::process_auto_spawn(float delta_time) {
    if (!auto_spawn_enabled_.load()) {
        static int debug_count = 0;
        if (debug_count++ % 100 == 0) {
            std::cout << "자동 스폰이 비활성화되어 있습니다." << std::endl;
        }
        return;
    }
    
    std::lock_guard<std::mutex> lock(spawn_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    static int debug_count = 0;
    if (debug_count++ % 100 == 0) {
        std::cout << "자동 스폰 처리 중... 설정된 스폰 설정 수: " << spawn_configs_.size() << std::endl;
    }
    
    // 10초마다 스폰 설정 수 로그 출력 (성능 개선)
    if (debug_count % 100 == 0) {
        std::cout << "현재 스폰 설정 수: " << spawn_configs_.size() << std::endl;
    }
    for (const auto& config : spawn_configs_) {
        if (!config.auto_spawn) {
            if (debug_count % 100 == 0) {
                std::cout << "스폰 설정 " << config.name << "은 자동 스폰이 비활성화되어 있습니다." << std::endl;
            }
            continue;
        }
        
        std::string config_key = config.name + "_" + std::to_string(static_cast<int>(config.type));
        
        // 현재 이름의 몬스터 수 확인
        size_t current_count = get_monster_count_by_name(config.name);
        // 10초마다 스폰 상태 로그 출력 (성능 개선)
        if (debug_count % 100 == 0) {
            std::cout << "스폰 설정 " << config.name << " - 현재 몬스터 수: " << current_count << ", 최대 수: " << config.max_count << std::endl;
        }
        if (current_count >= config.max_count) {
            if (debug_count % 100 == 0) {
                std::cout << "스폰 설정 " << config.name << " - 최대 수에 도달했습니다." << std::endl;
            }
            continue;
        }
        
        // 마지막 스폰 시간 확인
        auto last_spawn_it = last_spawn_times_.find(config_key);
        if (last_spawn_it != last_spawn_times_.end()) {
            auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(now - last_spawn_it->second).count();
            if (time_since_last < config.respawn_time) continue;
        }
        
        // 몬스터 스폰
        MonsterPosition spawn_pos = config.position;
        // 스폰 반경 내에서 랜덤 위치 조정
        if (config.spawn_radius > 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(-config.spawn_radius, config.spawn_radius);
            spawn_pos.x += dis(gen);
            spawn_pos.z += dis(gen);
        }
        
        auto monster = spawn_monster(config.type, config.name, spawn_pos);
        if (monster) {
            last_spawn_times_[config_key] = now;
            std::cout << "자동 스폰: " << config.name << " at (" << spawn_pos.x << ", " << spawn_pos.y << ", " << spawn_pos.z << ")" << std::endl;
        }
    }
}

void MonsterManager::process_respawn(float delta_time) {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    std::vector<uint32_t> dead_monsters;
    for (const auto& [id, monster] : monsters_) {
        if (!monster->is_alive()) {
            dead_monsters.push_back(id);
        }
    }
    
    // 죽은 몬스터 제거
    for (uint32_t id : dead_monsters) {
        monsters_.erase(id);
        std::cout << "죽은 몬스터 제거: ID " << id << std::endl;
    }
}

void MonsterManager::update(float delta_time) {
    static int update_count = 0;
    update_count++;
    
    // 10초마다 로그 출력 (성능 개선)
    if (update_count % 100 == 0) {
        std::cout << "MonsterManager::update 호출됨 (카운트: " << update_count << ", 몬스터 수: " << monsters_.size() << ")" << std::endl;
    }
    
    process_auto_spawn(delta_time);
    process_respawn(delta_time);
    
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    for (const auto& [id, monster] : monsters_) {
        monster->update(delta_time);
    }
    
    // 주기적으로 WebSocket으로 몬스터 상태 업데이트 브로드캐스트
    static float broadcast_timer = 0.0f;
    broadcast_timer += delta_time;
    if (broadcast_timer >= 1.0f) { // 1초마다 브로드캐스트
        broadcast_timer = 0.0f;
        
        if (websocket_server_) {
            nlohmann::json event;
            event["type"] = "monster_update";
            event["monsters"] = nlohmann::json::array();
            
            for (const auto& [id, monster] : monsters_) {
                if (monster) {
                    nlohmann::json monster_data;
                    monster_data["id"] = monster->get_id();
                    monster_data["name"] = monster->get_name();
                    monster_data["type"] = MonsterFactory::monster_type_to_string(monster->get_type());
                    monster_data["position"] = {
                        {"x", monster->get_position().x},
                        {"y", monster->get_position().y},
                        {"z", monster->get_position().z},
                        {"rotation", monster->get_position().rotation}
                    };
                    monster_data["health"] = monster->get_stats().health;
                    monster_data["max_health"] = monster->get_stats().max_health;
                    monster_data["level"] = monster->get_stats().level;
                    monster_data["ai_name"] = monster->get_ai_name();
                    monster_data["bt_name"] = monster->get_bt_name();
                    event["monsters"].push_back(monster_data);
                }
            }
            
            std::cout << "WebSocket 브로드캐스트: " << event["monsters"].size() << "마리 몬스터 전송" << std::endl;
            websocket_server_->broadcast(event.dump());
            
            // 서버 통계도 함께 브로드캐스트
            nlohmann::json stats_event;
            stats_event["type"] = "server_stats";
            stats_event["data"]["totalMonsters"] = monsters_.size();
            stats_event["data"]["activeMonsters"] = monsters_.size();
            // PlayerManager에서 실제 플레이어 수 가져오기
            if (player_manager_) {
                stats_event["data"]["totalPlayers"] = player_manager_->get_player_count();
            } else {
                stats_event["data"]["totalPlayers"] = 0;
            }
            stats_event["data"]["registeredBTTrees"] = 7; // 등록된 BT 트리 수
            stats_event["data"]["connectedClients"] = websocket_server_->get_connected_clients();
            websocket_server_->broadcast(stats_event.dump());
        }
    }
}

size_t MonsterManager::get_monster_count_by_type(MonsterType type) const {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    size_t count = 0;
    for (const auto& [id, monster] : monsters_) {
        if (monster->get_type() == type) {
            count++;
        }
    }
    return count;
}

size_t MonsterManager::get_monster_count_by_name(const std::string& name) const {
    std::lock_guard<std::mutex> lock(monsters_mutex_);
    size_t count = 0;
    for (const auto& [id, monster] : monsters_) {
        if (monster->get_name() == name) {
            count++;
        }
    }
    return count;
}

// PlayerManager 구현
PlayerManager::PlayerManager() : next_player_id_(1) {
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::add_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    players_[player->get_id()] = player;
    std::cout << "플레이어 추가: " << player->get_name() << " (ID: " << player->get_id() << ")" << std::endl;
}

void PlayerManager::remove_player(uint32_t player_id) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    auto it = players_.find(player_id);
    if (it != players_.end()) {
        std::cout << "플레이어 제거: " << it->second->get_name() << " (ID: " << player_id << ")" << std::endl;
        players_.erase(it);
    }
}

std::shared_ptr<Player> PlayerManager::get_player(uint32_t player_id) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    auto it = players_.find(player_id);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Player>> PlayerManager::get_all_players() {
    std::lock_guard<std::mutex> lock(players_mutex_);
    std::vector<std::shared_ptr<Player>> result;
    for (const auto& [id, player] : players_) {
        result.push_back(player);
    }
    return result;
}

std::vector<std::shared_ptr<Player>> PlayerManager::get_players_in_range(const MonsterPosition& position, float range) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    std::vector<std::shared_ptr<Player>> result;
    
    for (const auto& [id, player] : players_) {
        if (!player->is_alive()) continue;
        
        const auto& player_pos = player->get_position();
        float distance = std::sqrt(
            std::pow(player_pos.x - position.x, 2) +
            std::pow(player_pos.y - position.y, 2) +
            std::pow(player_pos.z - position.z, 2)
        );
        
        if (distance <= range) {
            result.push_back(player);
        }
    }
    
    return result;
}

std::shared_ptr<Player> PlayerManager::create_player(const std::string& name, const MonsterPosition& position) {
    uint32_t player_id = next_player_id_.fetch_add(1);
    auto player = std::make_shared<Player>(player_id, name);
    
    // 위치 설정
    player->set_position(position.x, position.y, position.z, position.rotation);
    
    // 기본 스탯 설정
    PlayerStats stats;
    stats.level = 1;
    stats.health = 100;
    stats.max_health = 100;
    stats.mana = 50;
    stats.max_mana = 50;
    stats.strength = 15;
    stats.agility = 10;
    stats.intelligence = 10;
    stats.vitality = 10;
    player->set_stats(stats);
    
    // 플레이어 상태를 ONLINE으로 설정
    player->set_state(PlayerState::ONLINE);
    
    add_player(player);
    return player;
}

std::shared_ptr<Player> PlayerManager::create_player_for_client(uint32_t client_id, const std::string& name, const MonsterPosition& position) {
    uint32_t player_id = next_player_id_.fetch_add(1);
    auto player = std::make_shared<Player>(player_id, name);
    
    // 위치 설정
    player->set_position(position.x, position.y, position.z, position.rotation);
    
    // 기본 스탯 설정
    PlayerStats stats;
    stats.level = 1;
    stats.health = 100;
    stats.max_health = 100;
    stats.mana = 50;
    stats.max_mana = 50;
    stats.strength = 15;
    stats.agility = 10;
    stats.intelligence = 10;
    stats.vitality = 10;
    player->set_stats(stats);
    
    // 플레이어 상태를 ONLINE으로 설정
    player->set_state(PlayerState::ONLINE);
    
    // 클라이언트 ID와 플레이어 ID 매핑
    {
        std::lock_guard<std::mutex> lock(players_mutex_);
        client_to_player_id_[client_id] = player_id;
        player_to_client_id_[player_id] = client_id;
        players_[player_id] = player;
    }
    
    std::cout << "클라이언트 ID " << client_id << "에 대한 플레이어 생성: " << name << " (플레이어 ID: " << player_id << ")" << std::endl;
    return player;
}

void PlayerManager::remove_player_by_client_id(uint32_t client_id) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    auto client_it = client_to_player_id_.find(client_id);
    if (client_it != client_to_player_id_.end()) {
        uint32_t player_id = client_it->second;
        
        // 플레이어 제거
        auto player_it = players_.find(player_id);
        if (player_it != players_.end()) {
            std::cout << "클라이언트 ID " << client_id << "의 플레이어 제거: " << player_it->second->get_name() << " (플레이어 ID: " << player_id << ")" << std::endl;
            players_.erase(player_it);
        }
        
        // 매핑 제거
        client_to_player_id_.erase(client_it);
        player_to_client_id_.erase(player_id);
    }
}

std::shared_ptr<Player> PlayerManager::get_player_by_client_id(uint32_t client_id) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    auto client_it = client_to_player_id_.find(client_id);
    if (client_it != client_to_player_id_.end()) {
        uint32_t player_id = client_it->second;
        auto player_it = players_.find(player_id);
        if (player_it != players_.end()) {
            return player_it->second;
        }
    }
    
    return nullptr;
}

uint32_t PlayerManager::get_client_id_by_player_id(uint32_t player_id) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    auto player_it = player_to_client_id_.find(player_id);
    if (player_it != player_to_client_id_.end()) {
        return player_it->second;
    }
    
    return 0; // 0은 유효하지 않은 클라이언트 ID
}

void PlayerManager::process_combat(float delta_time) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    for (const auto& [id, player] : players_) {
        if (!player->is_alive()) {
            // 플레이어가 사망한 경우 처리 (필요시 리스폰 로직 추가)
            std::cout << "플레이어 " << player->get_name() << " 사망!" << std::endl;
        }
    }
}

void PlayerManager::attack_player(uint32_t attacker_id, uint32_t target_id, uint32_t damage) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    auto target = get_player(target_id);
    if (!target || !target->is_alive()) return;
    
    target->take_damage(damage);
    std::cout << "플레이어 " << target->get_name() << " 데미지 받음: " << damage
              << " (남은 체력: " << target->get_stats().health << ")" << std::endl;
}

void PlayerManager::attack_monster(uint32_t attacker_id, uint32_t target_id, uint32_t damage) {
    // 이 함수는 MonsterManager에서 호출되어야 함
    // 여기서는 플레이어의 공격력을 가져오는 역할만 함
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    auto attacker = get_player(attacker_id);
    if (!attacker || !attacker->is_alive()) return;
    
    std::cout << "플레이어 " << attacker->get_name() << " 몬스터 ID " << target_id << " 공격 (데미지: " << damage << ")" << std::endl;
}

void PlayerManager::process_player_respawn(float delta_time) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    for (const auto& [id, player] : players_) {
        if (!player->is_alive()) {
            // 플레이어가 사망한 경우 자동 리스폰 (필요시 시간 체크 로직 추가)
            player->respawn();
            std::cout << "플레이어 " << player->get_name() << " 리스폰 완료!" << std::endl;
        }
    }
}

void PlayerManager::set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server) {
    websocket_server_ = server;
    std::cout << "PlayerManager에 WebSocket 서버 설정 완료" << std::endl;
}

void PlayerManager::update(float delta_time) {
    static int update_count = 0;
    update_count++;
    if (update_count % 100 == 0) { // 10초마다 로그 출력
        std::cout << "PlayerManager::update 호출됨 (카운트: " << update_count << ", 플레이어 수: " << players_.size() << ")" << std::endl;
    }
    
    process_combat(delta_time);
    process_player_ai(delta_time);
    process_player_respawn(delta_time);
    
    std::lock_guard<std::mutex> lock(players_mutex_);
    for (const auto& [id, player] : players_) {
        player->update_activity();
    }
    
    // WebSocket으로 플레이어 정보 브로드캐스트 (1초마다)
    static auto last_broadcast_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_broadcast_time).count() >= 1) {
        if (websocket_server_) {
            nlohmann::json player_event;
            player_event["type"] = "player_update";
            player_event["players"] = nlohmann::json::array();
            
            for (const auto& [id, player] : players_) {
                nlohmann::json player_data;
                player_data["id"] = player->get_id();
                player_data["name"] = player->get_name();
                player_data["state"] = static_cast<int>(player->get_state());
                const auto& pos = player->get_position();
                player_data["position"]["x"] = pos.x;
                player_data["position"]["y"] = pos.y;
                player_data["position"]["z"] = pos.z;
                player_data["position"]["rotation"] = pos.rotation;
                const auto& stats = player->get_stats();
                player_data["stats"]["level"] = stats.level;
                player_data["stats"]["health"] = stats.health;
                player_data["stats"]["max_health"] = stats.max_health;
                player_data["stats"]["mana"] = stats.mana;
                player_data["stats"]["max_mana"] = stats.max_mana;
                player_data["stats"]["experience"] = 0; // 기본값
                player_data["current_map_id"] = player->get_current_map_id();
                player_data["is_alive"] = player->is_alive();
                player_event["players"].push_back(player_data);
            }
            
            std::cout << "WebSocket 브로드캐스트: " << player_event["players"].size() << "명 플레이어 전송" << std::endl;
            websocket_server_->broadcast(player_event.dump());
        }
        last_broadcast_time = now;
    }
}

// 몬스터 AI 액션 함수들
namespace MonsterActions {
    BTNodeStatus attack(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 타겟 ID 가져오기
        uint32_t target_id = 0;
        if (context.has_data("target_id")) {
            target_id = context.get_data_as<uint32_t>("target_id");
        }
        
        if (target_id == 0) {
            return BTNodeStatus::FAILURE;
        }
        
        // 공격 로직 구현
        std::cout << "몬스터 공격 실행 (타겟 ID: " << target_id << ")" << std::endl;
        
        // TODO: 실제 데미지 계산 및 적용
        // PlayerManager나 MonsterManager를 통해 데미지 적용
        
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus move_to_target(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 타겟으로 이동 로직 구현
        std::cout << "몬스터 타겟으로 이동" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus patrol(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        auto monster = monster_ai->get_monster();
        if (!monster) {
            return BTNodeStatus::FAILURE;
        }
        
        // 현재 목표 순찰점 확인
        MonsterPosition target_point;
        if (context.has_data("patrol_target")) {
            target_point = context.get_data_as<MonsterPosition>("patrol_target");
        } else {
            // 새로운 순찰점 설정
            target_point = monster->get_next_patrol_point();
            context.set_data("patrol_target", target_point);
            std::cout << "몬스터 " << monster->get_name() << " 새로운 순찰점 설정: (" 
                      << target_point.x << ", " << target_point.y << ", " << target_point.z << ")" << std::endl;
        }
        
        // 현재 위치와 목표 위치 간의 거리 계산
        const auto& current_pos = monster->get_position();
        float distance = std::sqrt(
            std::pow(target_point.x - current_pos.x, 2) +
            std::pow(target_point.y - current_pos.y, 2) +
            std::pow(target_point.z - current_pos.z, 2)
        );
        
        // 순찰점에 도달했는지 확인 (1.0f 이내)
        if (distance <= 1.0f) {
            // 다음 순찰점으로 이동
            MonsterPosition next_point = monster->get_next_patrol_point();
            context.set_data("patrol_target", next_point);
            std::cout << "몬스터 " << monster->get_name() << " 순찰점 도달, 다음 목표: (" 
                      << next_point.x << ", " << next_point.y << ", " << next_point.z << ")" << std::endl;
            return BTNodeStatus::SUCCESS;
        }
        
        // 순찰점으로 이동
        float move_speed = monster->get_stats().move_speed;
        float delta_time = 0.1f; // 100ms 간격
        
        // 방향 벡터 계산
        float dx = target_point.x - current_pos.x;
        float dz = target_point.z - current_pos.z;
        float length = std::sqrt(dx * dx + dz * dz);
        
        if (length > 0) {
            // 정규화된 방향 벡터
            dx /= length;
            dz /= length;
            
            // 이동 거리 계산
            float move_distance = move_speed * delta_time;
            
            // 새 위치 계산
            float new_x = current_pos.x + dx * move_distance;
            float new_z = current_pos.z + dz * move_distance;
            
            // 몬스터 위치 업데이트
            monster->move_to(new_x, current_pos.y, new_z, current_pos.rotation);
            
            std::cout << "몬스터 " << monster->get_name() << " 순찰 이동: (" 
                      << new_x << ", " << new_z << ") 거리: " << distance << std::endl;
        }
        
        return BTNodeStatus::RUNNING; // 계속 실행 중
    }
    
    BTNodeStatus flee(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 도망 로직 구현
        std::cout << "몬스터 도망" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus idle(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 대기 로직 구현
        std::cout << "몬스터 대기" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus die(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 사망 로직 구현
        std::cout << "몬스터 사망 처리" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus cast_spell(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 마법 시전 로직 구현
        std::cout << "몬스터 마법 시전" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus summon_minion(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 미니언 소환 로직 구현
        std::cout << "몬스터 미니언 소환" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus heal_self(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 자가 치료 로직 구현
        std::cout << "몬스터 자가 치료" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
    
    BTNodeStatus use_item(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) {
            return BTNodeStatus::FAILURE;
        }
        
        // 아이템 사용 로직 구현
        std::cout << "몬스터 아이템 사용" << std::endl;
        return BTNodeStatus::SUCCESS;
    }
}

// 몬스터 AI 조건 함수들
namespace MonsterConditions {
    bool has_target(BTContext& context) {
        // 타겟 존재 여부 확인
        return context.has_data("target_id");
    }
    
    bool is_target_in_range(BTContext& context) {
        // 타겟이 공격 범위 내에 있는지 확인
        return true; // 임시 구현
    }
    
    bool is_target_visible(BTContext& context) {
        // 타겟이 시야에 있는지 확인
        return true; // 임시 구현
    }
    
    bool is_health_low(BTContext& context) {
        // 체력이 낮은지 확인
        return true; // 임시 구현
    }
    
    bool is_health_critical(BTContext& context) {
        // 체력이 위험한지 확인
        return false; // 임시 구현
    }
    
    bool is_mana_low(BTContext& context) {
        // 마나가 부족한지 확인
        return false; // 임시 구현
    }
    
    bool is_in_combat(BTContext& context) {
        // 전투 중인지 확인
        return context.has_data("in_combat");
    }
    
    bool is_dead(BTContext& context) {
        // 사망했는지 확인
        return false; // 임시 구현
    }
    
    bool can_cast_spell(BTContext& context) {
        // 마법을 시전할 수 있는지 확인
        return true; // 임시 구현
    }
    
    bool can_summon_minion(BTContext& context) {
        // 미니언을 소환할 수 있는지 확인
        return false; // 임시 구현
    }
    
    bool has_usable_item(BTContext& context) {
        // 사용 가능한 아이템이 있는지 확인
        return false; // 임시 구현
    }
    
    bool is_target_stronger(BTContext& context) {
        // 타겟이 더 강한지 확인
        return false; // 임시 구현
    }
    
    bool is_night_time(BTContext& context) {
        // 밤 시간인지 확인
        return false; // 임시 구현
    }
    
    // 환경 인지 조건들
    bool can_see_enemy(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) return false;
        
        const auto* env_info = monster_ai->get_context().get_environment_info();
        return env_info && !env_info->nearby_players.empty() && env_info->has_line_of_sight;
    }
    
    bool is_enemy_in_detection_range(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) return false;
        
        const auto* env_info = monster_ai->get_context().get_environment_info();
        return env_info && env_info->nearest_enemy_distance > 0 && env_info->nearest_enemy_distance <= 10.0f;
    }
    
    bool is_enemy_in_attack_range(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) return false;
        
        const auto* env_info = monster_ai->get_context().get_environment_info();
        return env_info && env_info->nearest_enemy_distance > 0 && env_info->nearest_enemy_distance <= 2.0f;
    }
    
    bool has_line_of_sight(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) return false;
        
        const auto* env_info = monster_ai->get_context().get_environment_info();
        return env_info && env_info->has_line_of_sight;
    }
    
    bool is_path_clear(BTContext& context) {
        auto monster_ai = context.get_monster_ai();
        if (!monster_ai) return false;
        
        const auto* env_info = monster_ai->get_context().get_environment_info();
        return env_info && env_info->obstacles.empty();
    }
}

// 몬스터별 Behavior Tree 생성 함수들
namespace MonsterBTs {
    std::shared_ptr<BehaviorTree> create_goblin_bt() {
        auto tree = std::make_shared<BehaviorTree>("goblin_bt");
        
        auto root = std::make_shared<BTSelector>("goblin_root");
        
        // 적 발견 및 공격 시퀀스
        auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
        attack_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
        attack_sequence->add_child(std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
        attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));
        
        // 적 추적 시퀀스
        auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
        chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
        chase_sequence->add_child(std::make_shared<BTCondition>("path_clear", MonsterConditions::is_path_clear));
        chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));
        
        // 적 탐지 시퀀스
        auto detection_sequence = std::make_shared<BTSequence>("detection_sequence");
        detection_sequence->add_child(std::make_shared<BTCondition>("enemy_in_range", MonsterConditions::is_enemy_in_detection_range));
        detection_sequence->add_child(std::make_shared<BTAction>("set_target", MonsterActions::idle)); // 타겟 설정 액션
        
        // 순찰
        auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);
        
        root->add_child(attack_sequence);
        root->add_child(chase_sequence);
        root->add_child(detection_sequence);
        root->add_child(patrol_action);
        
        tree->set_root(root);
        return tree;
    }
    
    std::shared_ptr<BehaviorTree> create_orc_bt() {
        auto tree = std::make_shared<BehaviorTree>("orc_bt");
        
        auto root = std::make_shared<BTSelector>("orc_root");
        
        // 공격 시퀀스
        auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
        attack_sequence->add_child(std::make_shared<BTCondition>("has_target", MonsterConditions::has_target));
        attack_sequence->add_child(std::make_shared<BTCondition>("in_range", MonsterConditions::is_target_in_range));
        attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));
        
        // 추적 시퀀스
        auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
        chase_sequence->add_child(std::make_shared<BTCondition>("has_target", MonsterConditions::has_target));
        chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));
        
        // 순찰
        auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);
        
        root->add_child(attack_sequence);
        root->add_child(chase_sequence);
        root->add_child(patrol_action);
        
        tree->set_root(root);
        return tree;
    }
    
    std::shared_ptr<BehaviorTree> create_dragon_bt() {
        auto tree = std::make_shared<BehaviorTree>("dragon_bt");
        
        auto root = std::make_shared<BTSelector>("dragon_root");
        
        // 강력한 공격 시퀀스
        auto powerful_attack_sequence = std::make_shared<BTSequence>("powerful_attack_sequence");
        powerful_attack_sequence->add_child(std::make_shared<BTCondition>("has_target", MonsterConditions::has_target));
        powerful_attack_sequence->add_child(std::make_shared<BTCondition>("in_range", MonsterConditions::is_target_in_range));
        powerful_attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));
        powerful_attack_sequence->add_child(std::make_shared<BTAction>("cast_spell", MonsterActions::cast_spell));
        
        // 추적 시퀀스
        auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
        chase_sequence->add_child(std::make_shared<BTCondition>("has_target", MonsterConditions::has_target));
        chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));
        
        // 순찰
        auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);
        
        root->add_child(powerful_attack_sequence);
        root->add_child(chase_sequence);
        root->add_child(patrol_action);
        
        tree->set_root(root);
        return tree;
    }
    
    std::shared_ptr<BehaviorTree> create_skeleton_bt() {
        return create_goblin_bt(); // 임시로 고블린과 동일
    }
    
    std::shared_ptr<BehaviorTree> create_zombie_bt() {
        return create_goblin_bt(); // 임시로 고블린과 동일
    }
    
    std::shared_ptr<BehaviorTree> create_merchant_bt() {
        auto tree = std::make_shared<BehaviorTree>("merchant_bt");
        
        auto root = std::make_shared<BTAction>("idle", MonsterActions::idle);
        tree->set_root(root);
        
        return tree;
    }
    
    std::shared_ptr<BehaviorTree> create_guard_bt() {
        auto tree = std::make_shared<BehaviorTree>("guard_bt");
        
        auto root = std::make_shared<BTSelector>("guard_root");
        
        // 공격 시퀀스
        auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
        attack_sequence->add_child(std::make_shared<BTCondition>("has_target", MonsterConditions::has_target));
        attack_sequence->add_child(std::make_shared<BTCondition>("in_range", MonsterConditions::is_target_in_range));
        attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));
        
        // 순찰
        auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);
        
        root->add_child(attack_sequence);
        root->add_child(patrol_action);
        
        tree->set_root(root);
        return tree;
    }
}

// MonsterFactory 문자열 변환 함수들
MonsterType MonsterFactory::string_to_monster_type(const std::string& type_str) {
    if (type_str == "GOBLIN") return MonsterType::GOBLIN;
    if (type_str == "ORC") return MonsterType::ORC;
    if (type_str == "DRAGON") return MonsterType::DRAGON;
    if (type_str == "SKELETON") return MonsterType::SKELETON;
    if (type_str == "ZOMBIE") return MonsterType::ZOMBIE;
    if (type_str == "NPC_MERCHANT") return MonsterType::NPC_MERCHANT;
    if (type_str == "NPC_GUARD") return MonsterType::NPC_GUARD;
    
    std::cerr << "알 수 없는 몬스터 타입: " << type_str << std::endl;
    return MonsterType::GOBLIN; // 기본값
}

std::string MonsterFactory::monster_type_to_string(MonsterType type) {
    switch (type) {
        case MonsterType::GOBLIN: return "GOBLIN";
        case MonsterType::ORC: return "ORC";
        case MonsterType::DRAGON: return "DRAGON";
        case MonsterType::SKELETON: return "SKELETON";
        case MonsterType::ZOMBIE: return "ZOMBIE";
        case MonsterType::NPC_MERCHANT: return "NPC_MERCHANT";
        case MonsterType::NPC_GUARD: return "NPC_GUARD";
        default: return "UNKNOWN";
    }
}

// Monster 클래스의 새로운 메서드들 구현

void Monster::attack_target() {
    if (target_id_ == 0) {
        return;
    }
    
    std::cout << "몬스터 " << name_ << "이 타겟 " << target_id_ << "을 공격합니다! (데미지: " << damage_ << ")" << std::endl;
    // 실제 데미지는 PlayerManager나 MonsterManager에서 처리
}

bool Monster::is_in_attack_range(uint32_t target_id) const {
    // 실제 구현에서는 타겟의 위치를 가져와서 거리 계산
    // 여기서는 간단히 attack_range_와 비교
    return true; // 임시 구현
}

bool Monster::is_in_detection_range(uint32_t target_id) const {
    // 실제 구현에서는 타겟의 위치를 가져와서 거리 계산
    return true; // 임시 구현
}

bool Monster::is_in_chase_range(uint32_t target_id) const {
    // 실제 구현에서는 타겟의 위치를 가져와서 거리 계산
    return true; // 임시 구현
}

float Monster::get_distance_to_target(uint32_t target_id) const {
    // 실제 구현에서는 타겟의 위치를 가져와서 거리 계산
    return 0.0f; // 임시 구현
}

bool Monster::should_respawn(float current_time) const {
    if (state_ != MonsterState::DEAD) {
        return false;
    }
    
    return (current_time - death_time_) >= respawn_time_;
}

// MonsterManager의 새로운 메서드들 구현

void MonsterManager::parse_patrol_points(const std::string& points_content, std::vector<MonsterPosition>& points) {
    // JSON 배열 파싱
    try {
        auto json_points = nlohmann::json::parse(points_content);
        for (const auto& point : json_points) {
            MonsterPosition pos;
            pos.x = point["x"];
            pos.y = point["y"];
            pos.z = point["z"];
            pos.rotation = point.value("rotation", 0.0f);
            points.push_back(pos);
        }
    } catch (const std::exception& e) {
        std::cerr << "순찰점 파싱 오류: " << e.what() << std::endl;
    }
}

MonsterPosition MonsterManager::get_random_respawn_point() const {
    if (player_respawn_points_.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, player_respawn_points_.size() - 1);
    
    return player_respawn_points_[dis(gen)];
}

// PlayerManager의 새로운 메서드들 구현

void PlayerManager::process_player_ai(float delta_time) {
    std::lock_guard<std::mutex> lock(players_mutex_);
    
    float current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;
    
    for (const auto& [id, player] : players_) {
        if (!player->is_alive()) {
            continue;
        }
        
        // 랜덤 이동
        auto last_move_it = player_last_move_time_.find(id);
        if (last_move_it == player_last_move_time_.end() || 
            (current_time - last_move_it->second) >= player_move_interval_) {
            move_player_to_random_location(id);
            player_last_move_time_[id] = current_time;
        }
        
        // 주변 몬스터 공격
        auto last_attack_it = player_last_attack_time_.find(id);
        if (last_attack_it == player_last_attack_time_.end() || 
            (current_time - last_attack_it->second) >= player_attack_interval_) {
            attack_nearby_monster(id);
            player_last_attack_time_[id] = current_time;
        }
    }
}

void PlayerManager::move_player_to_random_location(uint32_t player_id) {
    auto player = get_player(player_id);
    if (!player) {
        return;
    }
    
    // 랜덤 위치 생성 (현재 위치 주변)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-50.0f, 50.0f);
    
    const auto& current_pos = player->get_position();
    float new_x = current_pos.x + dis(gen);
    float new_z = current_pos.z + dis(gen);
    
    player->set_position(new_x, current_pos.y, new_z, current_pos.rotation);
    std::cout << "플레이어 " << player->get_name() << " 랜덤 이동: (" << new_x << ", " << new_z << ")" << std::endl;
}

void PlayerManager::attack_nearby_monster(uint32_t player_id) {
    auto player = get_player(player_id);
    if (!player) {
        return;
    }
    
    // 주변 몬스터 찾기 (간단한 구현)
    // 실제로는 MonsterManager에서 몬스터 정보를 가져와야 함
    std::cout << "플레이어 " << player->get_name() << "이 주변 몬스터를 공격합니다!" << std::endl;
}

void PlayerManager::respawn_player(uint32_t player_id) {
    auto player = get_player(player_id);
    if (!player) {
        return;
    }
    
    // 랜덤 리스폰 포인트에서 부활
    MonsterPosition respawn_pos = get_random_respawn_point();
    player->set_position(respawn_pos.x, respawn_pos.y, respawn_pos.z, respawn_pos.rotation);
    player->set_state(PlayerState::ONLINE);
    
    // 체력 회복
    auto stats = player->get_stats();
    stats.health = stats.max_health;
    player->set_stats(stats);
    
    std::cout << "플레이어 " << player->get_name() << " 리스폰: (" 
              << respawn_pos.x << ", " << respawn_pos.z << ")" << std::endl;
}

void PlayerManager::set_player_respawn_points(const std::vector<MonsterPosition>& points) {
    player_respawn_points_ = points;
}

MonsterPosition PlayerManager::get_random_respawn_point() const {
    if (player_respawn_points_.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, player_respawn_points_.size() - 1);
    
    return player_respawn_points_[dis(gen)];
}

} // namespace bt
