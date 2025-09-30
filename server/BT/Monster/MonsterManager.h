#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Monster.h"

// 전방 선언
namespace bt
{
    class SimpleWebSocketServer;
    class PlayerManager;
    class BehaviorTreeEngine;
} // namespace bt

namespace bt
{

    // 몬스터 매니저
    class MonsterManager
    {
    public:
        MonsterManager();
        ~MonsterManager();

        // 몬스터 관리
        void                                  add_monster(std::shared_ptr<Monster> monster);
        void                                  remove_monster(uint32_t monster_id);
        std::shared_ptr<Monster>              get_monster(uint32_t monster_id);
        std::vector<std::shared_ptr<Monster>> get_all_monsters();
        std::vector<std::shared_ptr<Monster>> get_monsters_in_range(const MonsterPosition& position, float range);

        // 몬스터 생성
        std::shared_ptr<Monster> spawn_monster(MonsterType            type,
                                               const std::string&     name,
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
        std::unordered_map<uint32_t, std::shared_ptr<Monster>>                 monsters_;
        std::vector<MonsterSpawnConfig>                                        spawn_configs_;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_spawn_times_;
        std::atomic<uint32_t>                                                  next_monster_id_;
        std::atomic<bool>                                                      auto_spawn_enabled_;
        std::shared_ptr<BehaviorTreeEngine>                                    bt_engine_;
        std::shared_ptr<bt::SimpleWebSocketServer>                             websocket_server_;
        std::shared_ptr<PlayerManager>                                         player_manager_;
        mutable std::mutex                                                     monsters_mutex_;
        mutable std::mutex                                                     spawn_mutex_;

        void process_auto_spawn(float delta_time);
        void process_respawn(float delta_time);

        // 플레이어 리스폰 포인트
        std::vector<MonsterPosition> player_respawn_points_;

        // JSON 파싱 헬퍼 함수들
        void            parse_patrol_points(const std::string& points_content, std::vector<MonsterPosition>& points);
        MonsterPosition get_random_respawn_point() const;
    };

} // namespace bt
