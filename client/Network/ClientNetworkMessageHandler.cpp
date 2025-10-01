#include "ClientNetworkMessageHandler.h"
#include "../Common/ClientMessageProcessor.h"
#include "../AsioTestClient.h"
#include <iostream>

namespace bt
{

    ClientNetworkMessageHandler::ClientNetworkMessageHandler(std::shared_ptr<AsioTestClient> client)
        : client_(client)
    {
    }

    void ClientNetworkMessageHandler::SetMessageProcessor(std::shared_ptr<ClientMessageProcessor> processor)
    {
        message_processor_ = processor;
    }

    void ClientNetworkMessageHandler::HandleMessage(std::shared_ptr<ClientMessage> message)
    {
        if (!message)
            return;

        switch (message->GetType())
        {
            case ClientMessageType::NETWORK_PACKET_RECEIVED:
                if (auto packet_msg = std::dynamic_pointer_cast<NetworkPacketMessage>(message))
                {
                    HandleNetworkPacket(packet_msg);
                }
                break;

            case ClientMessageType::NETWORK_CONNECTION_LOST:
                HandleConnectionLost(message);
                break;

            case ClientMessageType::NETWORK_CONNECTION_ESTABLISHED:
                HandleConnectionEstablished(message);
                break;

            default:
                std::cout << "클라이언트 네트워크 핸들러: 알 수 없는 메시지 타입: " << static_cast<int>(message->GetType()) << std::endl;
                break;
        }
    }

    void ClientNetworkMessageHandler::HandleNetworkPacket(std::shared_ptr<NetworkPacketMessage> message)
    {
        if (!client_ || !message_processor_)
            return;

        const auto& data = message->GetData();
        uint16_t packet_type = message->GetPacketType();

        // 패킷 타입에 따른 처리
        switch (static_cast<PacketType>(packet_type))
        {
            case PacketType::PLAYER_JOIN_RESPONSE:
                // 플레이어 참여 응답 처리
                if (data.size() >= sizeof(uint32_t))
                {
                    uint32_t player_id = *reinterpret_cast<const uint32_t*>(data.data());
                    client_->SetPlayerID(player_id);
                    std::cout << "플레이어 참여 성공: ID " << player_id << std::endl;
                }
                break;

            case PacketType::MONSTER_UPDATE:
                // 몬스터 업데이트 처리
                if (data.size() >= sizeof(uint32_t))
                {
                    uint32_t monster_count = *reinterpret_cast<const uint32_t*>(data.data());
                    std::unordered_map<uint32_t, std::tuple<float, float, float, float>> monsters;

                    size_t offset = sizeof(uint32_t);
                    for (uint32_t i = 0; i < monster_count && offset < data.size(); ++i)
                    {
                        if (offset + sizeof(uint32_t) > data.size())
                            break;

                        uint32_t id = *reinterpret_cast<const uint32_t*>(data.data() + offset);
                        offset += sizeof(uint32_t);

                        if (offset + sizeof(float) * 4 > data.size())
                            break;

                        float x = *reinterpret_cast<const float*>(data.data() + offset);
                        offset += sizeof(float);
                        float y = *reinterpret_cast<const float*>(data.data() + offset);
                        offset += sizeof(float);
                        float z = *reinterpret_cast<const float*>(data.data() + offset);
                        offset += sizeof(float);
                        float rotation = *reinterpret_cast<const float*>(data.data() + offset);
                        offset += sizeof(float);

                        monsters[id] = std::make_tuple(x, y, z, rotation);
                    }

                    // AI 로직으로 몬스터 업데이트 메시지 전송
                    auto monster_update_msg = std::make_shared<MonsterUpdateMessage>(monsters);
                    message_processor_->SendMessage(monster_update_msg);
                }
                break;

            case PacketType::BT_RESULT:
                // 전투 결과 처리
                if (data.size() >= sizeof(uint32_t) * 4)
                {
                    uint32_t attacker_id = *reinterpret_cast<const uint32_t*>(data.data());
                    uint32_t target_id = *reinterpret_cast<const uint32_t*>(data.data() + sizeof(uint32_t));
                    uint32_t damage = *reinterpret_cast<const uint32_t*>(data.data() + sizeof(uint32_t) * 2);
                    uint32_t remaining_health = *reinterpret_cast<const uint32_t*>(data.data() + sizeof(uint32_t) * 3);

                    // AI 로직으로 전투 결과 메시지 전송
                    auto combat_result_msg = std::make_shared<CombatResultMessage>(attacker_id, target_id, damage, remaining_health);
                    message_processor_->SendMessage(combat_result_msg);
                }
                break;

            case PacketType::WORLD_STATE_BROADCAST:
                // 월드 상태 브로드캐스트 처리
                HandleWorldStateBroadcast(data);
                break;

            default:
                std::cout << "클라이언트 네트워크 핸들러: 알 수 없는 패킷 타입: " << packet_type << std::endl;
                break;
        }
    }

