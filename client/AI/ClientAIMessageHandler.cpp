#include <iostream>

#include "../Common/ClientMessageProcessor.h"
#include "../TestClient.h"
#include "ClientAIMessageHandler.h"

namespace bt
{

    ClientAIMessageHandler::ClientAIMessageHandler(std::shared_ptr<TestClient> client) : client_(client) {}

    void ClientAIMessageHandler::SetMessageProcessor(std::shared_ptr<ClientMessageProcessor> processor)
    {
        message_processor_ = processor;
    }

    void ClientAIMessageHandler::HandleMessage(std::shared_ptr<ClientMessage> message)
    {
        if (!message)
            return;

        switch (message->GetType())
        {
            case ClientMessageType::AI_UPDATE_REQUEST:
                if (auto update_msg = std::dynamic_pointer_cast<AIUpdateMessage>(message))
                {
                    HandleAIUpdate(update_msg);
                }
                break;

            case ClientMessageType::AI_STATE_CHANGE:
                if (auto state_msg = std::dynamic_pointer_cast<AIStateChangeMessage>(message))
                {
                    HandleAIStateChange(state_msg);
                }
                break;

            case ClientMessageType::PLAYER_ACTION_REQUEST:
                if (auto action_msg = std::dynamic_pointer_cast<PlayerActionMessage>(message))
                {
                    HandlePlayerAction(action_msg);
                }
                break;

            case ClientMessageType::MONSTER_UPDATE:
                if (auto monster_msg = std::dynamic_pointer_cast<MonsterUpdateMessage>(message))
                {
                    HandleMonsterUpdate(monster_msg);
                }
                break;

            case ClientMessageType::COMBAT_RESULT:
                if (auto combat_msg = std::dynamic_pointer_cast<CombatResultMessage>(message))
                {
                    HandleCombatResult(combat_msg);
                }
                break;

            default:
                std::cout << "클라이언트 AI 핸들러: 알 수 없는 메시지 타입: " << static_cast<int>(message->GetType())
                          << std::endl;
                break;
        }
    }

    void ClientAIMessageHandler::HandleAIUpdate(std::shared_ptr<AIUpdateMessage> message)
    {
        if (!client_)
            return;

        // AI 업데이트는 메인 스레드에서 처리하므로 여기서는 로깅만
        std::cout << "클라이언트 AI 핸들러: AI 업데이트 요청 수신" << std::endl;
    }

    void ClientAIMessageHandler::HandleAIStateChange(std::shared_ptr<AIStateChangeMessage> message)
    {
        if (!client_)
            return;

        bool active = message->IsActive();

        if (active)
        {
            client_->StartAI();
            std::cout << "클라이언트 AI 핸들러: AI 활성화" << std::endl;
        }
        else
        {
            client_->StopAI();
            std::cout << "클라이언트 AI 핸들러: AI 비활성화" << std::endl;
        }
    }

    void ClientAIMessageHandler::HandlePlayerAction(std::shared_ptr<PlayerActionMessage> message)
    {
        if (!client_)
            return;

        auto action = message->GetAction();

        switch (action)
        {
            case PlayerActionMessage::ActionType::MOVE:
                client_->MoveTo(message->GetX(), message->GetY(), message->GetZ());
                break;

            case PlayerActionMessage::ActionType::ATTACK:
                client_->AttackTarget(message->GetTargetId());
                break;

            case PlayerActionMessage::ActionType::RESPAWN:
                client_->Respawn();
                break;

            default:
                std::cout << "클라이언트 AI 핸들러: 알 수 없는 액션 타입: " << static_cast<int>(action) << std::endl;
                break;
        }
    }

    void ClientAIMessageHandler::HandleMonsterUpdate(std::shared_ptr<MonsterUpdateMessage> message)
    {
        if (!client_)
            return;

        const auto& monsters = message->GetMonsters();

        // 클라이언트의 몬스터 정보 업데이트
        client_->UpdateMonsters(monsters);

        std::cout << "클라이언트 AI 핸들러: 몬스터 업데이트 - " << monsters.size() << "마리" << std::endl;
    }

    void ClientAIMessageHandler::HandleCombatResult(std::shared_ptr<CombatResultMessage> message)
    {
        if (!client_)
            return;

        uint32_t attacker_id      = message->GetAttackerId();
        uint32_t target_id        = message->GetTargetId();
        uint32_t damage           = message->GetDamage();
        uint32_t remaining_health = message->GetRemainingHealth();

        // 전투 결과 처리
        client_->HandleCombatResult(attacker_id, target_id, damage, remaining_health);

        std::cout << "클라이언트 AI 핸들러: 전투 결과 - 공격자:" << attacker_id << ", 타겟:" << target_id
                  << ", 데미지:" << damage << ", 남은체력:" << remaining_health << std::endl;
    }

} // namespace bt
