#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "BT/Monster/MonsterTypes.h"
#include "BT/Monster/MonsterManager.h"
#include "PlayerManager.h"
#include "../../BT/BehaviorTreeEngine.h"

namespace bt
{

    // 몬스터 상태 정보 (JSON 직렬화용)
    struct MonsterStatusInfo
    {
        uint32_t                              id;
        std::string                           name;
        std::string                           type;
        float                                 x, y, z, rotation;
        std::string                           state;
        uint32_t                              health;
        uint32_t                              max_health;
        uint32_t                              target_id;
        bool                                  has_target;
        std::string                           ai_name;
        std::string                           bt_name;
        bool                                  is_active;
        std::vector<float>                    patrol_points; // x, y, z 순서로 저장
        size_t                                current_patrol_index;
        std::chrono::steady_clock::time_point last_update;
    };

    // 서버 통계 정보
    struct ServerStats
    {
        size_t                                total_monsters;
        size_t                                active_monsters;
        size_t                                total_players;
        size_t                                active_players;
        size_t                                registered_bt_trees;
        std::chrono::steady_clock::time_point server_start_time;
        std::chrono::steady_clock::time_point last_update;
    };

    // REST API 서버 클래스 (실제로는 REST API + 정적 파일 서빙)
    class RestApiServer
    {
    public:
        RestApiServer(uint16_t port = 8080);
        ~RestApiServer();

        // 서버 시작/중지
        bool Start();
        void Stop();
        bool IsRunning() const { return running_.load(); }

        // 매니저 설정
        void SetMonsterManager(std::shared_ptr<MonsterManager> manager);
        void SetPlayerManager(std::shared_ptr<PlayerManager> manager);
        void SetBTEngine(std::shared_ptr<BehaviorTreeEngine> engine);
        void SetWebSocketServer(std::shared_ptr<SimpleWebSocketServer> server);

        // 포트 설정
        void     SetPort(uint16_t port) { port_ = port; }
        uint16_t GetPort() const { return port_; }

        // 통계 정보
        size_t GetConnectedClients() const { return connected_clients_.load(); }
        size_t GetTotalRequests() const { return total_requests_.load(); }

    private:
        uint16_t            port_;
        std::atomic<bool>   running_;
        std::atomic<size_t> connected_clients_;
        std::atomic<size_t> total_requests_;

        std::shared_ptr<MonsterManager>        monster_manager_;
        std::shared_ptr<PlayerManager>         player_manager_;
        std::shared_ptr<BehaviorTreeEngine>    bt_engine_;
        std::shared_ptr<SimpleWebSocketServer> websocket_server_;

        std::thread server_thread_;

        // HTTP 서버 메인 루프
        void ServerLoop();

        // HTTP 요청 처리
        void HandleRequest(const std::string& request, std::string& response);

        // API 엔드포인트들
        void HandleRoot(const std::string& request, std::string& response);
        void HandleApiMonsters(const std::string& request, std::string& response);
        void HandleApiMonsterDetail(const std::string& request, std::string& response);
        void HandleApiStats(const std::string& request, std::string& response);
        void HandleApiPlayers(const std::string& request, std::string& response);

        // 유틸리티 함수들
        std::string GetMonsterStatusJson() const;
        std::string GetServerStatsJson() const;
        std::string GetPlayerStatusJson() const;
        std::string MonsterTypeToString(MonsterType type) const;
        std::string MonsterStateToString(MonsterState state) const;

        // HTTP 헬퍼 함수들
        std::string ExtractPath(const std::string& request) const;
        std::string ExtractMethod(const std::string& request) const;
        std::string CreateHttpResponse(const std::string& content,
                                         const std::string& content_type = "text/html") const;
        std::string CreateJsonResponse(const std::string& json) const;
        std::string CreateErrorResponse(int status_code, const std::string& message) const;

        // HTML 템플릿
        std::string GetDashboardHtml() const;
        std::string GetMonsterDetailHtml() const;
    };

} // namespace bt
