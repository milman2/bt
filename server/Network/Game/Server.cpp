#include <iomanip>
#include <iostream>
#include <sstream>

#include "../../BT/Action/Action.h"
#include "../../BT/Condition/Condition.h"
#include "../../BT/Control/Selector.h"
#include "../../BT/Control/Sequence.h"
#include "../../BT/Tree.h"
#include "BT/Monster/MessageBasedMonsterManager.h"
#include "BT/Monster/MonsterBTExecutor.h"
#include "BT/Monster/MonsterTypes.h"
#include "Network/WebSocket/BeastHttpWebSocketServer.h"
#include "Network/WebSocket/NetworkMessageHandler.h"
#include "Player.h"
#include "Player/MessageBasedPlayerManager.h"
#include "Server.h"

namespace bt
{

    Server::Server(const AsioServerConfig& config)
        : config_(config)
        , running_(false)
        , acceptor_(io_context_)
        , total_packets_sent_(0)
        , total_packets_received_(0)
        , server_start_time_(std::chrono::steady_clock::now())
        , broadcast_running_(false)
    {
        // Behavior Tree 엔진 초기화
        bt_engine_ = std::make_shared<Engine>();

        // Behavior Tree 초기화
        // InitializeBehaviorTrees();

        // 매니저들 초기화 (메시지 큐 기반)

        // 메시지 큐 시스템 초기화
        message_processor_             = std::make_shared<GameMessageProcessor>();
        network_handler_               = std::make_shared<NetworkMessageHandler>();
        message_based_monster_manager_ = std::make_shared<MessageBasedMonsterManager>();
        message_based_player_manager_  = std::make_shared<MessageBasedPlayerManager>();

        // 통합 HTTP+WebSocket 서버 초기화 (설정에서 포트 가져오기)
        http_websocket_server_ = std::make_shared<BeastHttpWebSocketServer>(config_.http_websocket_port, io_context_);

        // HTTP 핸들러 등록
        RegisterHttpHandlers();

        // 메시지 기반 몬스터 매니저 설정
        message_based_monster_manager_->SetBTEngine(bt_engine_);
        message_based_monster_manager_->SetMessageProcessor(message_processor_);
        message_based_monster_manager_->SetPlayerManager(message_based_player_manager_);

        // 메시지 기반 플레이어 매니저 설정
        message_based_player_manager_->SetMessageProcessor(message_processor_);
        message_based_player_manager_->SetWebSocketServer(http_websocket_server_);

        // 네트워크 핸들러 설정
        network_handler_->SetWebSocketServer(http_websocket_server_);

        // 메시지 프로세서에 핸들러 등록
        // Note: 실제 구현에서는 핸들러를 unique_ptr로 관리해야 하지만,
        // 현재는 shared_ptr로 관리하므로 별도의 래퍼가 필요할 수 있습니다.
        // 일단 주석 처리하고 나중에 수정
        // message_processor_->RegisterGameHandler(
        //     std::unique_ptr<IGameMessageHandler>(message_based_monster_manager_.get()));
        // message_processor_->RegisterNetworkHandler(
        //     std::unique_ptr<IGameMessageHandler>(network_handler_.get()));

        LogMessage("Server 인스턴스가 생성되었습니다.");
    }

    Server::~Server()
    {
        Stop();
        LogMessage("Server 인스턴스가 소멸되었습니다.");
    }

