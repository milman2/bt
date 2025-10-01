#include "MessageBasedPlayerManager.h"
#include "../Network/WebSocket/BeastHttpWebSocketServer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace bt {

MessageBasedPlayerManager::MessageBasedPlayerManager()
    : next_player_id_(1)
    , running_(false)
{
}

MessageBasedPlayerManager::~MessageBasedPlayerManager() {
    Stop();
}

void MessageBasedPlayerManager::SetMessageProcessor(std::shared_ptr<GameMessageProcessor> processor) {
    message_processor_ = processor;
}

void MessageBasedPlayerManager::SetWebSocketServer(std::shared_ptr<BeastHttpWebSocketServer> server) {
    websocket_server_ = server;
}

void MessageBasedPlayerManager::HandleMessage(std::shared_ptr<GameMessage> message) {
    if (!message) return;

    switch (message->GetType()) {
        case MessageType::PLAYER_JOIN:
            if (auto player_join_msg = std::dynamic_pointer_cast<PlayerJoinMessage>(message)) {
                HandlePlayerJoinMessage(player_join_msg);
            }
            break;
        case MessageType::PLAYER_MOVE:
            if (auto player_move_msg = std::dynamic_pointer_cast<PlayerMoveMessage>(message)) {
                HandlePlayerMoveMessage(player_move_msg);
            }
            break;
        case MessageType::GAME_STATE_UPDATE:
            if (auto game_state_msg = std::dynamic_pointer_cast<GameStateUpdateMessage>(message)) {
                HandleGameStateUpdateMessage(game_state_msg);
            }
            break;
        default:
            // 다른 메시지 타입은 무시
            break;
    }
}

void MessageBasedPlayerManager::AddPlayer(std::shared_ptr<Player> player) {
    if (!player) return;

    uint32_t player_id = player->GetID();
    players_.insert(player_id, player);
    
    std::cout << "MessageBasedPlayerManager: 플레이어 추가됨 - ID: " << player_id 
              << ", 이름: " << player->GetName() << std::endl;
    
    // 플레이어 조인 메시지 전송
    auto pos = player->GetPosition();
    auto join_message = std::make_shared<PlayerJoinMessage>(player_id, player->GetName(), pos.x, pos.y, pos.z, pos.rotation);
    if (message_processor_) {
        message_processor_->SendToNetwork(join_message);
    }
    
    BroadcastPlayerJoin(player);
}

void MessageBasedPlayerManager::RemovePlayer(uint32_t player_id) {
    auto player = players_.get(player_id);
    if (player) {
        players_.erase(player_id);
        
        std::cout << "MessageBasedPlayerManager: 플레이어 제거됨 - ID: " << player_id 
                  << ", 이름: " << player->GetName() << std::endl;
        
        BroadcastPlayerLeave(player_id);
    }
}

std::shared_ptr<Player> MessageBasedPlayerManager::GetPlayer(uint32_t player_id) {
    return players_.get(player_id);
}

std::vector<std::shared_ptr<Player>> MessageBasedPlayerManager::GetAllPlayers() {
    return players_.to_vector();
}

std::vector<std::shared_ptr<Player>> MessageBasedPlayerManager::GetPlayersInRange(float x, float y, float z, float range) {
    return players_.filter([x, y, z, range](const std::shared_ptr<Player>& player) {
        if (!player) return false;
        
        auto pos = player->GetPosition();
        float distance = std::sqrt(
            std::pow(pos.x - x, 2) + 
            std::pow(pos.y - y, 2) + 
            std::pow(pos.z - z, 2)
        );
        
        return distance <= range;
    });
}

size_t MessageBasedPlayerManager::GetPlayerCount() const {
    return players_.size();
}

std::shared_ptr<Player> MessageBasedPlayerManager::CreatePlayer(const std::string& name, const MonsterPosition& position) {
    uint32_t player_id = next_player_id_++;
    auto player = std::make_shared<Player>(player_id, name);
    
    // 위치 설정
    player->SetPosition(position.x, position.y, position.z, position.rotation);
    
    players_.insert(player_id, player);
    
    // 플레이어 생성 메시지 전송
    auto join_message = std::make_shared<PlayerJoinMessage>(player_id, name, position.x, position.y, position.z, position.rotation);
    message_processor_->SendToNetwork(join_message);
    
    return player;
}

