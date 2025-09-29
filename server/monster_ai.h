#pragma once

#include "behavior_tree.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

// 전방 선언
namespace bt {
    class SimpleWebSocketServer;
    class PlayerManager;
    class Player;
}

namespace bt {

// 몬스터 타입 정의
enum class MonsterType {
    GOBLIN,
    ORC,
    DRAGON,
    SKELETON,
    ZOMBIE,
    NPC_MERCHANT,
    NPC_GUARD
};

// 몬스터 상태
enum class MonsterState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK,
    FLEE,
    DEAD
};

// 몬스터 통계
struct MonsterStats {
    uint32_t level = 1;
    uint32_t health = 100;
    uint32_t max_health = 100;
    uint32_t mana = 50;
    uint32_t max_mana = 50;
    uint32_t attack_power = 10;
    uint32_t defense = 5;
    float move_speed = 1.0f;
    float attack_range = 2.0f;
    float detection_range = 10.0f;
};

// 몬스터 위치
struct MonsterPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
};

// 몬스터 스폰 설정
struct MonsterSpawnConfig {
    MonsterType type;
    std::string name;
    MonsterPosition position;
    float respawn_time = 30.0f; // 리스폰 시간 (초)
    uint32_t max_count = 1; // 최대 생성 수
    float spawn_radius = 5.0f; // 스폰 반경
    bool auto_spawn = true; // 자동 스폰 여부
    std::vector<MonsterPosition> patrol_points; // 순찰 경로
    float detection_range = 15.0f; // 탐지 범위
    float attack_range = 3.0f; // 공격 범위
    float chase_range = 25.0f; // 추적 범위
    uint32_t health = 100; // 체력
    uint32_t damage = 10; // 공격력
    float move_speed = 2.0f; // 이동 속도
};


// 환경 인지 정보
struct EnvironmentInfo {
    std::vector<uint32_t> nearby_players; // 주변 플레이어 ID들
    std::vector<uint32_t> nearby_monsters; // 주변 몬스터 ID들
    std::vector<uint32_t> obstacles; // 장애물 ID들
    bool has_line_of_sight = true; // 시야 확보 여부
    float nearest_enemy_distance = -1.0f; // 가장 가까운 적과의 거리
    uint32_t nearest_enemy_id = 0; // 가장 가까운 적 ID
};

// 몬스터 클래스
class Monster {
public:
    Monster(const std::string& name, MonsterType type, const MonsterPosition& position);
    ~Monster();

    // 기본 정보
    uint32_t get_id() const { return id_; }
    void set_id(uint32_t id) { id_ = id; }
    const std::string& get_name() const { return name_; }
    MonsterType get_type() const { return type_; }
    MonsterState get_state() const { return state_; }
    void set_state(MonsterState state) { state_ = state; }

    // 위치 관련
    const MonsterPosition& get_position() const { return position_; }
    void set_position(const MonsterPosition& pos) { position_ = pos; }
    void set_position(float x, float y, float z, float rotation = 0.0f);
    void move_to(float x, float y, float z, float rotation = 0.0f);

    // 통계 관련
    const MonsterStats& get_stats() const { return stats_; }
    void set_stats(const MonsterStats& stats) { stats_ = stats; }
    void take_damage(uint32_t damage);
    void heal(uint32_t amount);
    bool is_alive() const { return stats_.health > 0; }
    uint32_t get_max_health() const { return stats_.max_health; }

    // AI 관련
    std::shared_ptr<MonsterAI> get_ai() const { return ai_; }
    void set_ai(std::shared_ptr<MonsterAI> ai) { ai_ = ai; }
    void set_ai_name(const std::string& ai_name) { ai_name_ = ai_name; }
    const std::string& get_ai_name() const { return ai_name_; }
    void set_bt_name(const std::string& bt_name) { bt_name_ = bt_name; }
    const std::string& get_bt_name() const { return bt_name_; }

    // 타겟 관리
    uint32_t get_target_id() const { return target_id_; }
    void set_target_id(uint32_t id) { target_id_ = id; }
    bool has_target() const { return target_id_ != 0; }
    void clear_target() { target_id_ = 0; }
    
