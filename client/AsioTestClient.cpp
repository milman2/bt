#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <cmath>
#include <random>

#include "AsioTestClient.h"

namespace bt
{

    AsioTestClient::AsioTestClient(const PlayerAIConfig& config)
        : config_(config), connected_(false), verbose_(false), ai_running_(false),
          bt_name_("player_bt"), position_(config.spawn_x, 0, config.spawn_z), 
          spawn_position_(config.spawn_x, 0, config.spawn_z), current_patrol_index_(0), 
          player_id_(0), target_id_(0), health_(config.health), 
          max_health_(config.health), last_attack_time_(0.0f), attack_cooldown_(2.0f),
          last_monster_update_(0.0f), network_running_(false)
    {
        socket_ = boost::make_shared<boost::asio::ip::tcp::socket>(io_context_);
        
        // 비동기 네트워크 버퍼 초기화
        read_buffer_.resize(4096);
        write_buffer_.resize(4096);
        
        // 순찰점 생성 (스폰 위치 주변)
        CreatePatrolPoints();
        
        // Behavior Tree 생성
        behavior_tree_ = PlayerBTs::CreatePlayerBT();
        // context_.SetAI는 나중에 설정됨 (shared_from_this() 사용을 위해)
        
        // 환경 인지 정보 초기화
        environment_info_.Clear();
        context_.SetEnvironmentInfo(&environment_info_);
        
        LogMessage("AI 플레이어 클라이언트 생성됨: " + config_.player_name);
    }
    
    void AsioTestClient::SetContextAI()
    {
        // shared_from_this() 사용을 위해 context에 AI 설정
        context_.SetAI(shared_from_this());
        
        // 메시지 큐 시스템 초기화 (shared_from_this() 사용 가능한 시점)
        InitializeMessageQueue();
    }

    AsioTestClient::~AsioTestClient()
    {
        StopAI();
        StopAsyncNetwork();
        Disconnect();
        ShutdownMessageQueue();
        LogMessage("AI 플레이어 클라이언트 소멸됨");
    }

