#pragma once

#include "MessageQueue.h"
#include "GameMessages.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <iostream>

namespace bt {

// 게임 메시지 핸들러 인터페이스
class IGameMessageHandler {
public:
    virtual ~IGameMessageHandler() = default;
    virtual void HandleMessage(std::shared_ptr<GameMessage> message) = 0;
};

// 게임 메시지 프로세서
class GameMessageProcessor {
private:
    MessageQueue<GameMessage> game_queue_;
    MessageQueue<GameMessage> network_queue_;
    
    std::vector<std::unique_ptr<IGameMessageHandler>> game_handlers_;
    std::vector<std::unique_ptr<IGameMessageHandler>> network_handlers_;
    
    std::thread game_thread_;
    std::thread network_thread_;
    std::atomic<bool> running_{false};
    
public:
    GameMessageProcessor() = default;
    ~GameMessageProcessor() {
        Stop();
    }
    
    // 게임 로직 핸들러 등록
    void RegisterGameHandler(std::unique_ptr<IGameMessageHandler> handler) {
        game_handlers_.push_back(std::move(handler));
    }
    
    // 네트워크 핸들러 등록
    void RegisterNetworkHandler(std::unique_ptr<IGameMessageHandler> handler) {
        network_handlers_.push_back(std::move(handler));
    }
    
    // 시작
    void Start() {
        if (running_) return;
        
        running_ = true;
        
        // 게임 로직 스레드 시작
        game_thread_ = std::thread([this] {
            ProcessGameMessages();
        });
        
        // 네트워크 스레드 시작
        network_thread_ = std::thread([this] {
            ProcessNetworkMessages();
        });
    }
    
    // 중지
    void Stop() {
        if (!running_) return;
        
        running_ = false;
        
        game_queue_.Shutdown();
        network_queue_.Shutdown();
        
        if (game_thread_.joinable()) {
            game_thread_.join();
        }
        
        if (network_thread_.joinable()) {
            network_thread_.join();
        }
    }
    
    // 게임 로직에 메시지 전송
    void SendToGame(std::shared_ptr<GameMessage> message) {
        game_queue_.Push(message);
    }
    
    template<typename T, typename... Args>
    void SendToGame(Args&&... args) {
        game_queue_.template Push<T>(std::forward<Args>(args)...);
    }
    
    // 네트워크에 메시지 전송
    void SendToNetwork(std::shared_ptr<GameMessage> message) {
        network_queue_.Push(message);
    }
    
    template<typename T, typename... Args>
    void SendToNetwork(Args&&... args) {
        network_queue_.template Push<T>(std::forward<Args>(args)...);
    }
    
    // 큐 상태
    size_t GameQueueSize() const { return game_queue_.Size(); }
    size_t NetworkQueueSize() const { return network_queue_.Size(); }
    bool IsRunning() const { return running_; }
    
private:
    void ProcessGameMessages() {
        while (running_) {
            auto message = game_queue_.Pop();
            if (!message) break;
            
            // 게임 로직 핸들러들에게 메시지 전달
            for (auto& handler : game_handlers_) {
                try {
                    handler->HandleMessage(message);
                } catch (const std::exception& e) {
                    std::cerr << "게임 메시지 처리 중 오류: " << e.what() << std::endl;
                }
            }
        }
    }
    
    void ProcessNetworkMessages() {
        while (running_) {
            auto message = network_queue_.Pop();
            if (!message) break;
            
            // 네트워크 핸들러들에게 메시지 전달
            for (auto& handler : network_handlers_) {
                try {
                    handler->HandleMessage(message);
                } catch (const std::exception& e) {
                    std::cerr << "네트워크 메시지 처리 중 오류: " << e.what() << std::endl;
                }
            }
        }
    }
};

// 메시지 라우터 - 메시지 타입에 따라 적절한 큐로 라우팅
class MessageRouter {
private:
    GameMessageProcessor* processor_;
    
public:
    explicit MessageRouter(GameMessageProcessor* processor) : processor_(processor) {}
    
    // 메시지 라우팅
    void RouteMessage(std::shared_ptr<GameMessage> message) {
        switch (message->GetType()) {
            // 게임 로직에서 처리해야 할 메시지들
            case MessageType::MONSTER_SPAWN:
            case MessageType::MONSTER_DESPAWN:
            case MessageType::MONSTER_MOVE:
            case MessageType::MONSTER_ATTACK:
            case MessageType::MONSTER_DAMAGE:
            case MessageType::MONSTER_DEATH:
            case MessageType::PLAYER_JOIN:
            case MessageType::PLAYER_LEAVE:
            case MessageType::PLAYER_MOVE:
            case MessageType::PLAYER_ATTACK:
            case MessageType::PLAYER_DAMAGE:
            case MessageType::PLAYER_DEATH:
            case MessageType::GAME_STATE_UPDATE:
            case MessageType::WORLD_UPDATE:
                processor_->SendToGame(message);
                break;
                
            // 네트워크에서 처리해야 할 메시지들
            case MessageType::NETWORK_BROADCAST:
            case MessageType::NETWORK_SEND_TO_CLIENT:
                processor_->SendToNetwork(message);
                break;
                
            // 시스템 메시지는 양쪽 모두에 전달
            case MessageType::SYSTEM_SHUTDOWN:
                processor_->SendToGame(message);
                processor_->SendToNetwork(message);
                break;
        }
    }
};

} // namespace bt
