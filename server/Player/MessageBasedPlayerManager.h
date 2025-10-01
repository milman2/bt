#pragma once

#include "PlayerManager.h"
#include "../Common/GameMessages.h"
#include "../Common/GameMessageProcessor.h"
#include "../Common/ReadOnlyView.h"
#include <memory>
#include <vector>
#include <atomic>

namespace bt {

class MessageBasedPlayerManager : public IGameMessageHandler {
public:
    MessageBasedPlayerManager();
    ~MessageBasedPlayerManager();

    // IGameMessageHandler interface
    void HandleMessage(std::shared_ptr<GameMessage> message) override;

    // Dependencies
    void SetMessageProcessor(std::shared_ptr<GameMessageProcessor> processor);
    void SetWebSocketServer(std::shared_ptr<BeastHttpWebSocketServer> server);

    // Player management
    void AddPlayer(std::shared_ptr<Player> player);
    void RemovePlayer(uint32_t player_id);
    std::shared_ptr<Player> GetPlayer(uint32_t player_id);
    std::vector<std::shared_ptr<Player>> GetAllPlayers();
    std::vector<std::shared_ptr<Player>> GetPlayersInRange(float x, float y, float z, float range);
    size_t GetPlayerCount() const;
    
    // Player creation
    std::shared_ptr<Player> CreatePlayer(const std::string& name, const MonsterPosition& position);
    std::shared_ptr<Player> CreatePlayerForClient(uint32_t client_id, const std::string& name, const MonsterPosition& position);
    void RemovePlayerByClientID(uint32_t client_id);
    std::shared_ptr<Player> GetPlayerByClientID(uint32_t client_id);
    uint32_t GetClientIDByPlayerID(uint32_t player_id);

    // Update methods
    void Update(float delta_time);
    void ProcessPlayerMovement(float delta_time);
    void ProcessPlayerCombat(float delta_time);
    void ProcessPlayerRespawn(float delta_time);
    void ProcessPlayerAI(float delta_time);

    // Message handlers
    void HandlePlayerJoinMessage(std::shared_ptr<PlayerJoinMessage> message);
    void HandlePlayerMoveMessage(std::shared_ptr<PlayerMoveMessage> message);
    void HandleGameStateUpdateMessage(std::shared_ptr<GameStateUpdateMessage> message);

    // Lifecycle
    void Start();
    void Stop();

private:
    std::shared_ptr<GameMessageProcessor> message_processor_;
    std::shared_ptr<BeastHttpWebSocketServer> websocket_server_;
    
    // Player storage
    OptimizedCollection<uint32_t, std::shared_ptr<Player>> players_;
    
    // Player management
    std::atomic<uint32_t> next_player_id_;
    
    // State
    std::atomic<bool> running_;
    
    // Helper methods
    void BroadcastPlayerUpdate(const std::shared_ptr<Player>& player);
    void BroadcastPlayerJoin(const std::shared_ptr<Player>& player);
    void BroadcastPlayerLeave(uint32_t player_id);
};

} // namespace bt