    void AsioTestClient::CreatePatrolPoints()
    {
        patrol_points_.clear();
        
        // 스폰 위치를 중심으로 4방향 순찰점 생성
        float radius = config_.patrol_radius;
        
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z});
        patrol_points_.push_back({spawn_position_.x + radius, spawn_position_.y, spawn_position_.z});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z + radius});
        patrol_points_.push_back({spawn_position_.x - radius, spawn_position_.y, spawn_position_.z});
        patrol_points_.push_back({spawn_position_.x, spawn_position_.y, spawn_position_.z - radius});
        
        current_patrol_index_ = 0;
        
        LogMessage("순찰점 " + std::to_string(patrol_points_.size()) + "개 생성 완료");
    }

    bool AsioTestClient::Connect()
    {
        if (connected_.load())
        {
            LogMessage("이미 연결되어 있습니다", true);
            return true;
        }

        LogMessage("서버에 연결 시도 중: " + config_.server_host + ":" + std::to_string(config_.server_port));

        if (!CreateConnection())
        {
            LogMessage("서버 연결 실패", true);
            return false;
        }

        connected_.store(true);
        LogMessage("서버 연결 성공");

        // 비동기 네트워크 시작
        StartAsyncNetwork();

        // 게임 참여 시도 (실패해도 연결 유지)
        if (!JoinGame())
        {
            LogMessage("게임 참여 실패 - 서버 응답 없음, 오프라인 모드로 AI 시작", true);
            // 연결은 유지하고 AI는 시작
        }

        return true;
    }

    void AsioTestClient::Disconnect()
    {
        if (!connected_.load())
            return;

        StopAI();
        StopAsyncNetwork();
        
        if (socket_ && socket_->is_open())
        {
            try
            {
                Packet disconnect_packet = CreateDisconnectPacket();
                SendPacket(disconnect_packet);
                socket_->close();
            }
            catch (const std::exception& e)
            {
                LogMessage("연결 종료 중 오류: " + std::string(e.what()), true);
            }
        }

        connected_.store(false);
        LogMessage("서버 연결 종료");
    }

    bool AsioTestClient::CreateConnection()
    {
        try
        {
            boost::asio::ip::tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(config_.server_host, std::to_string(config_.server_port));

            boost::asio::connect(*socket_, endpoints);
            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("연결 생성 실패: " + std::string(e.what()), true);
            return false;
        }
    }

    void AsioTestClient::CloseConnection()
    {
        if (socket_ && socket_->is_open())
        {
            try
            {
                socket_->close();
            }
            catch (const std::exception& e)
            {
                LogMessage("연결 종료 중 오류: " + std::string(e.what()), true);
            }
        }
    }

    bool AsioTestClient::JoinGame()
    {
        Packet join_packet = CreatePlayerJoinPacket(config_.player_name);
        if (!SendPacket(join_packet))
        {
            LogMessage("게임 참여 패킷 전송 실패", true);
            return false;
        }

        LogMessage("게임 참여 패킷 전송 완료, 응답 대기 중...");

        // 응답 대기 (타임아웃 포함)
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(5);
        
        while (std::chrono::steady_clock::now() - start_time < timeout)
        {
            Packet response;
            if (ReceivePacket(response))
            {
                if (ParsePacketResponse(response))
                {
                    LogMessage("게임 참여 성공: " + config_.player_name);
                    return true;
                }
            }
            
            // 짧은 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        LogMessage("게임 참여 타임아웃", true);
            return false;
    }

    bool AsioTestClient::MoveTo(float x, float y, float z)
    {
        // 위치 업데이트 (연결 상태와 관계없이)
        position_.x = x;
        position_.y = y;
        position_.z = z;

        // 서버에 전송 (연결된 경우에만)
        if (connected_.load())
        {
            Packet move_packet = CreatePlayerMovePacket(x, y, z);
            if (SendPacket(move_packet))
            {
                if (verbose_)
                {
                    LogMessage("이동: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
                }
                return true;
            }
            else
            {
                if (verbose_)
                {
                    LogMessage("이동 패킷 전송 실패, 로컬 위치만 업데이트: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
                }
            }
        }
        else
        {
            if (verbose_)
            {
                LogMessage("오프라인 모드 - 이동: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
            }
        }
        
        return true; // 로컬 위치 업데이트는 항상 성공
    }

    bool AsioTestClient::AttackTarget(uint32_t target_id)
    {
        if (!IsAlive())
            return false;

        float current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;

        if (current_time - last_attack_time_ < attack_cooldown_)
            return false;

        // 공격 쿨다운 업데이트 (연결 상태와 관계없이)
        last_attack_time_ = current_time;

        // 서버에 전송 (연결된 경우에만)
        if (connected_.load())
        {
            Packet attack_packet = CreatePlayerAttackPacket(target_id);
            if (SendPacket(attack_packet))
            {
                if (verbose_)
                {
                    LogMessage("공격: 타겟 ID " + std::to_string(target_id));
                }
                return true;
            }
            else
            {
                if (verbose_)
                {
                    LogMessage("공격 패킷 전송 실패, 로컬 공격만 실행: 타겟 ID " + std::to_string(target_id));
                }
            }
        }
        else
        {
            if (verbose_)
            {
                LogMessage("오프라인 모드 - 공격: 타겟 ID " + std::to_string(target_id));
            }
        }
        
        return true; // 로컬 공격은 항상 성공
    }

    bool AsioTestClient::Respawn()
    {
        // 부활 처리 (연결 상태와 관계없이)
        position_ = spawn_position_;
        health_ = max_health_;
        target_id_ = 0;

        // MoveTo는 이미 연결 상태를 확인하므로 그대로 호출
        if (MoveTo(spawn_position_.x, spawn_position_.y, spawn_position_.z))
        {
            LogMessage("부활 완료: " + config_.player_name);
            return true;
        }
        return false;
    }

    void AsioTestClient::StartAI()
    {
        if (ai_running_.load())
            return;

        ai_running_.store(true);
        LogMessage("AI 시작: " + config_.player_name);
    }

    void AsioTestClient::StopAI()
    {
        if (!ai_running_.load())
            return;

        ai_running_.store(false);
        LogMessage("AI 중지: " + config_.player_name);
    }

    void AsioTestClient::UpdateAI(float delta_time)
    {
        if (!ai_running_.load())
            return;

        // AI 업데이트 호출 확인 (매 틱마다 로깅)
        static int update_count = 0;
        update_count++;
        if (verbose_ && update_count % 50 == 0)
        {
            LogMessage("AI 업데이트 호출됨 (카운트: " + std::to_string(update_count) + ")");
        }

        // 패킷 수신 처리 (메시지 큐로 전송) - 연결된 경우에만
        if (connected_.load())
        {
            Packet packet;
            while (ReceivePacket(packet))
            {
                // 패킷을 메시지 큐로 전송
                if (message_processor_)
                {
                    auto packet_msg = std::make_shared<NetworkPacketMessage>(packet.data, packet.type);
                    message_processor_->SendMessage(packet_msg);
                }
            }
        }

        // 환경 인지 정보 업데이트
        UpdateEnvironmentInfo();

        // Behavior Tree 실행
        if (behavior_tree_)
        {
            // 실행 카운트 증가
            context_.IncrementExecutionCount();
            
            // BT 실행
            NodeStatus status = behavior_tree_->Execute(context_);
            
            // 실행 상태 로깅 (디버깅용)
            if (verbose_ && context_.GetExecutionCount() % 10 == 0)
            {
                std::string status_str = (status == NodeStatus::SUCCESS) ? "SUCCESS" :
                                       (status == NodeStatus::FAILURE) ? "FAILURE" : "RUNNING";
                LogMessage("BT 실행 상태: " + status_str + " (실행 횟수: " + 
                          std::to_string(context_.GetExecutionCount()) + ")");
            }
        }
    }

    // IExecutor 인터페이스 구현
    void AsioTestClient::Update(float delta_time)
    {
        UpdateAI(delta_time);
    }

    void AsioTestClient::SetBehaviorTree(std::shared_ptr<Tree> tree)
    {
        behavior_tree_ = tree;
    }

    std::shared_ptr<Tree> AsioTestClient::GetBehaviorTree() const
    {
        return behavior_tree_;
    }

    Context& AsioTestClient::GetContext()
    {
        return context_;
    }

    const Context& AsioTestClient::GetContext() const
    {
        return context_;
    }

    const std::string& AsioTestClient::GetName() const
    {
        return config_.player_name;
    }

    const std::string& AsioTestClient::GetBTName() const
    {
        return bt_name_;
    }

    bool AsioTestClient::IsActive() const
    {
        return ai_running_.load();
    }

    void AsioTestClient::SetActive(bool active)
    {
        if (active)
        {
            StartAI();
        }
        else
        {
            StopAI();
        }
    }

    void AsioTestClient::UpdatePatrol(float delta_time)
    {
        if (HasTarget() || !IsAlive())
            return;

        if (patrol_points_.empty())
            return;

        auto target_point = patrol_points_[current_patrol_index_];
        
        // 목표 지점까지의 거리 계산
        float dx = target_point.x - position_.x;
        float dz = target_point.z - position_.z;
        float distance = std::sqrt(dx * dx + dz * dz);

        const float arrival_threshold = 5.0f;

        if (distance <= arrival_threshold)
        {
            // 다음 순찰점으로 이동
            current_patrol_index_ = (current_patrol_index_ + 1) % patrol_points_.size();
            if (verbose_)
            {
                LogMessage("순찰점 도착: " + std::to_string(current_patrol_index_));
            }
        }
        else
        {
            // 목표 지점으로 이동
            float step_size = config_.move_speed * delta_time;
            float normalized_dx = dx / distance;
            float normalized_dz = dz / distance;
            
            float new_x = position_.x + normalized_dx * step_size;
            float new_z = position_.z + normalized_dz * step_size;
            
            MoveTo(new_x, position_.y, new_z);
        }
    }

    void AsioTestClient::UpdateCombat(float delta_time)
    {
        if (!IsAlive())
        {
            // 사망 시 부활
            Respawn();
            return;
        }

        // 가장 가까운 몬스터 찾기 (BT에서 사용할 수 있도록 타겟 설정)
        FindNearestMonster();
    }

    void AsioTestClient::FindNearestMonster()
    {
        if (monsters_.empty())
            return;

        uint32_t nearest_id = 0;
        float nearest_distance = std::numeric_limits<float>::max();

        for (const auto& [id, pos] : monsters_)
        {
            float dx = pos.x - position_.x;
            float dz = pos.z - position_.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            if (distance < nearest_distance && distance <= config_.detection_range)
            {
                nearest_distance = distance;
                nearest_id = id;
            }
        }

        if (nearest_id != 0)
        {
            target_id_ = nearest_id;
        }
    }

    float AsioTestClient::GetDistanceToTarget() const
    {
        if (!HasTarget())
            return std::numeric_limits<float>::max();

        auto it = monsters_.find(target_id_);
        if (it == monsters_.end())
            return std::numeric_limits<float>::max();

        float dx = it->second.x - position_.x;
        float dz = it->second.z - position_.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    uint32_t AsioTestClient::GetNearestMonster() const
    {
        if (monsters_.empty())
            return 0;

        uint32_t nearest_id = 0;
        float nearest_distance = std::numeric_limits<float>::max();

        for (const auto& [id, pos] : monsters_)
        {
            float dx = pos.x - position_.x;
            float dz = pos.z - position_.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            if (distance < nearest_distance)
            {
                nearest_distance = distance;
                nearest_id = id;
            }
        }

        return nearest_id;
    }

    const PlayerPosition* AsioTestClient::GetMonsterPosition(uint32_t monster_id) const
    {
        auto it = monsters_.find(monster_id);
        if (it != monsters_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool AsioTestClient::IsInRange(float x, float z, float range) const
    {
        float dx = x - position_.x;
        float dz = z - position_.z;
        float distance = std::sqrt(dx * dx + dz * dz);
        return distance <= range;
    }

    PlayerPosition AsioTestClient::GetNextPatrolPoint() const
    {
        if (patrol_points_.empty())
        {
            return spawn_position_;
        }

        // 현재 순찰점을 반환 (인덱스는 변경하지 않음)
        return patrol_points_[current_patrol_index_];
    }

    void AsioTestClient::AdvanceToNextPatrolPoint()
    {
        if (!patrol_points_.empty())
        {
            current_patrol_index_ = (current_patrol_index_ + 1) % patrol_points_.size();
        }
    }

    bool AsioTestClient::SendPacket(const Packet& packet)
    {
        if (!connected_.load() || !socket_ || !socket_->is_open())
            return false;

        // 비동기 네트워크가 활성화된 경우 비동기 전송 사용
        if (network_running_.load())
        {
            AsyncSendPacket(packet);
            return true;
        }

        // 동기 전송 (fallback)
        try
        {
            // 패킷 헤더 생성
            uint32_t total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
            std::vector<uint8_t> buffer;
            buffer.resize(total_size);
            
            size_t offset = 0;
            
            // 크기 (4바이트)
            *reinterpret_cast<uint32_t*>(buffer.data() + offset) = total_size;
            offset += sizeof(uint32_t);
            
            // 타입 (2바이트)
            *reinterpret_cast<uint16_t*>(buffer.data() + offset) = packet.type;
            offset += sizeof(uint16_t);
            
            // 데이터
            std::memcpy(buffer.data() + offset, packet.data.data(), packet.data.size());
            
            boost::asio::write(*socket_, boost::asio::buffer(buffer));
            return true;
        }
        catch (const std::exception& e)
        {
            LogMessage("패킷 전송 실패: " + std::string(e.what()), true);
            
            // 서버 연결이 끊어진 경우 클라이언트 종료
            std::string error_msg = e.what();
            if (error_msg.find("Broken pipe") != std::string::npos ||
                error_msg.find("Connection reset") != std::string::npos ||
                error_msg.find("End of file") != std::string::npos)
            {
                LogMessage("서버 연결이 끊어졌습니다. 클라이언트를 종료합니다.", true);
                connected_.store(false);
                ai_running_.store(false);
            }
            
            return false;
        }
    }

    bool AsioTestClient::ReceivePacket(Packet& packet)
    {
        if (!connected_.load() || !socket_ || !socket_->is_open())
            return false;

        try
        {
            // 소켓을 논블로킹 모드로 설정
            socket_->non_blocking(true);
            
            // 패킷 크기 읽기
            uint32_t packet_size;
            boost::system::error_code ec;
            size_t bytes_read = socket_->read_some(boost::asio::buffer(&packet_size, sizeof(packet_size)), ec);
            
            if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
            {
                // 데이터가 없으면 false 반환
                return false;
            }
            
            if (bytes_read != sizeof(packet_size))
            {
                // 완전한 패킷 크기를 읽지 못한 경우
                return false;
            }

            if (ec)
            {
                LogMessage("패킷 크기 수신 오류: " + ec.message(), true);
                
                // 서버 연결이 끊어진 경우 클라이언트 종료
                if (ec == boost::asio::error::eof || 
                    ec == boost::asio::error::connection_reset ||
                    ec == boost::asio::error::broken_pipe)
                {
                    LogMessage("서버 연결이 끊어졌습니다. 클라이언트를 종료합니다.", true);
                    connected_.store(false);
                    ai_running_.store(false);
                }
                return false;
            }
            
            if (packet_size > sizeof(uint32_t))
            {
                // 패킷 데이터 읽기 (크기 - 헤더 크기)
                size_t data_size = packet_size - sizeof(uint32_t);
                std::vector<uint8_t> buffer(data_size);
                
                bytes_read = socket_->read_some(boost::asio::buffer(buffer), ec);
                
                if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
                {
                    // 데이터가 없으면 false 반환
                    return false;
                }
                
                if (ec)
                {
                    LogMessage("패킷 데이터 수신 오류: " + ec.message(), true);
                    return false;
                }
                
                if (bytes_read != data_size)
                {
                    // 완전한 패킷 데이터를 읽지 못한 경우
                    return false;
                }
                
                if (bytes_read == data_size && data_size >= sizeof(uint16_t))
                {
                    // 타입 읽기
                    uint16_t type = *reinterpret_cast<uint16_t*>(buffer.data());
                    
                    // 데이터 읽기
                    std::vector<uint8_t> data(buffer.begin() + sizeof(uint16_t), buffer.end());
                    
                    packet = Packet(type, data);
                    return true;
                }
            }
        }
        catch (const boost::system::system_error& e)
        {
            if (e.code() != boost::asio::error::would_block)
            {
                LogMessage("패킷 수신 실패: " + std::string(e.what()), true);
            }
        }
        catch (const std::exception& e)
        {
            LogMessage("패킷 수신 오류: " + std::string(e.what()), true);
        }

        return false;
    }

    Packet AsioTestClient::CreateConnectRequest()
    {
        std::vector<uint8_t> data;
        data.insert(data.end(), config_.player_name.begin(), config_.player_name.end());
        return Packet(static_cast<uint16_t>(PacketType::CONNECT_REQUEST), data);
    }

    Packet AsioTestClient::CreatePlayerJoinPacket(const std::string& name)
    {
        std::vector<uint8_t> data;

        // 이름 길이 + 이름
        uint32_t name_len = name.length();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&name_len), reinterpret_cast<uint8_t*>(&name_len) + sizeof(uint32_t));
        data.insert(data.end(), name.begin(), name.end());
        
        // 위치
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&position_.x), reinterpret_cast<uint8_t*>(&position_.x) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&position_.y), reinterpret_cast<uint8_t*>(&position_.y) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&position_.z), reinterpret_cast<uint8_t*>(&position_.z) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&position_.rotation), reinterpret_cast<uint8_t*>(&position_.rotation) + sizeof(float));

        return Packet(static_cast<uint16_t>(PacketType::PLAYER_JOIN), data);
    }

    Packet AsioTestClient::CreatePlayerMovePacket(float x, float y, float z)
    {
        std::vector<uint8_t> data;

        data.insert(data.end(), reinterpret_cast<uint8_t*>(&player_id_), reinterpret_cast<uint8_t*>(&player_id_) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&x), reinterpret_cast<uint8_t*>(&x) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&y), reinterpret_cast<uint8_t*>(&y) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&z), reinterpret_cast<uint8_t*>(&z) + sizeof(float));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&position_.rotation), reinterpret_cast<uint8_t*>(&position_.rotation) + sizeof(float));

        return Packet(static_cast<uint16_t>(PacketType::PLAYER_MOVE), data);
    }

    Packet AsioTestClient::CreatePlayerAttackPacket(uint32_t target_id)
    {
        std::vector<uint8_t> data;

        data.insert(data.end(), reinterpret_cast<uint8_t*>(&player_id_), reinterpret_cast<uint8_t*>(&player_id_) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&target_id), reinterpret_cast<uint8_t*>(&target_id) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&config_.damage), reinterpret_cast<uint8_t*>(&config_.damage) + sizeof(uint32_t));

        return Packet(static_cast<uint16_t>(PacketType::PLAYER_ATTACK), data);
    }

    Packet AsioTestClient::CreateDisconnectPacket()
    {
        std::vector<uint8_t> data;
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&player_id_), reinterpret_cast<uint8_t*>(&player_id_) + sizeof(uint32_t));
        return Packet(static_cast<uint16_t>(PacketType::DISCONNECT), data);
    }

    bool AsioTestClient::ParsePacketResponse(const Packet& packet)
    {
        if (packet.data.empty())
            return false;

        switch (static_cast<PacketType>(packet.type))
        {
            case PacketType::CONNECT_RESPONSE:
                {
                    if (packet.data.size() >= sizeof(uint8_t) + sizeof(uint32_t))
                    {
                        uint8_t success = packet.data[0];
                        if (success)
                        {
                            player_id_ = *reinterpret_cast<const uint32_t*>(packet.data.data() + 1);
                            LogMessage("서버 연결 응답 성공, 플레이어 ID: " + std::to_string(player_id_));
                            return true;
                        }
                        else
                        {
                            LogMessage("서버 연결 응답 실패", true);
                            return false;
                        }
                    }
                }
                break;

            case PacketType::PLAYER_JOIN_RESPONSE:
                {
                    if (packet.data.size() >= sizeof(bool) + sizeof(uint32_t))
                    {
                        bool success = *reinterpret_cast<const bool*>(packet.data.data());
                        if (success)
                        {
                            player_id_ = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(bool));
                            LogMessage("게임 참여 성공, 플레이어 ID: " + std::to_string(player_id_));
                            return true;
                        }
                        else
                        {
                            LogMessage("게임 참여 실패", true);
            return false;
                        }
                    }
                }
                break;

            case PacketType::MONSTER_UPDATE:
                HandleMonsterUpdate(packet);
                break;

            case PacketType::PLAYER_STATS:
                HandlePlayerUpdate(packet);
                break;

            case PacketType::BT_RESULT:
                HandleCombatResult(packet);
                break;

            default:
                if (verbose_)
                {
                    LogMessage("알 수 없는 패킷 타입: " + std::to_string(packet.type));
                }
                break;
        }

        return true;
    }

    void AsioTestClient::HandleMonsterUpdate(const Packet& packet)
    {
        if (packet.data.size() < sizeof(uint32_t))
            return;
            
        uint32_t monster_count = *reinterpret_cast<const uint32_t*>(packet.data.data());
        monsters_.clear();

        size_t offset = sizeof(uint32_t);
        for (uint32_t i = 0; i < monster_count && offset < packet.data.size(); ++i)
        {
            if (offset + sizeof(uint32_t) > packet.data.size())
                break;
                
            uint32_t id = *reinterpret_cast<const uint32_t*>(packet.data.data() + offset);
            offset += sizeof(uint32_t);
            
            if (offset + sizeof(float) * 4 > packet.data.size())
                break;
                
            float x = *reinterpret_cast<const float*>(packet.data.data() + offset);
            offset += sizeof(float);
            float y = *reinterpret_cast<const float*>(packet.data.data() + offset);
            offset += sizeof(float);
            float z = *reinterpret_cast<const float*>(packet.data.data() + offset);
            offset += sizeof(float);
            float rotation = *reinterpret_cast<const float*>(packet.data.data() + offset);
            offset += sizeof(float);

            monsters_[id] = PlayerPosition(x, y, z, rotation);
        }

        last_monster_update_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;
    }

    void AsioTestClient::HandlePlayerUpdate(const Packet& packet)
    {
        // 플레이어 업데이트 처리 (필요시 구현)
    }

    void AsioTestClient::HandleCombatResult(const Packet& packet)
    {
        if (packet.data.size() < sizeof(uint32_t) * 4)
            return;
            
        uint32_t attacker_id = *reinterpret_cast<const uint32_t*>(packet.data.data());
        uint32_t target_id = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(uint32_t));
        uint32_t damage = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(uint32_t) * 2);
        uint32_t remaining_health = *reinterpret_cast<const uint32_t*>(packet.data.data() + sizeof(uint32_t) * 3);

        if (attacker_id == player_id_)
        {
            if (verbose_)
            {
                LogMessage("공격 결과: 타겟 " + std::to_string(target_id) + 
                          ", 데미지 " + std::to_string(damage) + 
                          ", 남은 체력 " + std::to_string(remaining_health));
            }
        }
        else if (target_id == player_id_)
        {
            health_ = remaining_health;
            if (health_ <= 0)
            {
                LogMessage("플레이어 사망!", true);
                target_id_ = 0;
            }
        }
    }

    void AsioTestClient::LogMessage(const std::string& message, bool is_error)
    {
        boost::lock_guard<boost::mutex> lock(log_mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        
        if (is_error)
        {
            oss << "[ERROR] ";
        }
        else
        {
            oss << "[INFO] ";
        }
        
        oss << "[" << config_.player_name << "] " << message;
        
        std::cout << oss.str() << std::endl;
    }

    void AsioTestClient::UpdateEnvironmentInfo()
    {
        // 환경 인지 정보 초기화
        environment_info_.Clear();
        
        // 테스트용 가상 몬스터 추가 (서버에서 몬스터 정보를 받지 못하는 경우)
        if (monsters_.empty())
        {
            // 플레이어 위치 근처에 가상 몬스터 생성
            static bool test_monster_added = false;
            if (!test_monster_added)
            {
                // 플레이어 위치에서 10유닛 떨어진 곳에 몬스터 배치
                PlayerPosition monster_pos = position_;
                monster_pos.x += 10.0f; // X축으로 10유닛 이동
                monsters_[999] = monster_pos; // 테스트 몬스터 ID: 999
                test_monster_added = true;
                LogMessage("테스트용 몬스터 추가: ID 999, 위치 (" + 
                          std::to_string(monster_pos.x) + ", " + 
                          std::to_string(monster_pos.y) + ", " + 
                          std::to_string(monster_pos.z) + ")");
            }
        }
        
        // 주변 몬스터 정보 업데이트
        for (const auto& [monster_id, monster_pos] : monsters_)
        {
            float distance = std::sqrt(
                std::pow(monster_pos.x - position_.x, 2) + 
                std::pow(monster_pos.z - position_.z, 2)
            );
            
            // 탐지 범위 내의 몬스터만 추가
            if (distance <= config_.detection_range)
            {
                environment_info_.nearby_monsters.push_back(monster_id);
                
                // 가장 가까운 적 업데이트
                if (environment_info_.nearest_enemy_id == 0 || 
                    distance < environment_info_.nearest_enemy_distance)
                {
                    environment_info_.nearest_enemy_id = monster_id;
                    environment_info_.nearest_enemy_distance = distance;
                }
            }
        }
        
        // 시야 확보 여부 (간단한 구현 - 장애물이 없으면 시야 확보)
        environment_info_.has_line_of_sight = true; // TODO: 실제 장애물 검사 구현
        
        // 디버그 로깅
        if (verbose_ && !environment_info_.nearby_monsters.empty())
        {
            LogMessage("환경 인지: 주변 몬스터 " + std::to_string(environment_info_.nearby_monsters.size()) + 
                      "마리, 가장 가까운 적 ID: " + std::to_string(environment_info_.nearest_enemy_id) +
                      " (거리: " + std::to_string(environment_info_.nearest_enemy_distance) + ")");
        }
    }

    // 메시지 큐 관련 메서드들
    void AsioTestClient::InitializeMessageQueue()
    {
        // 메시지 프로세서 생성
        message_processor_ = std::make_shared<ClientMessageProcessor>();
        
        // 네트워크 핸들러 생성
        network_handler_ = std::make_shared<ClientNetworkMessageHandler>(shared_from_this());
        network_handler_->SetMessageProcessor(message_processor_);
        
        // AI 핸들러 생성
        ai_handler_ = std::make_shared<ClientAIMessageHandler>(shared_from_this());
        ai_handler_->SetMessageProcessor(message_processor_);
        
        // 핸들러 등록
        message_processor_->RegisterHandler(ClientMessageType::NETWORK_PACKET_RECEIVED, network_handler_);
        message_processor_->RegisterHandler(ClientMessageType::NETWORK_CONNECTION_LOST, network_handler_);
        message_processor_->RegisterHandler(ClientMessageType::NETWORK_CONNECTION_ESTABLISHED, network_handler_);
        message_processor_->RegisterHandler(ClientMessageType::AI_UPDATE_REQUEST, ai_handler_);
        message_processor_->RegisterHandler(ClientMessageType::AI_STATE_CHANGE, ai_handler_);
        message_processor_->RegisterHandler(ClientMessageType::PLAYER_ACTION_REQUEST, ai_handler_);
        message_processor_->RegisterHandler(ClientMessageType::MONSTER_UPDATE, ai_handler_);
        message_processor_->RegisterHandler(ClientMessageType::COMBAT_RESULT, ai_handler_);
        
        // 메시지 프로세서 시작
        message_processor_->Start();
        
        LogMessage("메시지 큐 시스템 초기화 완료");
    }

    void AsioTestClient::ShutdownMessageQueue()
    {
        if (message_processor_)
        {
            message_processor_->Stop();
            message_processor_.reset();
        }
        
        network_handler_.reset();
        ai_handler_.reset();
        
        LogMessage("메시지 큐 시스템 종료 완료");
    }

    void AsioTestClient::SendNetworkPacket(const std::vector<uint8_t>& data, uint16_t packet_type)
    {
        if (!message_processor_)
            return;

        auto packet_msg = std::make_shared<NetworkPacketMessage>(data, packet_type);
        message_processor_->SendMessage(packet_msg);
    }

    void AsioTestClient::UpdateMonsters(const std::unordered_map<uint32_t, std::tuple<float, float, float, float>>& monsters)
    {
        monsters_.clear();
        
        for (const auto& [id, pos] : monsters)
        {
            auto [x, y, z, rotation] = pos;
            monsters_[id] = PlayerPosition(x, y, z, rotation);
        }
        
        last_monster_update_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;
    }

    void AsioTestClient::HandleCombatResult(uint32_t attacker_id, uint32_t target_id, uint32_t damage, uint32_t remaining_health)
    {
        if (attacker_id == player_id_)
        {
            if (verbose_)
            {
                LogMessage("공격 결과: 타겟 " + std::to_string(target_id) + 
                          ", 데미지 " + std::to_string(damage) + 
                          ", 남은 체력 " + std::to_string(remaining_health));
            }
        }
        else if (target_id == player_id_)
        {
            health_ = remaining_health;
            if (health_ <= 0)
            {
                LogMessage("플레이어 사망!", true);
                target_id_ = 0;
            }
        }
    }

    // 비동기 네트워크 메서드들
    void AsioTestClient::StartAsyncNetwork()
    {
        if (network_running_.load())
            return;

        network_running_.store(true);
        network_thread_ = std::thread(&AsioTestClient::NetworkWorker, this);
        
        // 비동기 읽기 시작
        socket_->async_read_some(
            boost::asio::buffer(read_buffer_),
            boost::bind(&AsioTestClient::HandleAsyncRead, this,
                       boost::asio::placeholders::error,
                       boost::asio::placeholders::bytes_transferred)
        );
        
        LogMessage("비동기 네트워크 시작됨");
    }

    void AsioTestClient::StopAsyncNetwork()
    {
        if (!network_running_.load())
            return;

        network_running_.store(false);
        send_queue_cv_.notify_all();
        
        if (network_thread_.joinable())
        {
            network_thread_.join();
        }
        
        LogMessage("비동기 네트워크 종료됨");
    }

    void AsioTestClient::AsyncSendPacket(const Packet& packet)
    {
        if (!connected_.load() || !network_running_.load())
            return;

        {
            std::lock_guard<std::mutex> lock(send_queue_mutex_);
            send_queue_.push(packet);
        }
        send_queue_cv_.notify_one();
    }

    void AsioTestClient::HandleAsyncRead(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!network_running_.load())
            return;

        if (error)
        {
            if (error != boost::asio::error::eof)
            {
                LogMessage("비동기 읽기 오류: " + error.message(), true);
            }
            return;
        }

        // 수신된 데이터 처리
        if (bytes_transferred > 0)
        {
            // 패킷 파싱 및 처리 로직
            // 여기서는 간단히 메시지 큐로 전송
            if (message_processor_)
            {
                auto packet_msg = std::make_shared<NetworkPacketMessage>(
                    std::vector<uint8_t>(read_buffer_.begin(), read_buffer_.begin() + bytes_transferred),
                    static_cast<uint16_t>(PacketType::CONNECT_REQUEST)
                );
                message_processor_->SendMessage(packet_msg);
            }
        }

        // 다음 읽기 시작
        if (network_running_.load())
        {
            socket_->async_read_some(
                boost::asio::buffer(read_buffer_),
                boost::bind(&AsioTestClient::HandleAsyncRead, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred)
            );
        }
    }

    void AsioTestClient::HandleAsyncWrite(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (error)
        {
            LogMessage("비동기 쓰기 오류: " + error.message(), true);
        }
    }

    void AsioTestClient::NetworkWorker()
    {
        while (network_running_.load())
        {
            std::unique_lock<std::mutex> lock(send_queue_mutex_);
            send_queue_cv_.wait(lock, [this] { return !send_queue_.empty() || !network_running_.load(); });

            if (!network_running_.load())
                break;

            if (!send_queue_.empty())
            {
                Packet packet = send_queue_.front();
                send_queue_.pop();
                lock.unlock();

                // 패킷을 비동기로 전송
                try
                {
                    uint32_t total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
                    write_buffer_.resize(total_size);
                    
                    size_t offset = 0;
                    
                    // 크기 (4바이트)
                    std::memcpy(write_buffer_.data() + offset, &total_size, sizeof(uint32_t));
                    offset += sizeof(uint32_t);
                    
                    // 타입 (2바이트)
                    uint16_t packet_type = static_cast<uint16_t>(packet.type);
                    std::memcpy(write_buffer_.data() + offset, &packet_type, sizeof(uint16_t));
                    offset += sizeof(uint16_t);
                    
                    // 데이터
                    if (!packet.data.empty())
                    {
                        std::memcpy(write_buffer_.data() + offset, packet.data.data(), packet.data.size());
                    }

                    boost::asio::async_write(
                        *socket_,
                        boost::asio::buffer(write_buffer_),
                        boost::bind(&AsioTestClient::HandleAsyncWrite, this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred)
                    );
                }
                catch (const std::exception& e)
                {
                    LogMessage("패킷 전송 중 오류: " + std::string(e.what()), true);
                }
            }
        }
    }

} // namespace bt