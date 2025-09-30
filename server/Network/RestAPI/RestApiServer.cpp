#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Player.h"
#include "PlayerManager.h"
#include "RestApiServer.h"
#include "Network/WebSocket/SimpleWebSocketServer.h"
#include "BT/Monster/MonsterBTExecutor.h"

namespace bt
{

    RestApiServer::RestApiServer(uint16_t port)
        : port_(port), running_(false), connected_clients_(0), total_requests_(0)
    {
    }

    RestApiServer::~RestApiServer()
    {
        Stop();
    }

    bool RestApiServer::Start()
    {
        if (running_.load())
        {
            return true;
        }

        running_.store(true);
        server_thread_ = std::thread(&RestApiServer::ServerLoop, this);

        std::cout << "웹 서버가 포트 " << port_ << "에서 시작되었습니다." << std::endl;
        std::cout << "브라우저에서 http://localhost:" << port_ << " 으로 접속하세요." << std::endl;

        return true;
    }

    void RestApiServer::Stop()
    {
        if (!running_.load())
        {
            return;
        }

        running_.store(false);
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }

        std::cout << "웹 서버가 중지되었습니다." << std::endl;
    }

    void RestApiServer::SetMonsterManager(std::shared_ptr<MonsterManager> manager)
    {
        monster_manager_ = manager;
    }

    void RestApiServer::SetPlayerManager(std::shared_ptr<PlayerManager> manager)
    {
        player_manager_ = manager;
    }

    void RestApiServer::SetBTEngine(std::shared_ptr<BehaviorTreeEngine> engine)
    {
        bt_engine_ = engine;
    }

    void RestApiServer::SetWebSocketServer(std::shared_ptr<SimpleWebSocketServer> server)
    {
        websocket_server_ = server;
    }

    void RestApiServer::ServerLoop()
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0)
        {
            std::cerr << "소켓 생성 실패" << std::endl;
            return;
        }

        // 소켓 옵션 설정
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            std::cerr << "소켓 옵션 설정 실패" << std::endl;
            close(server_fd);
            return;
        }

        // 주소 설정
        struct sockaddr_in address;
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port        = htons(port_);

        // 바인딩
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
        {
            std::cerr << "바인딩 실패" << std::endl;
            close(server_fd);
            return;
        }

        // 리스닝
        if (listen(server_fd, 10) < 0)
        {
            std::cerr << "리스닝 실패" << std::endl;
            close(server_fd);
            return;
        }

        std::cout << "웹 서버가 포트 " << port_ << "에서 리스닝 중..." << std::endl;

        while (running_.load())
        {
            struct sockaddr_in client_address;
            int                addrlen = sizeof(client_address);

            // 클라이언트 연결 대기 (논블로킹)
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);

            struct timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0;

            int activity = select(server_fd + 1, &readfds, NULL, NULL, &timeout);

            if (activity < 0)
            {
                if (running_.load())
                {
                    std::cerr << "select 오류" << std::endl;
                }
                continue;
            }

            if (activity == 0)
            {
                // 타임아웃 - 계속 실행
                continue;
            }

            if (FD_ISSET(server_fd, &readfds))
            {
                int client_socket = accept(server_fd, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
                if (client_socket < 0)
                {
                    continue;
                }

                connected_clients_++;

                // 클라이언트 요청 처리
                char buffer[4096] = {0};
                int  bytes_read   = read(client_socket, buffer, 4096);

                if (bytes_read > 0)
                {
                    std::string request(buffer, bytes_read);
                    std::string response;

                    HandleRequest(request, response);

                    send(client_socket, response.c_str(), response.length(), 0);
                    total_requests_++;
                }

                close(client_socket);
                connected_clients_--;
            }
        }

        close(server_fd);
    }

    void RestApiServer::HandleRequest(const std::string& request, std::string& response)
    {
        std::string method = ExtractMethod(request);
        std::string path   = ExtractPath(request);

        if (path == "/")
        {
            HandleRoot(request, response);
        }
        else if (path == "/api/monsters")
        {
            HandleApiMonsters(request, response);
        }
        else if (path.find("/api/monsters/") == 0)
        {
            HandleApiMonsterDetail(request, response);
        }
        else if (path == "/api/stats")
        {
            HandleApiStats(request, response);
        }
        else if (path == "/api/players")
        {
            HandleApiPlayers(request, response);
        }
        else
        {
            response = CreateErrorResponse(404, "Not Found");
        }
    }

    void RestApiServer::HandleRoot(const std::string& request, std::string& response)
    {
        response = CreateHttpResponse(GetDashboardHtml());
    }

    void RestApiServer::HandleApiMonsters(const std::string& request, std::string& response)
    {
        std::string json = GetMonsterStatusJson();
        response         = CreateJsonResponse(json);
    }

    void RestApiServer::HandleApiMonsterDetail(const std::string& request, std::string& response)
    {
        // 간단한 구현 - 모든 몬스터 정보 반환
        std::string json = GetMonsterStatusJson();
        response         = CreateJsonResponse(json);
    }

    void RestApiServer::HandleApiStats(const std::string& request, std::string& response)
    {
        std::string json = GetServerStatsJson();
        response         = CreateJsonResponse(json);
    }

    void RestApiServer::HandleApiPlayers(const std::string& request, std::string& response)
    {
        std::string json = GetPlayerStatusJson();
        response         = CreateJsonResponse(json);
    }

    std::string RestApiServer::GetMonsterStatusJson() const
    {
        std::ostringstream json;
        json << "{\n";
        json << "  \"monsters\": [\n";

        if (monster_manager_)
        {
            auto monsters = monster_manager_->GetAllMonsters();
            bool first    = true;

            for (const auto& monster : monsters)
            {
                if (!first)
                    json << ",\n";
                first = false;

                json << "    {\n";
                json << "      \"id\": " << monster->GetTargetID() << ",\n";
                json << "      \"name\": \"" << monster->GetName() << "\",\n";
                json << "      \"type\": \"" << MonsterTypeToString(monster->GetType()) << "\",\n";

                const auto& pos = monster->GetPosition();
                json << "      \"position\": {\n";
                json << "        \"x\": " << std::fixed << std::setprecision(2) << pos.x << ",\n";
                json << "        \"y\": " << pos.y << ",\n";
                json << "        \"z\": " << pos.z << ",\n";
                json << "        \"rotation\": " << pos.rotation << "\n";
                json << "      },\n";

                json << "      \"state\": \"" << MonsterStateToString(monster->GetState()) << "\",\n";

                const auto& stats = monster->GetStats();
                json << "      \"health\": " << stats.health << ",\n";
                json << "      \"max_health\": " << stats.max_health << ",\n";
                json << "      \"tarGetID\": " << monster->GetTargetID() << ",\n";
                json << "      \"HasTarget\": " << (monster->HasTarget() ? "true" : "false") << ",\n";

                auto ai = monster->GetAI();
                if (ai)
                {
                    json << "      \"ai_name\": \"" << ai->GetName() << "\",\n";
                    json << "      \"bt_name\": \"" << ai->GetBTName() << "\",\n";
                    json << "      \"is_active\": " << (ai->IsActive() ? "true" : "false") << ",\n";
                }
                else
                {
                    json << "      \"ai_name\": \"\",\n";
                    json << "      \"bt_name\": \"\",\n";
                    json << "      \"is_active\": false,\n";
                }

                json << "      \"has_patrol_points\": " << (monster->HasPatrolPoints() ? "true" : "false") << "\n";
                json << "    }";
            }
        }

        json << "\n  ],\n";
        json << "  \"timestamp\": "
             << std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count()
             << "\n";
        json << "}\n";

        return json.str();
    }

    std::string RestApiServer::GetServerStatsJson() const
    {
        std::ostringstream json;
        json << "{\n";

        if (monster_manager_)
        {
            json << "  \"total_monsters\": " << monster_manager_->GetMonsterCount() << ",\n";
        }
        else
        {
            json << "  \"total_monsters\": 0,\n";
        }

        if (player_manager_)
        {
            json << "  \"total_players\": " << player_manager_->GetPlayerCount() << ",\n";
        }
        else
        {
            json << "  \"total_players\": 0,\n";
        }

        if (bt_engine_)
        {
            json << "  \"registered_bt_trees\": " << bt_engine_->GetRegisteredTrees() << ",\n";
            json << "  \"active_monsters\": " << bt_engine_->GetActiveMonsters() << ",\n";
        }
        else
        {
            json << "  \"registered_bt_trees\": 0,\n";
            json << "  \"active_monsters\": 0,\n";
        }

        // WebSocket 서버의 실제 연결 수 사용
        size_t ws_clients = websocket_server_ ? websocket_server_->get_connected_clients() : 0;
        json << "  \"connected_clients\": " << ws_clients << ",\n";
        json << "  \"total_requests\": " << total_requests_.load() << ",\n";
        json << "  \"timestamp\": "
             << std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count()
             << "\n";
        json << "}\n";

        return json.str();
    }

    std::string RestApiServer::GetPlayerStatusJson() const
    {
        std::ostringstream json;
        json << "{\n";
        json << "  \"players\": [\n";

        if (player_manager_)
        {
            auto players = player_manager_->GetAllPlayers();
            bool first   = true;

            for (const auto& player : players)
            {
                if (!first)
                    json << ",\n";
                first = false;

                json << "    {\n";
                json << "      \"id\": " << player->GetID() << ",\n";
                json << "      \"name\": \"" << player->GetName() << "\",\n";

                const auto& pos = player->GetPosition();
                json << "      \"position\": {\n";
                json << "        \"x\": " << std::fixed << std::setprecision(2) << pos.x << ",\n";
                json << "        \"y\": " << pos.y << ",\n";
                json << "        \"z\": " << pos.z << ",\n";
                json << "        \"rotation\": " << pos.rotation << "\n";
                json << "      },\n";

                json << "      \"state\": \"" << static_cast<int>(player->GetState()) << "\",\n";
                const auto& stats = player->GetStats();
                json << "      \"health\": " << stats.health << ",\n";
                json << "      \"max_health\": " << stats.max_health << ",\n";
                json << "      \"IsAlive\": " << (player->IsAlive() ? "true" : "false") << "\n";
                json << "    }";
            }
        }

        json << "\n  ],\n";
        json << "  \"timestamp\": "
             << std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count()
             << "\n";
        json << "}\n";

        return json.str();
    }

    std::string RestApiServer::MonsterTypeToString(MonsterType type) const
    {
        switch (type)
        {
            case MonsterType::GOBLIN:
                return "GOBLIN";
            case MonsterType::ORC:
                return "ORC";
            case MonsterType::DRAGON:
                return "DRAGON";
            case MonsterType::SKELETON:
                return "SKELETON";
            case MonsterType::ZOMBIE:
                return "ZOMBIE";
            case MonsterType::NPC_MERCHANT:
                return "NPC_MERCHANT";
            case MonsterType::NPC_GUARD:
                return "NPC_GUARD";
            default:
                return "UNKNOWN";
        }
    }

    std::string RestApiServer::MonsterStateToString(MonsterState state) const
    {
        switch (state)
        {
            case MonsterState::IDLE:
                return "IDLE";
            case MonsterState::PATROL:
                return "PATROL";
            case MonsterState::CHASE:
                return "CHASE";
            case MonsterState::ATTACK:
                return "ATTACK";
            case MonsterState::FLEE:
                return "FLEE";
            case MonsterState::DEAD:
                return "DEAD";
            default:
                return "UNKNOWN";
        }
    }

    std::string RestApiServer::ExtractPath(const std::string& request) const
    {
        size_t start = request.find(' ');
        if (start == std::string::npos)
            return "/";

        size_t end = request.find(' ', start + 1);
        if (end == std::string::npos)
            return "/";

        return request.substr(start + 1, end - start - 1);
    }

    std::string RestApiServer::ExtractMethod(const std::string& request) const
    {
        size_t end = request.find(' ');
        if (end == std::string::npos)
            return "GET";

        return request.substr(0, end);
    }

    std::string RestApiServer::CreateHttpResponse(const std::string& content, const std::string& content_type) const
    {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: " << content_type << "; charset=utf-8\r\n";
        response << "Content-Length: " << content.length() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        response << "Access-Control-Allow-Headers: Content-Type\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << content;

        return response.str();
    }

    std::string RestApiServer::CreateJsonResponse(const std::string& json) const
    {
        return CreateHttpResponse(json, "application/json");
    }

    std::string RestApiServer::CreateErrorResponse(int status_code, const std::string& message) const
    {
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << message.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << message;

        return response.str();
    }

    std::string RestApiServer::GetDashboardHtml() const
    {
        std::ifstream file("web/dashboard.html");
        if (!file.is_open())
        {
            return "<html><body><h1>Error: Dashboard file not found</h1></body></html>";
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        return content;
    }

} // namespace bt