    // 전투 관련
    void attack_target();
    bool is_in_attack_range(uint32_t target_id) const;
    bool is_in_detection_range(uint32_t target_id) const;
    bool is_in_chase_range(uint32_t target_id) const;
    float get_distance_to_target(uint32_t target_id) const;
    
    // 리스폰 관련
    void set_respawn_time(float time) { respawn_time_ = time; }
    float get_respawn_time() const { return respawn_time_; }
    void set_death_time(float time) { death_time_ = time; }
    float get_death_time() const { return death_time_; }
    bool should_respawn(float current_time) const;

    // 환경 인지
    EnvironmentInfo get_environment_info() const { return environment_info_; }
    void update_environment_info(const std::vector<std::shared_ptr<Player>>& players, 
                                const std::vector<std::shared_ptr<Monster>>& monsters);
    
    // 순찰 관련
    void set_patrol_points(const std::vector<MonsterPosition>& points);
    void add_patrol_point(const MonsterPosition& point);
    MonsterPosition get_next_patrol_point();
    bool has_patrol_points() const { return !patrol_points_.empty(); }
    void reset_patrol_index() { current_patrol_index_ = 0; }

    // 업데이트
    void update(float delta_time);

private:
    uint32_t id_;
    std::string name_;
    MonsterType type_;
    MonsterState state_;
    MonsterPosition position_;
    MonsterStats stats_;
    std::shared_ptr<MonsterAI> ai_;
    std::string ai_name_;
    std::string bt_name_;
    uint32_t target_id_;
    std::chrono::steady_clock::time_point last_update_time_;
    EnvironmentInfo environment_info_;
    
    // 순찰 관련
    std::vector<MonsterPosition> patrol_points_;
    size_t current_patrol_index_;
    MonsterPosition spawn_position_; // 스폰 위치 (기본 순찰점)
    
    // 전투 관련
    float detection_range_ = 15.0f;
    float attack_range_ = 3.0f;
    float chase_range_ = 25.0f;
    uint32_t damage_ = 10;
    float move_speed_ = 2.0f;
    
    // 리스폰 관련
    float respawn_time_ = 30.0f;
    float death_time_ = 0.0f;
    
    void set_default_patrol_points();
};

// 몬스터 팩토리
class MonsterFactory {
public:
    static std::shared_ptr<Monster> create_monster(MonsterType type, const std::string& name, 
                                                   const MonsterPosition& position);
    static std::shared_ptr<Monster> create_monster(const MonsterSpawnConfig& config);
    
    // 몬스터별 기본 통계 설정
    static MonsterStats get_default_stats(MonsterType type);
    
    // 몬스터별 Behavior Tree 이름 반환
    static std::string get_bt_name(MonsterType type);
    
    // 문자열을 MonsterType으로 변환
    static MonsterType string_to_monster_type(const std::string& type_str);
    
    // MonsterType을 문자열로 변환
    static std::string monster_type_to_string(MonsterType type);
};

// 몬스터 매니저
class MonsterManager {
public:
    MonsterManager();
    ~MonsterManager();

    // 몬스터 관리
    void add_monster(std::shared_ptr<Monster> monster);
    void remove_monster(uint32_t monster_id);
    std::shared_ptr<Monster> get_monster(uint32_t monster_id);
    std::vector<std::shared_ptr<Monster>> get_all_monsters();
    std::vector<std::shared_ptr<Monster>> get_monsters_in_range(const MonsterPosition& position, float range);

    // 몬스터 생성
    std::shared_ptr<Monster> spawn_monster(MonsterType type, const std::string& name, 
                                          const MonsterPosition& position);

    // 자동 스폰 관리
    void add_spawn_config(const MonsterSpawnConfig& config);
    void remove_spawn_config(MonsterType type, const std::string& name);
    void start_auto_spawn();
    void stop_auto_spawn();
    
    // 설정 파일 관리
    bool load_spawn_configs_from_file(const std::string& file_path);
    void clear_all_spawn_configs();

    // 업데이트
    void update(float delta_time);
    
    // Behavior Tree 엔진 설정
    void set_bt_engine(std::shared_ptr<BehaviorTreeEngine> engine);
    
