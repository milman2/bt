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

} // namespace bt
