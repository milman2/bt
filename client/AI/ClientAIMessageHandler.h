#pragma once

#include "../Common/ClientMessageProcessor.h"
#include <memory>

namespace bt
{
    // Forward declaration
    class AsioTestClient;

    // 클라이언트 AI 메시지 핸들러
    class ClientAIMessageHandler : public IClientMessageHandler
    {
    public:
        ClientAIMessageHandler(std::shared_ptr<AsioTestClient> client);
        ~ClientAIMessageHandler() = default;

        // IClientMessageHandler 구현
        void HandleMessage(std::shared_ptr<ClientMessage> message) override;

        // 메시지 프로세서 설정
        void SetMessageProcessor(std::shared_ptr<ClientMessageProcessor> processor);

    private:
        void HandleAIUpdate(std::shared_ptr<AIUpdateMessage> message);
        void HandleAIStateChange(std::shared_ptr<AIStateChangeMessage> message);
        void HandlePlayerAction(std::shared_ptr<PlayerActionMessage> message);
        void HandleMonsterUpdate(std::shared_ptr<MonsterUpdateMessage> message);
        void HandleCombatResult(std::shared_ptr<CombatResultMessage> message);

        std::shared_ptr<AsioTestClient> client_;
        std::shared_ptr<ClientMessageProcessor> message_processor_;
    };

} // namespace bt