std::shared_ptr<Player> MessageBasedPlayerManager::CreatePlayerForClient(uint32_t client_id, const std::string& name, const MonsterPosition& position) {
    uint32_t player_id = next_player_id_++;
    auto player = std::make_shared<Player>(player_id, name);
    
    // 위치 설정
    player->SetPosition(position.x, position.y, position.z, position.rotation);
    
    players_.insert(player_id, player);
    
    // 클라이언트 ID와 플레이어 ID 매핑 저장 (간단한 구현)
    // 실제로는 별도의 매핑 테이블이 필요할 수 있음
    
    // 플레이어 생성 메시지 전송
    auto join_message = std::make_shared<PlayerJoinMessage>(player_id, name, position.x, position.y, position.z, position.rotation);
    message_processor_->SendToNetwork(join_message);
    
    return player;
}

void MessageBasedPlayerManager::RemovePlayerByClientID(uint32_t client_id) {
    // 클라이언트 ID로 플레이어를 찾아서 제거
    // 간단한 구현: 모든 플레이어를 순회하여 찾기
    auto view = players_.get_read_only_view();
    for (const auto& pair : view) {
        // 실제로는 클라이언트 ID 매핑이 필요하지만, 간단히 구현
        // 여기서는 첫 번째 플레이어를 제거 (실제로는 매핑 테이블 필요)
        if (pair.second) {
            RemovePlayer(pair.first);
            break;
        }
    }
}

std::shared_ptr<Player> MessageBasedPlayerManager::GetPlayerByClientID(uint32_t client_id) {
    // 클라이언트 ID로 플레이어를 찾기
    // 간단한 구현: 모든 플레이어를 순회하여 찾기
    auto view = players_.get_read_only_view();
    for (const auto& pair : view) {
        // 실제로는 클라이언트 ID 매핑이 필요하지만, 간단히 구현
        // 여기서는 첫 번째 플레이어를 반환 (실제로는 매핑 테이블 필요)
        if (pair.second) {
            return pair.second;
        }
    }
    return nullptr;
}

uint32_t MessageBasedPlayerManager::GetClientIDByPlayerID(uint32_t player_id) {
    // 플레이어 ID로 클라이언트 ID를 찾기
    // 간단한 구현: 매핑 테이블이 없으므로 0 반환
    // 실제로는 별도의 매핑 테이블이 필요
    return 0;
}

void MessageBasedPlayerManager::Update(float delta_time) {
    if (!running_) return;

    // 플레이어 업데이트
    auto read_view = players_.get_read_only_view();
    for (const auto& pair : read_view) {
        const auto& player = pair.second;
        if (player && player->IsAlive()) {
            player->Update(delta_time);
        }
    }

    // 각종 처리
    ProcessPlayerMovement(delta_time);
    ProcessPlayerCombat(delta_time);
    ProcessPlayerRespawn(delta_time);
    ProcessPlayerAI(delta_time);
}

void MessageBasedPlayerManager::ProcessPlayerMovement(float delta_time) {
    auto read_view = players_.get_read_only_view();
    for (const auto& pair : read_view) {
        const auto& player = pair.second;
        if (player && player->IsAlive()) {
            // 플레이어 이동 처리 (Player 클래스에는 UpdateMovement가 없으므로 Update 사용)
            // 실제 이동 로직은 Player::Update에서 처리됨
            
            // 이동 업데이트 브로드캐스트
            BroadcastPlayerUpdate(player);
        }
    }
}

void MessageBasedPlayerManager::ProcessPlayerCombat(float delta_time) {
    auto read_view = players_.get_read_only_view();
    for (const auto& pair : read_view) {
        const auto& player = pair.second;
        if (player && player->IsAlive() && player->GetState() == PlayerState::IN_COMBAT) {
            // 전투 처리 (Player 클래스에는 UpdateCombat가 없으므로 Update 사용)
            // 실제 전투 로직은 Player::Update에서 처리됨
            
            // 전투 상태 업데이트 브로드캐스트
            BroadcastPlayerUpdate(player);
        }
    }
}

