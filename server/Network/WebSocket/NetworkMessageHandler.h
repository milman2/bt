#pragma once

#include <memory>
#include <unordered_set>

#include "../../Common/GameMessageProcessor.h"
#include "../../Common/GameMessages.h"

namespace bt
{

    class BeastHttpWebSocketServer;

    // 네트워크 메시지 핸들러
    class NetworkMessageHandler : public IGameMessageHandler
    {
    public:
        NetworkMessageHandler();
        ~NetworkMessageHandler();

        // IGameMessageHandler 구현
        void HandleMessage(std::shared_ptr<GameMessage> message) override;

        // WebSocket 서버 설정
        void SetWebSocketServer(std::shared_ptr<BeastHttpWebSocketServer> server);

        // 연결된 클라이언트 관리
        void   AddClient(const std::string& client_id);
        void   RemoveClient(const std::string& client_id);
        size_t GetConnectedClientCount() const;

    private:
        std::shared_ptr<BeastHttpWebSocketServer> websocket_server_;
        std::unordered_set<std::string>           connected_clients_;

        // 메시지 처리 메서드들
        void HandleMonsterSpawn(std::shared_ptr<MonsterSpawnMessage> message);
        void HandleMonsterMove(std::shared_ptr<MonsterMoveMessage> message);
        void HandleMonsterDeath(std::shared_ptr<MonsterDeathMessage> message);
        void HandlePlayerJoin(std::shared_ptr<PlayerJoinMessage> message);
        void HandlePlayerMove(std::shared_ptr<PlayerMoveMessage> message);
        void HandleGameStateUpdate(std::shared_ptr<GameStateUpdateMessage> message);
        void HandleNetworkBroadcast(std::shared_ptr<NetworkBroadcastMessage> message);
        void HandleSystemShutdown(std::shared_ptr<SystemShutdownMessage> message);

        // 브로드캐스트 헬퍼
        void BroadcastToClients(const std::string& event_type, const nlohmann::json& data);
    };

} // namespace bt
