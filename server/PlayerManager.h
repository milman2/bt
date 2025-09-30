#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "BT/Monster/MonsterTypes.h" // MonsterPosition을 위해 필요
#include "Player.h"

namespace bt
{

    // 전방 선언
    class SimpleWebSocketServer;

    // 플레이어 매니저
    class PlayerManager
    {
    public:
        PlayerManager();
        ~PlayerManager();

        // 플레이어 관리
        void                                 add_player(std::shared_ptr<Player> player);
        void                                 remove_player(uint32_t player_id);
        std::shared_ptr<Player>              get_player(uint32_t player_id);
        std::vector<std::shared_ptr<Player>> get_all_players();
        std::vector<std::shared_ptr<Player>> get_players_in_range(const MonsterPosition& position, float range);

        // 플레이어 생성
        std::shared_ptr<Player> create_player(const std::string& name, const MonsterPosition& position);

        // 클라이언트 ID와 플레이어 ID 매핑
        std::shared_ptr<Player> create_player_for_client(uint32_t               client_id,
                                                         const std::string&     name,
                                                         const MonsterPosition& position);
        void                    remove_player_by_client_id(uint32_t client_id);
        std::shared_ptr<Player> get_player_by_client_id(uint32_t client_id);
        uint32_t                get_client_id_by_player_id(uint32_t player_id);

        // 전투 시스템
        void process_combat(float delta_time);
        void attack_player(uint32_t attacker_id, uint32_t target_id, uint32_t damage);
        void attack_monster(uint32_t attacker_id, uint32_t target_id, uint32_t damage);

        // 플레이어 AI
        void process_player_ai(float delta_time);
        void move_player_to_random_location(uint32_t player_id);
        void attack_nearby_monster(uint32_t player_id);

        // 리스폰 시스템
        void process_player_respawn(float delta_time);
        void respawn_player(uint32_t player_id);
        void set_player_respawn_points(const std::vector<MonsterPosition>& points);

        // 업데이트
        void update(float delta_time);

        // WebSocket 서버 설정
        void set_websocket_server(std::shared_ptr<bt::SimpleWebSocketServer> server);

        // 통계
        size_t get_player_count() const { return players_.size(); }

    private:
        std::unordered_map<uint32_t, std::shared_ptr<Player>> players_;
        std::unordered_map<uint32_t, uint32_t>                client_to_player_id_; // 클라이언트 ID -> 플레이어 ID
        std::unordered_map<uint32_t, uint32_t>                player_to_client_id_; // 플레이어 ID -> 클라이언트 ID
        std::atomic<uint32_t>                                 next_player_id_;
        std::shared_ptr<bt::SimpleWebSocketServer>            websocket_server_;
        mutable std::mutex                                    players_mutex_;

        // 플레이어 리스폰 포인트
        std::vector<MonsterPosition> player_respawn_points_;

        // 플레이어 AI 관련
        std::unordered_map<uint32_t, float> player_last_move_time_;
        std::unordered_map<uint32_t, float> player_last_attack_time_;
        float                               player_move_interval_   = 5.0f; // 5초마다 랜덤 이동
        float                               player_attack_interval_ = 2.0f; // 2초마다 공격 시도

        MonsterPosition get_random_respawn_point() const;
    };

} // namespace bt
