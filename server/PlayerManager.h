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
    class BeastHttpWebSocketServer;

    // 플레이어 매니저
    class PlayerManager
    {
    public:
        PlayerManager();
        ~PlayerManager();

        // 플레이어 관리
        void                                 AddPlayer(std::shared_ptr<Player> player);
        void                                 RemovePlayer(uint32_t player_id);
        std::shared_ptr<Player>              GetPlayer(uint32_t player_id);
        std::vector<std::shared_ptr<Player>> GetAllPlayers();
        std::vector<std::shared_ptr<Player>> GetPlayersInRange(const MonsterPosition& position, float range);

        // 플레이어 생성
        std::shared_ptr<Player> CreatePlayer(const std::string& name, const MonsterPosition& position);

        // 클라이언트 ID와 플레이어 ID 매핑
        std::shared_ptr<Player> CreatePlayerForClient(uint32_t               client_id,
                                                         const std::string&     name,
                                                         const MonsterPosition& position);
        void                    RemovePlayerByClientID(uint32_t client_id);
        std::shared_ptr<Player> GetPlayerByClientID(uint32_t client_id);
        uint32_t                GetClientIDByPlayerID(uint32_t player_id);

        // 전투 시스템
        void ProcessCombat(float delta_time);
        void AttackPlayer(uint32_t attacker_id, uint32_t target_id, uint32_t damage);
        void AttackMonster(uint32_t attacker_id, uint32_t target_id, uint32_t damage);

        // 플레이어 AI
        void ProcessPlayerAI(float delta_time);
        void MovePlayerToRandomLocation(uint32_t player_id);
        void AttackNearbyMonster(uint32_t player_id);

        // 리스폰 시스템
        void ProcessPlayerRespawn(float delta_time);
        void RespawnPlayer(uint32_t player_id);
        void SetPlayerRespawnPoints(const std::vector<MonsterPosition>& points);

        // 업데이트
        void Update(float delta_time);

        // 통합 HTTP+WebSocket 서버 설정
        void SetHttpWebSocketServer(std::shared_ptr<bt::BeastHttpWebSocketServer> server);

        // 통계
        size_t GetPlayerCount() const { return players_.size(); }

    private:
        std::unordered_map<uint32_t, std::shared_ptr<Player>> players_;
        std::unordered_map<uint32_t, uint32_t>                client_to_player_id_; // 클라이언트 ID -> 플레이어 ID
        std::unordered_map<uint32_t, uint32_t>                player_to_client_id_; // 플레이어 ID -> 클라이언트 ID
        std::atomic<uint32_t>                                 next_player_id_;
        std::shared_ptr<bt::BeastHttpWebSocketServer>        http_websocket_server_;
        mutable std::mutex                                    players_mutex_;

        // 플레이어 리스폰 포인트
        std::vector<MonsterPosition> player_respawn_points_;

        // 플레이어 AI 관련
        std::unordered_map<uint32_t, float> player_last_move_time_;
        std::unordered_map<uint32_t, float> player_last_attack_time_;
        float                               player_move_interval_   = 5.0f; // 5초마다 랜덤 이동
        float                               player_attack_interval_ = 2.0f; // 2초마다 공격 시도

        MonsterPosition GetRandomRespawnPoint() const;
    };

} // namespace bt
