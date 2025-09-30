#include <iomanip>
#include <iostream>
#include <sstream>

#include "AsioServer.h"
#include "BT/BehaviorTree.h"
#include "BT/Monster/MonsterTypes.h"
#include "Player.h"
#include "PlayerManager.h"
#include "Network/RestAPI/RestApiServer.h"
#include "Network/WebSocket/SimpleWebSocketServer.h"

namespace bt
{

    AsioServer::AsioServer(const AsioServerConfig& config)
        : config_(config), running_(false), acceptor_(io_context_), total_packets_sent_(0), total_packets_received_(0)
    {
        // Behavior Tree 엔진 초기화
        bt_engine_ = std::make_unique<BehaviorTreeEngine>();

        // 매니저들 초기화
        monster_manager_ = std::make_shared<MonsterManager>();
        player_manager_  = std::make_shared<PlayerManager>();

        // 웹 서버 초기화
        rest_api_server_ = std::make_shared<RestApiServer>(8081); // REST API 서버는 8081 포트 사용
        rest_api_server_->set_monster_manager(monster_manager_);
        rest_api_server_->set_player_manager(player_manager_);
        // bt_engine_을 shared_ptr로 변환 (원본은 유지)
        std::shared_ptr<BehaviorTreeEngine> shared_bt_engine(bt_engine_.get(),
                                                             [](BehaviorTreeEngine*)
                                                             {
                                                             });
        rest_api_server_->set_bt_engine(shared_bt_engine);

        // WebSocket 서버 초기화
        websocket_server_ = std::make_shared<SimpleWebSocketServer>(8082); // WebSocket 서버는 8082 포트 사용

        // RestApiServer에 WebSocket 서버 참조 설정
        rest_api_server_->set_websocket_server(websocket_server_);

        // MonsterManager에 WebSocket 서버 설정
        monster_manager_->SetWebSocketServer(websocket_server_);

        // MonsterManager에 PlayerManager 설정
        monster_manager_->SetPlayerManager(player_manager_);

        // MonsterManager에 BT 엔진 설정
        monster_manager_->SetBTEngine(shared_bt_engine);

        // PlayerManager에 WebSocket 서버 설정
        player_manager_->set_websocket_server(websocket_server_);

        LogMessage("AsioServer 인스턴스가 생성되었습니다.");
    }

    AsioServer::~AsioServer()
    {
        Stop();
        LogMessage("AsioServer 인스턴스가 소멸되었습니다.");
    }

