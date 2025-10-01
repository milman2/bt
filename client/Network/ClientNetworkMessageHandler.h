#pragma once

#include "../Common/ClientMessageProcessor.h"
#include <memory>

namespace bt
{
    // Forward declaration
    class AsioTestClient;

    // 클라이언트 네트워크 메시지 핸들러
    class ClientNetworkMessageHandler : public IClientMessageHandler
    {
    public:
        ClientNetworkMessageHandler(std::shared_ptr<AsioTestClient> client);
        ~ClientNetworkMessageHandler() = default;

        // IClientMessageHandler 구현
        void HandleMessage(std::shared_ptr<ClientMessage> message) override;

        // 메시지 프로세서 설정
        void SetMessageProcessor(std::shared_ptr<ClientMessageProcessor> processor);

    private:
        void HandleNetworkPacket(std::shared_ptr<NetworkPacketMessage> message);
        void HandleConnectionLost(std::shared_ptr<ClientMessage> message);
        void HandleConnectionEstablished(std::shared_ptr<ClientMessage> message);

        std::shared_ptr<AsioTestClient> client_;
        std::shared_ptr<ClientMessageProcessor> message_processor_;
    };

} // namespace bt