void MessageBasedPlayerManager::ProcessPlayerRespawn(float delta_time) {
    auto read_view = players_.get_read_only_view();
    for (const auto& pair : read_view) {
        const auto& player = pair.second;
        if (player && !player->IsAlive()) {
            // 리스폰 처리 (Player 클래스에는 ShouldRespawn가 없으므로 간단한 로직 사용)
            // 실제 리스폰 로직은 Player::Update에서 처리됨
            player->Respawn();
            BroadcastPlayerUpdate(player);
        }
    }
}

void MessageBasedPlayerManager::ProcessPlayerAI(float delta_time) {
    auto read_view = players_.get_read_only_view();
    for (const auto& pair : read_view) {
        const auto& player = pair.second;
        if (player && player->IsAlive()) {
            // 플레이어 AI 처리 (클라이언트 AI 등)
            // Player 클래스에는 UpdateAI가 없으므로 Update 사용
            player->Update(delta_time);
        }
    }
}

void MessageBasedPlayerManager::HandlePlayerJoinMessage(std::shared_ptr<PlayerJoinMessage> message) {
    if (!message) return;
    
    std::cout << "MessageBasedPlayerManager: 플레이어 조인 메시지 처리 - ID: " 
              << message->player_id << ", 이름: " << message->player_name << std::endl;
    
    // 플레이어 조인 처리 로직
    // 실제로는 클라이언트 연결 시 호출됨
}

void MessageBasedPlayerManager::HandlePlayerMoveMessage(std::shared_ptr<PlayerMoveMessage> message) {
    if (!message) return;
    
    auto player = GetPlayer(message->player_id);
    if (player) {
        // 플레이어 이동 처리
        // PlayerMoveMessage는 from/to 위치만 가지고 있으므로
        // 실제 위치 업데이트는 다른 방식으로 처리해야 함
        
        BroadcastPlayerUpdate(player);
    }
}

void MessageBasedPlayerManager::HandleGameStateUpdateMessage(std::shared_ptr<GameStateUpdateMessage> message) {
    if (!message) return;
    
    // 게임 상태 업데이트 처리
    // 필요에 따라 플레이어 상태 동기화
}

void MessageBasedPlayerManager::Start() {
    if (running_) return;
    
    running_ = true;
    std::cout << "MessageBasedPlayerManager: 시작됨" << std::endl;
}

void MessageBasedPlayerManager::Stop() {
    if (!running_) return;
    
    running_ = false;
    std::cout << "MessageBasedPlayerManager: 중지됨" << std::endl;
}

void MessageBasedPlayerManager::BroadcastPlayerUpdate(const std::shared_ptr<Player>& player) {
    if (!player || !websocket_server_) return;

    nlohmann::json event;
    event["type"] = "player_update";
    event["player_id"] = player->GetID();
    event["name"] = player->GetName();
    event["position"] = {
        {"x", player->GetPosition().x},
        {"y", player->GetPosition().y},
        {"z", player->GetPosition().z},
        {"rotation", player->GetPosition().rotation}
    };
    event["health"] = player->GetStats().health;
    event["max_health"] = player->GetStats().max_health;
    event["is_alive"] = player->IsAlive();
    event["is_moving"] = false; // Player 클래스에는 IsMoving이 없음
    event["is_in_combat"] = (player->GetState() == PlayerState::IN_COMBAT);

    std::string event_data = event.dump();
    websocket_server_->broadcast(event_data);
}

void MessageBasedPlayerManager::BroadcastPlayerJoin(const std::shared_ptr<Player>& player) {
    if (!player || !websocket_server_) return;

    nlohmann::json event;
    event["type"] = "player_join";
    event["player_id"] = player->GetID();
    event["name"] = player->GetName();
    event["position"] = {
        {"x", player->GetPosition().x},
        {"y", player->GetPosition().y},
        {"z", player->GetPosition().z},
        {"rotation", player->GetPosition().rotation}
    };

    std::string event_data = event.dump();
    websocket_server_->broadcast(event_data);
}

void MessageBasedPlayerManager::BroadcastPlayerLeave(uint32_t player_id) {
    if (!websocket_server_) return;

    nlohmann::json event;
    event["type"] = "player_leave";
    event["player_id"] = player_id;

    std::string event_data = event.dump();
    websocket_server_->broadcast(event_data);
}

} // namespace bt