    void ClientNetworkMessageHandler::HandleConnectionLost(std::shared_ptr<ClientMessage> message)
    {
        std::cout << "클라이언트 네트워크 핸들러: 연결 끊어짐" << std::endl;
        
        if (client_)
        {
            client_->SetConnected(false);
        }
    }

    void ClientNetworkMessageHandler::HandleConnectionEstablished(std::shared_ptr<ClientMessage> message)
    {
        std::cout << "클라이언트 네트워크 핸들러: 연결 성공" << std::endl;
        
        if (client_)
        {
            client_->SetConnected(true);
        }
    }

    void ClientNetworkMessageHandler::HandleWorldStateBroadcast(const std::vector<uint8_t>& data)
    {
        if (!client_ || data.size() < sizeof(uint64_t) + sizeof(uint32_t) * 2)
            return;

        size_t offset = 0;
        
        // 타임스탬프 읽기
        uint64_t timestamp = *reinterpret_cast<const uint64_t*>(data.data() + offset);
        offset += sizeof(uint64_t);
        
        // 플레이어 수 읽기
        uint32_t player_count = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);
        
        // 몬스터 수 읽기
        uint32_t monster_count = *reinterpret_cast<const uint32_t*>(data.data() + offset);
        offset += sizeof(uint32_t);

        // 플레이어 데이터 읽기
        std::unordered_map<uint32_t, PlayerPosition> players;
        for (uint32_t i = 0; i < player_count && offset < data.size(); ++i)
        {
            if (offset + sizeof(uint32_t) + sizeof(float) * 4 > data.size())
                break;

            uint32_t id = *reinterpret_cast<const uint32_t*>(data.data() + offset);
            offset += sizeof(uint32_t);
            
            float x = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            float y = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            float z = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            uint32_t health = *reinterpret_cast<const uint32_t*>(data.data() + offset);
            offset += sizeof(uint32_t);

            PlayerPosition pos;
            pos.x = x;
            pos.y = y;
            pos.z = z;
            pos.rotation = 0.0f; // 회전 정보는 현재 전송하지 않음
            
            players[id] = pos;
        }

        // 몬스터 데이터 읽기
        std::unordered_map<uint32_t, PlayerPosition> monsters;
        for (uint32_t i = 0; i < monster_count && offset < data.size(); ++i)
        {
            if (offset + sizeof(uint32_t) + sizeof(float) * 4 > data.size())
                break;

            uint32_t id = *reinterpret_cast<const uint32_t*>(data.data() + offset);
            offset += sizeof(uint32_t);
            
            float x = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            float y = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            float z = *reinterpret_cast<const float*>(data.data() + offset);
            offset += sizeof(float);
            uint32_t health = *reinterpret_cast<const uint32_t*>(data.data() + offset);
            offset += sizeof(uint32_t);

            PlayerPosition pos;
            pos.x = x;
            pos.y = y;
            pos.z = z;
            pos.rotation = 0.0f; // 회전 정보는 현재 전송하지 않음
            
            monsters[id] = pos;
        }

        // 클라이언트의 월드 상태 업데이트
        client_->UpdateWorldState(timestamp, players, monsters);
        
        // 디버그 로그 (1초마다)
        static auto last_log_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1)
        {
            std::cout << "월드 상태 업데이트: 플레이어 " << player_count << "명, 몬스터 " << monster_count << "마리" << std::endl;
            last_log_time = now;
        }
    }

} // namespace bt