    bool Server::Start()
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
            boost::asio::ip::tcp::endpoint endpoint;
            try
            {
                endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(config_.host), config_.port);
            }
            catch (const boost::system::system_error& e)
            {
                LogMessage("호스트 주소 설정 실패: " + std::string(e.what()), true);
                LogMessage("서버를 종료합니다.", true);
            return false;
        }

            // 어셉터 설정
            try
            {
                acceptor_.open(endpoint.protocol());
                acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

                // 바인딩 시도
                acceptor_.bind(endpoint);
                LogMessage("포트 " + std::to_string(config_.port) + "에 바인딩 성공");

                acceptor_.listen();
                LogMessage("포트 " + std::to_string(config_.port) + "에서 리스닝 시작");
            }
            catch (const boost::system::system_error& e)
            {
                LogMessage("포트 " + std::to_string(config_.port) + " 설정 실패: " + e.what(), true);
                LogMessage("서버를 종료합니다.", true);
                return false;
            }

            // running_ 플래그를 먼저 설정
            running_.store(true);

            // 서버 시작 시간 기록
            server_start_time_ = std::chrono::steady_clock::now();

            // 연결 수락 시작 (워커 스레드 시작 전에)
            try
            {
                StartAccept();
            }
            catch (const std::exception& e)
            {
                LogMessage("연결 수락 시작 실패: " + std::string(e.what()), true);
                LogMessage("서버를 종료합니다.", true);
                return false;
            }

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

            // 메시지 큐 시스템 시작
            if (message_processor_)
            {
                message_processor_->Start();
                LogMessage("메시지 큐 시스템 시작됨");
            }

            // 메시지 기반 몬스터 매니저 시작
            if (message_based_monster_manager_)
            {
                message_based_monster_manager_->Start();
                LogMessage("메시지 기반 몬스터 매니저 시작됨");
            }

            // 메시지 기반 플레이어 매니저 시작
            if (message_based_player_manager_)
            {
                message_based_player_manager_->Start();
                LogMessage("메시지 기반 플레이어 매니저 시작됨");
            }

            // 통합 HTTP+WebSocket 서버 시작
            if (http_websocket_server_)
            {
                http_websocket_server_->start();
            }
        
        // 브로드캐스팅 루프 시작
        StartBroadcastLoop();

        LogMessage("서버가 성공적으로 시작되었습니다. 포트: " + std::to_string(config_.port));
            LogMessage("통합 HTTP+WebSocket 서버: http://localhost:" + std::to_string(config_.http_websocket_port) +
                       " (대시보드 + API + WebSocket)");

        return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("서버 시작 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    void Server::Stop()
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

        // 브로드캐스팅 루프 중지
        StopBroadcastLoop();
        
            // 메시지 기반 몬스터 매니저 중지
            if (message_based_monster_manager_)
            {
                message_based_monster_manager_->Stop();
                LogMessage("메시지 기반 몬스터 매니저 중지됨");
            }

            // 메시지 기반 플레이어 매니저 중지
            if (message_based_player_manager_)
            {
                message_based_player_manager_->Stop();
                LogMessage("메시지 기반 플레이어 매니저 중지됨");
            }

            // 메시지 큐 시스템 중지
            if (message_processor_)
            {
                message_processor_->Stop();
                LogMessage("메시지 큐 시스템 중지됨");
            }

            // 통합 HTTP+WebSocket 서버 중지
            if (http_websocket_server_)
            {
                http_websocket_server_->stop();
            }

            LogMessage("서버가 종료되었습니다.");
        }
        catch (const std::exception& e)
        {
            LogMessage("서버 종료 중 오류: " + std::string(e.what()), true);
        }
    }

    void Server::StartAccept()
    {
        if (!running_.load())
        {
            LogMessage("서버가 실행 중이 아니므로 연결 수락을 시작하지 않습니다.", true);
            return;
        }

        auto client = boost::make_shared<Client>(io_context_, this);

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

    void Server::HandleAccept(boost::shared_ptr<Client> client, const boost::system::error_code& error)
    {
        LogMessage("handle_accept 호출됨, 에러: " + (error ? error.message() : "없음"));

        if (!error)
        {
            LogMessage("클라이언트 연결 성공, IP: " + client->GetIPAddress() + ":" + std::to_string(client->GetPort()));

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

            LogMessage("새 클라이언트 연결 완료: " + client->GetIPAddress() + ":" + std::to_string(client->GetPort()));
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

    void Server::AddClient(boost::shared_ptr<Client> client)
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

        // 클라이언트 연결 시 플레이어는 생성하지 않음
        // PLAYER_JOIN 패킷에서 플레이어 생성

        // 클라이언트 패킷 수신 시작
        LogMessage("클라이언트 연결 완료: " + info.ip_address + ":" + std::to_string(info.port));
        client->ReceivePacket();
        LogMessage("클라이언트 패킷 수신 시작: " + info.ip_address);
    }

    void Server::RemoveClient(boost::shared_ptr<Client> client)
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
        if (message_based_player_manager_ && client_id != 0)
        {
            message_based_player_manager_->RemovePlayerByClientID(client_id);
        }

        if (!client_info.empty())
        {
            LogMessage("클라이언트 연결 종료: " + client_info);
        }
    }

    void Server::BroadcastPacket(const Packet& packet, boost::shared_ptr<Client> exclude_client)
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

    void Server::SendPacket(boost::shared_ptr<Client> client, const Packet& packet)
    {
        if (client && client->IsConnected())
        {
            LogMessage("패킷 전송: type=" + std::to_string(packet.type) +
                       ", size=" + std::to_string(packet.data.size()));
            client->SendPacket(packet);
            total_packets_sent_.fetch_add(1);
        }
    }

    void Server::ProcessPacket(boost::shared_ptr<Client> client, const Packet& packet)
    {
        total_packets_received_.fetch_add(1);

        LogMessage("패킷 수신: 타입=" + std::to_string(packet.type) + ", 크기=" + std::to_string(packet.size) +
                   ", 데이터크기=" + std::to_string(packet.data.size()) + ", 클라이언트=" + client->GetIPAddress());

        // 패킷 타입에 따른 처리
        switch (static_cast<PacketType>(packet.type))
        {
            case PacketType::CONNECT_REQ:
                // 연결 요청 처리
                LogMessage("연결 요청 수신: " + client->GetIPAddress());
                // 연결 응답 전송
                SendConnectResponse(client);
                break;

            case PacketType::PLAYER_JOIN_REQ:
                // 플레이어 참여 요청 처리
                LogMessage("플레이어 참여 요청 수신: " + client->GetIPAddress() +
                           ", 데이터 크기: " + std::to_string(packet.data.size()));
                HandlePlayerJoin(client, packet);
                break;

            case PacketType::PLAYER_MOVE_REQ:
                // 플레이어 이동 요청 처리
                LogMessage("플레이어 이동 요청 수신: " + client->GetIPAddress() +
                           ", 데이터 크기: " + std::to_string(packet.data.size()));
                HandlePlayerMove(client, packet);
                break;

            case PacketType::MONSTER_UPDATE_EVT:
                // 몬스터 업데이트 처리
                LogMessage("몬스터 업데이트 요청 수신");
                // 몬스터 업데이트 응답 전송
                SendMonsterUpdateResponse(client, true);
                break;

            case PacketType::BT_EXECUTE_REQ:
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

    void Server::SendConnectResponse(boost::shared_ptr<Client> client)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RES);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success = 1; // 성공
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("연결 응답 전송 완료");
    }

    void Server::SendMonsterSpawnResponse(boost::shared_ptr<Client> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RES);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("몬스터 스폰 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void Server::SendMonsterUpdateResponse(boost::shared_ptr<Client> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RES);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("몬스터 업데이트 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void Server::SendBTExecuteResponse(boost::shared_ptr<Client> client, bool success)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::CONNECT_RES);
        response.size = sizeof(uint32_t) * 2; // type + success
        response.data.resize(response.size);

        uint32_t success_value = success ? 1 : 0;
        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), &success_value, sizeof(uint32_t));

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("BT 실행 응답 전송 완료: " + std::string(success ? "성공" : "실패"));
    }

    void Server::SendErrorResponse(boost::shared_ptr<Client> client, const std::string& error_message)
    {
        Packet response;
        response.type = static_cast<uint32_t>(PacketType::ERROR_MESSAGE_EVT);
        response.size = sizeof(uint32_t) + error_message.length(); // type + message
        response.data.resize(response.size);

        memcpy(response.data.data(), &response.type, sizeof(uint32_t));
        memcpy(response.data.data() + sizeof(uint32_t), error_message.c_str(), error_message.length());

        client->SendPacket(response);
        total_packets_sent_.fetch_add(1);
        LogMessage("에러 응답 전송 완료: " + error_message);
    }

    size_t Server::GetConnectedClients() const
    {
        boost::lock_guard<boost::mutex> lock(clients_mutex_);
        return clients_.size();
    }

    void Server::LogMessage(const std::string& message, bool is_error)
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
    bool Server::IsHealthy() const
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
    ServerHealthInfo Server::GetHealthInfo() const
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

    void Server::HandlePlayerJoin(boost::shared_ptr<Client> client, const Packet& packet)
    {
        try
        {
            LogMessage("HandlePlayerJoin 시작: 클라이언트=" + client->GetIPAddress() +
                       ", 패킷 데이터 크기=" + std::to_string(packet.data.size()));

            // 패킷 데이터 파싱
            const uint8_t* data   = packet.data.data();
            size_t         offset = 0;

            // 이름 길이 읽기
            uint32_t name_len = *reinterpret_cast<const uint32_t*>(data + offset);
            offset += sizeof(uint32_t);

            LogMessage("이름 길이: " + std::to_string(name_len));

            // 이름 읽기
            std::string player_name(data + offset, data + offset + name_len);
            offset += name_len;

            LogMessage("플레이어 이름: " + player_name);

            // 위치 읽기
            float x = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float y = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float z = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float rotation = *reinterpret_cast<const float*>(data + offset);

            LogMessage("플레이어 참여 요청: " + player_name + " 위치(" + std::to_string(x) + ", " + std::to_string(y) +
                       ", " + std::to_string(z) + ")");

            // 플레이어 매니저를 통해 플레이어 생성
            if (message_based_player_manager_)
            {
                // MonsterPosition 구조체 생성
                MonsterPosition position;
                position.x        = x;
                position.y        = y;
                position.z        = z;
                position.rotation = rotation;

                auto player = message_based_player_manager_->CreatePlayer(player_name, position);
                if (player)
                {
                    // 성공 응답 전송
                    SendPlayerJoinResponse(client, true, player->GetID());

                    LogMessage("플레이어 생성 성공: " + player_name + " (ID: " + std::to_string(player->GetID()) + ")");
                }
                else
                {
                    // 실패 응답 전송
                    SendPlayerJoinResponse(client, false, 0);
                    LogMessage("플레이어 생성 실패: " + player_name, true);
                }
            }
            else
            {
                // 플레이어 매니저가 없는 경우
                SendPlayerJoinResponse(client, false, 0);
                LogMessage("플레이어 매니저가 초기화되지 않음", true);
            }
        }
        catch (const std::exception& e)
        {
            LogMessage("플레이어 참여 처리 중 오류: " + std::string(e.what()), true);
            SendPlayerJoinResponse(client, false, 0);
        }
    }

    void Server::HandlePlayerMove(boost::shared_ptr<Client> client, const Packet& packet)
    {
        try
        {
            LogMessage("HandlePlayerMove 시작: 클라이언트=" + client->GetIPAddress() +
                       ", 패킷 데이터 크기=" + std::to_string(packet.data.size()));

            // 패킷 데이터 파싱
            const uint8_t* data   = packet.data.data();
            size_t         offset = 0;

            // 플레이어 ID 읽기
            uint32_t player_id = *reinterpret_cast<const uint32_t*>(data + offset);
            offset += sizeof(uint32_t);

            // 위치 읽기
            float x = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float y = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float z = *reinterpret_cast<const float*>(data + offset);
            offset += sizeof(float);
            float rotation = *reinterpret_cast<const float*>(data + offset);

            LogMessage("플레이어 이동 요청: ID=" + std::to_string(player_id) + " 위치(" + std::to_string(x) + ", " +
                       std::to_string(y) + ", " + std::to_string(z) + ")");

            // 플레이어 매니저를 통해 플레이어 위치 업데이트
            if (message_based_player_manager_)
            {
                auto player = message_based_player_manager_->GetPlayer(player_id);
                if (player)
                {
                    // 플레이어 위치 업데이트
                    player->SetPosition(x, y, z, rotation);
                    LogMessage("플레이어 위치 업데이트 성공: ID=" + std::to_string(player_id));
                }
                else
                {
                    LogMessage("플레이어를 찾을 수 없음: ID=" + std::to_string(player_id), true);
                }
            }
            else
            {
                LogMessage("플레이어 매니저가 초기화되지 않음", true);
            }
        }
        catch (const std::exception& e)
        {
            LogMessage("플레이어 이동 처리 중 오류: " + std::string(e.what()), true);
        }
    }

    void Server::SendPlayerJoinResponse(boost::shared_ptr<Client> client, bool success, uint32_t player_id)
    {
        LogMessage("PLAYER_JOIN_RESPONSE 전송 시작: success=" + std::to_string(success) +
                   ", player_id=" + std::to_string(player_id));

        std::vector<uint8_t> data;

        // 성공 여부
        data.insert(
            data.end(), reinterpret_cast<uint8_t*>(&success), reinterpret_cast<uint8_t*>(&success) + sizeof(bool));

        // 플레이어 ID
        data.insert(data.end(),
                    reinterpret_cast<uint8_t*>(&player_id),
                    reinterpret_cast<uint8_t*>(&player_id) + sizeof(uint32_t));

        Packet response(static_cast<uint16_t>(PacketType::PLAYER_JOIN_RES), data);
        SendPacket(client, response);

        LogMessage("PLAYER_JOIN_RESPONSE 전송 완료");
    }

    void Server::RegisterHttpHandlers()
    {
        if (!http_websocket_server_)
            return;

        // 루트 경로 - 대시보드 HTML
        http_websocket_server_->register_get_handler("/",
                                                     [this](const http_request& req, http_response& res)
                                                     {
                                                         std::string html = R"(
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BT MMORPG 서버 대시보드</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }
        .stat-card { background: #f8f9fa; padding: 20px; border-radius: 8px; text-align: center; border-left: 4px solid #007bff; }
        .stat-value { font-size: 2em; font-weight: bold; color: #007bff; }
        .stat-label { color: #666; margin-top: 5px; }
        .monster-list { background: #f8f9fa; padding: 20px; border-radius: 8px; }
        .monster-item { background: white; padding: 15px; margin: 10px 0; border-radius: 5px; border-left: 4px solid #28a745; }
        .status { padding: 4px 8px; border-radius: 4px; font-size: 0.8em; font-weight: bold; }
        .status.active { background: #d4edda; color: #155724; }
        .status.inactive { background: #f8d7da; color: #721c24; }
        .connection-status { position: fixed; top: 20px; right: 20px; padding: 10px 20px; border-radius: 20px; font-weight: bold; }
        .connected { background: #d4edda; color: #155724; }
        .disconnected { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="connection-status disconnected" id="connectionStatus">연결 끊김</div>
    <div class="container">
        <div class="header">
            <h1>🐉 BT MMORPG 서버 대시보드</h1>
            <p>실시간 몬스터 AI 및 서버 상태 모니터링</p>
        </div>
        
        <div class="stats">
            <div class="stat-card">
                <div class="stat-value" id="totalMonsters">0</div>
                <div class="stat-label">총 몬스터</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="activeMonsters">0</div>
                <div class="stat-label">활성 몬스터</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="totalPlayers">0</div>
                <div class="stat-label">총 플레이어</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="activePlayers">0</div>
                <div class="stat-label">활성 플레이어</div>
            </div>
        </div>
        
        <div class="monster-list">
            <h2>몬스터 상태</h2>
            <div id="monsterList">로딩 중...</div>
        </div>
    </div>

    <script>
        let ws = null;
        let reconnectInterval = null;

        function connect() {
            ws = new WebSocket('ws://localhost:' + config_.http_websocket_port + '/');
            
            ws.onopen = function() {
                console.log('WebSocket 연결됨');
                document.getElementById('connectionStatus').textContent = '연결됨';
                document.getElementById('connectionStatus').className = 'connection-status connected';
                if (reconnectInterval) {
                    clearInterval(reconnectInterval);
                    reconnectInterval = null;
                }
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    if (data.type === 'monster_update') {
                        updateMonsterList(data.monsters);
                    } else if (data.type === 'system_message') {
                        console.log('시스템 메시지:', data.data.message);
                    }
                } catch (e) {
                    console.error('메시지 파싱 오류:', e);
                }
            };
            
            ws.onclose = function() {
                console.log('WebSocket 연결 끊김');
                document.getElementById('connectionStatus').textContent = '연결 끊김';
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                if (!reconnectInterval) {
                    reconnectInterval = setInterval(connect, 3000);
                }
            };
            
            ws.onerror = function(error) {
                console.error('WebSocket 오류:', error);
            };
        }

        function updateMonsterList(monsters) {
            const container = document.getElementById('monsterList');
            const totalMonsters = monsters.length;
            const activeMonsters = monsters.filter(m => m.is_active).length;
            
            document.getElementById('totalMonsters').textContent = totalMonsters;
            document.getElementById('activeMonsters').textContent = activeMonsters;
            
            if (monsters.length === 0) {
                container.innerHTML = '<p>활성 몬스터가 없습니다.</p>';
                return;
            }
            
            container.innerHTML = monsters.map(monster => `
                <div class="monster-item">
                    <h3>${monster.name} (${monster.type})</h3>
                    <p><strong>위치:</strong> (${monster.position.x.toFixed(2)}, ${monster.position.z.toFixed(2)})</p>
                    <p><strong>체력:</strong> ${monster.health}/${monster.max_health}</p>
                    <p><strong>AI:</strong> ${monster.ai_name} (${monster.bt_name})</p>
                    <p><strong>상태:</strong> <span class="status ${monster.is_active ? 'active' : 'inactive'}">${monster.is_active ? '활성' : '비활성'}</span></p>
                </div>
            `).join('');
        }

        // 페이지 로드 시 연결 시작
        connect();
    </script>
</body>
</html>
            )";
                                                         res = http_websocket_server_->create_http_response(
                                                             boost::beast::http::status::ok, html, "text/html");
                                                     });

        // API 엔드포인트들
        http_websocket_server_->register_get_handler(
            "/api/monsters",
            [this](const http_request& req, http_response& res)
            {
                if (!message_based_monster_manager_)
                {
                    res = http_websocket_server_->create_error_response(
                        boost::beast::http::status::internal_server_error, "MonsterManager not available");
                    return;
                }

                // MonsterType을 문자열로 변환하는 함수
                auto monster_type_to_string = [](MonsterType type) -> std::string
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
                };

                // 실제 몬스터 정보를 JSON으로 반환
                auto        monsters = message_based_monster_manager_->GetAllMonsters();
                std::string json     = "{\"monsters\":[";

                bool first = true;
                for (const auto& monster : monsters)
                {
                    if (!first)
                        json += ",";
                    json += "{";
                    json += "\"id\":" + std::to_string(monster->GetID()) + ",";
                    json += "\"name\":\"" + monster->GetName() + "\",";
                    json += "\"type\":\"" + monster_type_to_string(monster->GetType()) + "\",";
                    json += "\"position\":{\"x\":" + std::to_string(monster->GetPosition().x) +
                            ",\"y\":" + std::to_string(monster->GetPosition().y) +
                            ",\"z\":" + std::to_string(monster->GetPosition().z) + "},";
                    json += "\"health\":" + std::to_string(monster->GetStats().health) + ",";
                    json += "\"max_health\":" + std::to_string(monster->GetMaxHealth()) + ",";
                    json += "\"is_active\":" + std::string(monster->IsAlive() ? "true" : "false");
                    json += "}";
                    first = false;
                }

                json += "],\"total\":" + std::to_string(monsters.size()) + "}";
                res = http_websocket_server_->create_json_response(json);
            });

        http_websocket_server_->register_get_handler(
            "/api/stats",
            [this](const http_request& req, http_response& res)
            {
                // 실제 서버 통계 정보를 JSON으로 반환
                auto monsters = message_based_monster_manager_ ? message_based_monster_manager_->GetAllMonsters()
                                                               : std::vector<std::shared_ptr<Monster>>();
                auto players  = message_based_player_manager_ ? message_based_player_manager_->GetAllPlayers()
                                                              : std::vector<std::shared_ptr<Player>>();

                int active_monsters = 0;
                for (const auto& monster : monsters)
                {
                    if (monster && monster->IsAlive())
                    {
                        active_monsters++;
                    }
                }

                int active_players = 0;
                for (const auto& player : players)
                {
                    if (player && player->IsAlive())
                    {
                        active_players++;
                    }
                }

                // 서버 업타임 계산 (초 단위)
                auto now             = std::chrono::steady_clock::now();
                auto uptime_duration = std::chrono::duration_cast<std::chrono::seconds>(now - server_start_time_);
                int  uptime_seconds  = static_cast<int>(uptime_duration.count());

                std::string json = "{";
                json += "\"total_monsters\":" + std::to_string(monsters.size()) + ",";
                json += "\"active_monsters\":" + std::to_string(active_monsters) + ",";
                json += "\"total_players\":" + std::to_string(players.size()) + ",";
                json += "\"active_players\":" + std::to_string(active_players) + ",";
                json += "\"server_uptime\":" + std::to_string(uptime_seconds) + ",";
                json += "\"connected_clients\":" + std::to_string(clients_.size());
                json += "}";

                res = http_websocket_server_->create_json_response(json);
            });

        LogMessage("HTTP 핸들러 등록 완료");
    }

    void Server::StartBroadcastLoop()
    {
        if (broadcast_running_.load())
            return;

        broadcast_running_.store(true);
        last_broadcast_time_ = std::chrono::steady_clock::now();
        broadcast_thread_    = boost::thread(&Server::BroadcastLoopThread, this);
        
        LogMessage("월드 상태 브로드캐스팅 루프 시작됨 (FPS: 10)");
    }

    void Server::StopBroadcastLoop()
    {
        if (!broadcast_running_.load())
            return;

        broadcast_running_.store(false);
        
        if (broadcast_thread_.joinable())
        {
            broadcast_thread_.join();
        }
        
        LogMessage("월드 상태 브로드캐스팅 루프 중지됨");
    }

    void Server::BroadcastLoopThread()
    {
        const float target_frame_time = 1.0f / 10.0f; // 10 FPS
        
        while (broadcast_running_.load())
        {
            auto current_time = std::chrono::steady_clock::now();
            auto delta_time   = std::chrono::duration<float>(current_time - last_broadcast_time_).count();
            
            if (delta_time >= target_frame_time)
            {
                BroadcastWorldState();
                last_broadcast_time_ = current_time;
            }
            else
            {
                // 프레임 시간 조절
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
            }
        }
    }

    void Server::BroadcastWorldState()
    {
        // 클라이언트가 없으면 브로드캐스팅하지 않음
        {
            boost::lock_guard<boost::mutex> lock(clients_mutex_);
            if (clients_.empty())
                return;
        }

        // 월드 상태 수집
        bt::WorldStateBroadcastEvt world_state;
        world_state.set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());

        // 플레이어 상태 수집
        std::vector<bt::PlayerState> players;
            // TODO: 실제 플레이어 데이터 수집 구현
        world_state.set_player_count(0);

        // 몬스터 상태 수집 (MessageBasedMonsterManager에서)
        std::vector<bt::MonsterState> monsters;
        if (message_based_monster_manager_)
        {
            monsters = message_based_monster_manager_->GetMonsterStates();
            world_state.set_monster_count(monsters.size());
        }
        else
        {
            world_state.set_monster_count(0);
        }

        // 월드 상태 직렬화
        std::vector<uint8_t> serialized_data;
        
        // 타임스탬프
        uint64_t timestamp = world_state.timestamp();
        serialized_data.insert(serialized_data.end(), 
                              reinterpret_cast<uint8_t*>(&timestamp), 
                              reinterpret_cast<uint8_t*>(&timestamp) + sizeof(timestamp));
        
        // 플레이어 수
        uint32_t player_count = world_state.player_count();
        serialized_data.insert(serialized_data.end(), 
                              reinterpret_cast<uint8_t*>(&player_count), 
                              reinterpret_cast<uint8_t*>(&player_count) + sizeof(player_count));
        
        // 몬스터 수
        uint32_t monster_count = world_state.monster_count();
        serialized_data.insert(serialized_data.end(), 
                              reinterpret_cast<uint8_t*>(&monster_count), 
                              reinterpret_cast<uint8_t*>(&monster_count) + sizeof(monster_count));

        // 플레이어 데이터 추가
        for (const auto& player : players)
        {
            uint32_t id = player.id();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&id), 
                                  reinterpret_cast<const uint8_t*>(&id) + sizeof(id));
            
            float x = player.x(), y = player.y(), z = player.z();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&x), 
                                  reinterpret_cast<const uint8_t*>(&x) + sizeof(x));
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&y), 
                                  reinterpret_cast<const uint8_t*>(&y) + sizeof(y));
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&z), 
                                  reinterpret_cast<const uint8_t*>(&z) + sizeof(z));
            
            uint32_t health = player.health();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&health), 
                                  reinterpret_cast<const uint8_t*>(&health) + sizeof(health));
        }

        // 몬스터 데이터 추가
        for (const auto& monster : monsters)
        {
            uint32_t id = monster.id();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&id), 
                                  reinterpret_cast<const uint8_t*>(&id) + sizeof(id));
            
            float x = monster.x(), y = monster.y(), z = monster.z();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&x), 
                                  reinterpret_cast<const uint8_t*>(&x) + sizeof(x));
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&y), 
                                  reinterpret_cast<const uint8_t*>(&y) + sizeof(y));
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&z), 
                                  reinterpret_cast<const uint8_t*>(&z) + sizeof(z));
            
            uint32_t health = monster.health();
            serialized_data.insert(serialized_data.end(), 
                                  reinterpret_cast<const uint8_t*>(&health), 
                                  reinterpret_cast<const uint8_t*>(&health) + sizeof(health));
        }

        // 패킷 생성 및 브로드캐스팅
        Packet world_packet(static_cast<uint16_t>(PacketType::WORLD_STATE_BROADCAST_EVT), serialized_data);
        BroadcastPacket(world_packet);
        
        // 디버그 로그 (1초마다)
        static auto last_log_time = std::chrono::steady_clock::now();
        auto        now           = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1)
        {
            LogMessage("월드 상태 브로드캐스팅: 플레이어 " + std::to_string(world_state.player_count()) +
                       "명, 몬스터 " + std::to_string(world_state.monster_count()) + "마리");
            last_log_time = now;
        }
    }

} // namespace bt