    // WebSocket 서버 설정
    void set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server);
    
    // PlayerManager 설정
    void set_player_manager(std::shared_ptr<PlayerManager> manager);

    // 통계
    size_t get_monster_count() const { return monsters_.size(); }
    size_t get_monster_count_by_type(MonsterType type) const;
    size_t get_monster_count_by_name(const std::string& name) const;

private:
    std::unordered_map<uint32_t, std::shared_ptr<Monster>> monsters_;
    std::vector<MonsterSpawnConfig> spawn_configs_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_spawn_times_;
    std::atomic<uint32_t> next_monster_id_;
    std::atomic<bool> auto_spawn_enabled_;
    std::shared_ptr<BehaviorTreeEngine> bt_engine_;
    std::shared_ptr<bt::SimpleWebSocketServer> websocket_server_;
    std::shared_ptr<PlayerManager> player_manager_;
    mutable std::mutex monsters_mutex_;
    mutable std::mutex spawn_mutex_;
    
    void process_auto_spawn(float delta_time);
    void process_respawn(float delta_time);
    
    // 플레이어 리스폰 포인트
    std::vector<MonsterPosition> player_respawn_points_;
    
    // JSON 파싱 헬퍼 함수들
    void parse_patrol_points(const std::string& points_content, std::vector<MonsterPosition>& points);
    MonsterPosition get_random_respawn_point() const;
};


// 몬스터 AI 액션 함수들
namespace MonsterActions {
    // 기본 액션들
    BTNodeStatus attack(BTContext& context);
    BTNodeStatus move_to_target(BTContext& context);
    BTNodeStatus patrol(BTContext& context);
    BTNodeStatus flee(BTContext& context);
    BTNodeStatus idle(BTContext& context);
    BTNodeStatus die(BTContext& context);
    
    // 전투 관련 액션들
    BTNodeStatus chase_target(BTContext& context);
    BTNodeStatus detect_enemy(BTContext& context);
    BTNodeStatus return_to_patrol(BTContext& context);
    
    // 특수 액션들
    BTNodeStatus cast_spell(BTContext& context);
    BTNodeStatus summon_minion(BTContext& context);
    BTNodeStatus heal_self(BTContext& context);
    BTNodeStatus use_item(BTContext& context);
}

// 몬스터 AI 조건 함수들
namespace MonsterConditions {
    // 기본 조건들
    bool has_target(BTContext& context);
    bool is_target_in_range(BTContext& context);
    bool is_target_visible(BTContext& context);
    bool is_health_low(BTContext& context);
    bool is_health_critical(BTContext& context);
    bool is_mana_low(BTContext& context);
    bool is_in_combat(BTContext& context);
    bool is_dead(BTContext& context);
    
    // 환경 인지 조건들
    bool can_see_enemy(BTContext& context);
    bool is_enemy_in_detection_range(BTContext& context);
    bool is_enemy_in_attack_range(BTContext& context);
    bool is_enemy_in_chase_range(BTContext& context);
    bool has_line_of_sight(BTContext& context);
    bool is_path_clear(BTContext& context);
    
    // 전투 관련 조건들
    bool has_enemy_target(BTContext& context);
    bool is_target_alive(BTContext& context);
    bool is_target_in_attack_range(BTContext& context);
    bool is_target_in_detection_range(BTContext& context);
    bool is_target_in_chase_range(BTContext& context);
    bool should_return_to_patrol(BTContext& context);
    
    // 특수 조건들
    bool can_cast_spell(BTContext& context);
    bool can_summon_minion(BTContext& context);
    bool has_usable_item(BTContext& context);
    bool is_target_stronger(BTContext& context);
    bool is_night_time(BTContext& context);
}

// 몬스터별 Behavior Tree 생성 함수들
namespace MonsterBTs {
    // 기본 몬스터 AI
    std::shared_ptr<BehaviorTree> create_goblin_bt();
    std::shared_ptr<BehaviorTree> create_orc_bt();
    std::shared_ptr<BehaviorTree> create_skeleton_bt();
    std::shared_ptr<BehaviorTree> create_zombie_bt();
    
    // 보스 몬스터 AI
    std::shared_ptr<BehaviorTree> create_dragon_bt();
    
    // NPC AI
    std::shared_ptr<BehaviorTree> create_merchant_bt();
    std::shared_ptr<BehaviorTree> create_guard_bt();
}

} // namespace bt
