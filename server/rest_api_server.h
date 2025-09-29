#pragma once

#include "monster_ai.h"
#include "player_manager.h"
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace bt {

// 몬스터 상태 정보 (JSON 직렬화용)
struct MonsterStatusInfo {
    uint32_t id;
    std::string name;
    std::string type;
    float x, y, z, rotation;
    std::string state;
    uint32_t health;
    uint32_t max_health;
    uint32_t target_id;
    bool has_target;
    std::string ai_name;
    std::string bt_name;
    bool is_active;
    std::vector<float> patrol_points; // x, y, z 순서로 저장
    size_t current_patrol_index;
    std::chrono::steady_clock::time_point last_update;
};

// 서버 통계 정보
struct ServerStats {
    size_t total_monsters;
    size_t active_monsters;
    size_t total_players;
    size_t active_players;
    size_t registered_bt_trees;
    std::chrono::steady_clock::time_point server_start_time;
    std::chrono::steady_clock::time_point last_update;
};

// REST API 서버 클래스 (실제로는 REST API + 정적 파일 서빙)
class RestApiServer {
public:
    RestApiServer(uint16_t port = 8080);
    ~RestApiServer();

    // 서버 시작/중지
    bool start();
    void stop();
    bool is_running() const { return running_.load(); }

    // 매니저 설정
    void set_monster_manager(std::shared_ptr<MonsterManager> manager);
    void set_player_manager(std::shared_ptr<PlayerManager> manager);
    void set_bt_engine(std::shared_ptr<BehaviorTreeEngine> engine);
    void set_websocket_server(std::shared_ptr<SimpleWebSocketServer> server);

    // 포트 설정
    void set_port(uint16_t port) { port_ = port; }
    uint16_t get_port() const { return port_; }

    // 통계 정보
    size_t get_connected_clients() const { return connected_clients_.load(); }
    size_t get_total_requests() const { return total_requests_.load(); }

private:
    uint16_t port_;
    std::atomic<bool> running_;
    std::atomic<size_t> connected_clients_;
    std::atomic<size_t> total_requests_;
    
    std::shared_ptr<MonsterManager> monster_manager_;
    std::shared_ptr<PlayerManager> player_manager_;
    std::shared_ptr<BehaviorTreeEngine> bt_engine_;
    std::shared_ptr<SimpleWebSocketServer> websocket_server_;
    
    std::thread server_thread_;
    
    // HTTP 서버 메인 루프
    void server_loop();
    
    // HTTP 요청 처리
    void handle_request(const std::string& request, std::string& response);
    
    // API 엔드포인트들
    void handle_root(const std::string& request, std::string& response);
    void handle_api_monsters(const std::string& request, std::string& response);
    void handle_api_monster_detail(const std::string& request, std::string& response);
    void handle_api_stats(const std::string& request, std::string& response);
    void handle_api_players(const std::string& request, std::string& response);
    
    // 유틸리티 함수들
    std::string get_monster_status_json() const;
    std::string get_server_stats_json() const;
    std::string get_player_status_json() const;
    std::string monster_type_to_string(MonsterType type) const;
    std::string monster_state_to_string(MonsterState state) const;
    
    // HTTP 헬퍼 함수들
    std::string extract_path(const std::string& request) const;
    std::string extract_method(const std::string& request) const;
    std::string create_http_response(const std::string& content, const std::string& content_type = "text/html") const;
    std::string create_json_response(const std::string& json) const;
    std::string create_error_response(int status_code, const std::string& message) const;
    
    // HTML 템플릿
    std::string get_dashboard_html() const;
    std::string get_monster_detail_html() const;
};

} // namespace bt