    bool AsioServer::Start()
    {
        if (running_.load())
        {
            LogMessage("서버가 이미 실행 중입니다.", true);
            return false;
        }

        LogMessage("서버 시작 중...");

        try
        {
            // 엔드포인트 설정
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(config_.host), config_.port);

            // 어셉터 설정
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor_.bind(endpoint);
            acceptor_.listen();

            // running_ 플래그를 먼저 설정
            running_.store(true);

            // 서버 시작 시간 기록
            server_start_time_ = std::chrono::steady_clock::now();

            // 연결 수락 시작 (워커 스레드 시작 전에)
            StartAccept();

            // 워커 스레드 시작 (async_accept 후에)
            for (size_t i = 0; i < config_.worker_threads; ++i)
            {
                worker_threads_.create_thread(
                    [this, i]()
                    {
                        LogMessage("워커 스레드 " + std::to_string(i) + " 시작");
                        io_context_.run();
                        LogMessage("워커 스레드 " + std::to_string(i) + " 종료");
                    });
            }

            // REST API 서버 시작
            if (rest_api_server_)
            {
                rest_api_server_->start();
            }

            // WebSocket 서버 시작
            if (websocket_server_)
            {
                websocket_server_->start();
            }

            LogMessage("서버가 성공적으로 시작되었습니다. 포트: " + std::to_string(config_.port));
            LogMessage("REST API 서버: http://localhost:8081");
            LogMessage("WebSocket 실시간 연결: ws://localhost:8082");

            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("서버 시작 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    void AsioServer::Stop()
    {
        if (!running_.load())
        {
            return;
        }

        LogMessage("서버 종료 중...");
        running_.store(false);

        try
        {
            // 어셉터 닫기
            acceptor_.close();

            // 모든 클라이언트 연결 종료
            {
                boost::lock_guard<boost::mutex> lock(clients_mutex_);
                for (auto& [client, info] : clients_)
                {
                    client->Stop();
                }
                clients_.clear();
            }

            // IO 컨텍스트 중지
            io_context_.stop();

            // 워커 스레드 종료
            worker_threads_.join_all();

            // REST API 서버 중지
            if (rest_api_server_)
            {
                rest_api_server_->stop();
            }

            // WebSocket 서버 중지
            if (websocket_server_)
            {
                websocket_server_->stop();
            }

            LogMessage("서버가 종료되었습니다.");
        }
        catch (const std::exception& e)
        {
            LogMessage("서버 종료 중 오류: " + std::string(e.what()), true);
        }
    }

    void AsioServer::StartAccept()
    {
        if (!running_.load())
        {
            LogMessage("서버가 실행 중이 아니므로 연결 수락을 시작하지 않습니다.", true);
            return;
        }

        auto client = boost::make_shared<AsioClient>(io_context_, this);

        LogMessage("새로운 클라이언트 연결을 기다리는 중...");
        LogMessage("async_accept 호출 중...");

        try
        {
            acceptor_.async_accept(
                client->Socket(),
                [this, client](const boost::system::error_code& error)
                {
                    LogMessage("async_accept 콜백 호출됨, 에러: " + (error ? error.message() : "없음"));
                    HandleAccept(client, error);
                });
            LogMessage("async_accept 호출 완료");
        }
        catch (const std::exception& e)
        {
            LogMessage("async_accept 호출 중 예외 발생: " + std::string(e.what()), true);
        }
    }

    void AsioServer::HandleAccept(boost::shared_ptr<AsioClient> client, const boost::system::error_code& error)
    {
        LogMessage("handle_accept 호출됨, 에러: " + (error ? error.message() : "없음"));

        if (!error)
        {
            LogMessage("클라이언트 연결 성공, IP: " + client->GetIPAddress() + ":" +
                        std::to_string(client->GetPort()));

            // 클라이언트 수 제한 확인
            {
                boost::lock_guard<boost::mutex> lock(clients_mutex_);
                if (clients_.size() >= config_.max_clients)
                {
                    LogMessage("최대 클라이언트 수 초과. 연결 거부", true);
                    client->Stop();
                    StartAccept();
                    return;
                }
            }

            // 클라이언트 추가
            AddClient(client);
            client->Start();

            LogMessage("새 클라이언트 연결 완료: " + client->GetIPAddress() + ":" +
                        std::to_string(client->GetPort()));
        }
        else
        {
            if (running_.load())
            {
                LogMessage("연결 수락 오류: " + error.message(), true);
            }
            else
            {
                LogMessage("서버가 종료 중이므로 연결 수락 오류 무시: " + error.message());
            }
        }

        // 다음 연결 수락 시작
        if (running_.load())
        {
            LogMessage("다음 클라이언트 연결을 기다리는 중...");
            StartAccept();
        }
        else
        {
            LogMessage("서버가 종료 중이므로 다음 연결 수락을 시작하지 않습니다.");
        }
    }

    void AsioServer::AddClient(boost::shared_ptr<AsioClient> client)
    {
        AsioClientInfo info;
        info.client           = client;
        info.ip_address       = client->GetIPAddress();
        info.port             = client->GetPort();
        info.connect_time     = client->GetConnectTime();
        info.is_authenticated = false;
        info.player_id        = 0;
        info.client_type      = "unknown";

        boost::lock_guard<boost::mutex> lock(clients_mutex_);
        clients_[client] = info;

        // 클라이언트 연결 시 플레이어 생성
        if (player_manager_)
        {
            // 클라이언트 ID 생성 (포인터 주소의 해시를 사용)
            uint32_t client_id = static_cast<uint32_t>(std::hash<void*>{}(client.get()));

            // 플레이어 이름 생성 (클라이언트 IP 기반)
            std::string player_name = "Player_" + info.ip_address.substr(info.ip_address.find_last_of('.') + 1);

            // 기본 위치 설정 (스폰 지점)
            MonsterPosition spawn_position = {0.0f, 0.0f, 0.0f, 0.0f};

            // 플레이어 생성
            auto player = player_manager_->create_player_for_client(client_id, player_name, spawn_position);
            if (player)
            {
                info.player_id          = player->GetID();
                std::string client_info = info.ip_address + ":" + std::to_string(info.port);
                LogMessage("클라이언트 연결: " + client_info + " -> 플레이어 생성: " + player_name +
                            " (ID: " + std::to_string(player->GetID()) + ")");
            }
        }
    }

    void AsioServer::RemoveClient(boost::shared_ptr<AsioClient> client)
    {
        std::string client_info;
        uint32_t    client_id = 0;

        {
            boost::lock_guard<boost::mutex> lock(clients_mutex_);
            auto                            it = clients_.find(client);
            if (it != clients_.end())
            {
                client_info = it->second.ip_address + ":" + std::to_string(it->second.port);
                client_id   = static_cast<uint32_t>(std::hash<void*>{}(client.get()));
                clients_.erase(it);
            }
        }

        // 클라이언트 연결 해제 시 플레이어 제거
        if (player_manager_ && client_id != 0)
        {
            player_manager_->remove_player_by_client_id(client_id);
        }

        if (!client_info.empty())
        {
            LogMessage("클라이언트 연결 종료: " + client_info);
        }
    }

    void AsioServer::BroadcastPacket(const Packet& packet, boost::shared_ptr<AsioClient> exclude_client)
    {
        boost::lock_guard<boost::mutex> lock(clients_mutex_);

        for (const auto& [client, info] : clients_)
        {
            if (client != exclude_client)
            {
                SendPacket(client, packet);
            }
        }
    }

    void AsioServer::SendPacket(boost::shared_ptr<AsioClient> client, const Packet& packet)
    {
        if (client && client->IsConnected())
        {
            client->SendPacket(packet);
            total_packets_sent_.fetch_add(1);
        }
    }

    void AsioServer::ProcessPacket(boost::shared_ptr<AsioClient> client, const Packet& packet)
    {
        total_packets_received_.fetch_add(1);

        // 패킷 타입에 따른 처리
        switch (static_cast<PacketType>(packet.type))
        {
            case PacketType::CONNECT_REQUEST:
                // 연결 요청 처리
                LogMessage("연결 요청 수신: " + client->GetIPAddress());
                // 연결 응답 전송
                SendConnectResponse(client);
                break;

            case PacketType::MONSTER_SPAWN:
                // 몬스터 스폰 요청 처리
                LogMessage("몬스터 스폰 요청 수신");
                // 몬스터 스폰 응답 전송
                SendMonsterSpawnResponse(client, true);
                break;

            case PacketType::MONSTER_UPDATE:
                // 몬스터 업데이트 처리
                LogMessage("몬스터 업데이트 요청 수신");
                // 몬스터 업데이트 응답 전송
                SendMonsterUpdateResponse(client, true);
                break;

            case PacketType::BT_EXECUTE:
                // Behavior Tree 실행 요청 처리
                LogMessage("BT 실행 요청 수신");
                if (bt_engine_)
                {
                    // BT 실행 로직
                    SendBTExecuteResponse(client, true);
                }
                else
                {
                    SendBTExecuteResponse(client, false);
                }
                break;

            default:
                LogMessage("알 수 없는 패킷 타입: " + std::to_string(packet.type), true);
                // 에러 응답 전송
                SendErrorResponse(client, "알 수 없는 패킷 타입");
                break;
        }
    }

    void AsioServer::SendConnectResponse(boost::shared_ptr<AsioClient> client)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success = 1; // 성공
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("연결 응답 전송 완료");
    }

    void AsioServer::SendMonsterSpawnResponse(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("몬스터 스폰 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::SendMonsterUpdateResponse(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("몬스터 업데이트 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::SendBTExecuteResponse(boost::shared_ptr<AsioClient> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RESPONSE);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("BT 실행 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void AsioServer::SendErrorResponse(boost::shared_ptr<AsioClient> client, const std::string& error_message)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::ERROR_MESSAGE);
        response.size = sizeof(uint32_t) + error_message.length(); // type + message
        response.data.resize(response.size);

        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), error_message.c_str(), error_message.length());

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("에러 응답 전송 완료: " + error_message);
    }

    size_t AsioServer::GetConnectedClients() const
    {
        boost::lock_guard<boost::mutex> lock(clients_mutex_);
        return clients_.size();
    }

    void AsioServer::LogMessage(const std::string& message, bool is_error)
    {
        boost::lock_guard<boost::mutex> lock(log_mutex_);

        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm     = *std::localtime(&time_t);

        std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        if (is_error)
        {
            std::cout << "[ERROR] ";
        }
        else
        {
            std::cout << "[INFO] ";
        }
        std::cout << message << std::endl;
    }

    // 서버 상태 모니터링을 위한 헬스체크 추가
    bool AsioServer::IsHealthy() const
    {
        try
        {
            // 기본적인 서버 상태 확인
            if (!running_.load())
            {
                return false;
            }

            // IO 컨텍스트가 정상적으로 동작하는지 확인
            if (io_context_.stopped())
            {
                return false;
            }

            // 워커 스레드가 정상적으로 동작하는지 확인
            if (worker_threads_.size() == 0)
            {
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            // const 메서드에서는 로그 출력 불가, 단순히 false 반환
            return false;
        }
    }

    // 서버 통계 정보 반환
    ServerHealthInfo AsioServer::GetHealthInfo() const
    {
        ServerHealthInfo info;
        info.is_healthy             = IsHealthy();
        info.connected_clients      = GetConnectedClients();
        info.total_packets_sent     = total_packets_sent_.load();
        info.total_packets_received = total_packets_received_.load();
        info.worker_threads         = config_.worker_threads;
        info.max_clients            = config_.max_clients;
        info.uptime_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - server_start_time_)
                .count();

        return info;
    }


} // namespace bt
