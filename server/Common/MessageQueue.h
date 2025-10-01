#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <type_traits>
#include <iostream>

namespace bt {

// 메시지 타입 정의
enum class MessageType {
    // 몬스터 관련
    MONSTER_SPAWN,
    MONSTER_DESPAWN,
    MONSTER_MOVE,
    MONSTER_ATTACK,
    MONSTER_DAMAGE,
    MONSTER_DEATH,
    
    // 플레이어 관련
    PLAYER_JOIN,
    PLAYER_LEAVE,
    PLAYER_MOVE,
    PLAYER_ATTACK,
    PLAYER_DAMAGE,
    PLAYER_DEATH,
    
    // 게임 상태 관련
    GAME_STATE_UPDATE,
    WORLD_UPDATE,
    
    // 네트워크 관련
    NETWORK_BROADCAST,
    NETWORK_SEND_TO_CLIENT,
    
    // 시스템 관련
    SYSTEM_SHUTDOWN
};

// 기본 메시지 인터페이스
class IMessage {
public:
    virtual ~IMessage() = default;
    virtual MessageType GetType() const = 0;
    virtual std::string ToString() const = 0;
};

// 메시지 큐 클래스
template<typename T>
class MessageQueue {
private:
    std::queue<std::shared_ptr<T>> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> shutdown_{false};
    
public:
    MessageQueue() = default;
    ~MessageQueue() = default;
    
    // 메시지 추가 (비동기)
    void Push(std::shared_ptr<T> message) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (shutdown_) return;
            queue_.push(message);
        }
        condition_.notify_one();
    }
    
    // 메시지 추가 (템플릿 버전)
    template<typename U, typename... Args>
    void Push(Args&&... args) {
        static_assert(std::is_base_of_v<T, U>, "U must be derived from T");
        Push(std::make_shared<U>(std::forward<Args>(args)...));
    }
    
    // 메시지 가져오기 (블로킹)
    std::shared_ptr<T> Pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
        
        if (shutdown_ && queue_.empty()) {
            return nullptr;
        }
        
        auto message = queue_.front();
        queue_.pop();
        return message;
    }
    
    // 메시지 가져오기 (논블로킹)
    std::shared_ptr<T> TryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        
        auto message = queue_.front();
        queue_.pop();
        return message;
    }
    
    // 모든 메시지 가져오기
    std::vector<std::shared_ptr<T>> PopAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<T>> messages;
        
        while (!queue_.empty()) {
            messages.push_back(queue_.front());
            queue_.pop();
        }
        
        return messages;
    }
    
    // 큐 크기
    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    // 비어있는지 확인
    bool Empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    // 셧다운
    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        condition_.notify_all();
    }
    
    // 셧다운 상태 확인
    bool IsShutdown() const {
        return shutdown_;
    }
};

// 메시지 핸들러 인터페이스
template<typename T>
class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;
    virtual void HandleMessage(std::shared_ptr<T> message) = 0;
};

// 메시지 프로세서 클래스
template<typename T>
class MessageProcessor {
private:
    MessageQueue<T> queue_;
    std::vector<std::unique_ptr<IMessageHandler<T>>> handlers_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_{false};
    
public:
    MessageProcessor() = default;
    ~MessageProcessor() {
        Stop();
    }
    
    // 핸들러 등록
    void RegisterHandler(std::unique_ptr<IMessageHandler<T>> handler) {
        handlers_.push_back(std::move(handler));
    }
    
    // 워커 스레드 시작
    void Start(int worker_count = 1) {
        if (running_) return;
        
        running_ = true;
        
        for (int i = 0; i < worker_count; ++i) {
            worker_threads_.emplace_back([this] {
                ProcessMessages();
            });
        }
    }
    
    // 워커 스레드 중지
    void Stop() {
        if (!running_) return;
        
        running_ = false;
        queue_.Shutdown();
        
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads_.clear();
    }
    
    // 메시지 전송
    void SendMessage(std::shared_ptr<T> message) {
        queue_.Push(message);
    }
    
    template<typename U, typename... Args>
    void SendMessage(Args&&... args) {
        queue_.template Push<U>(std::forward<Args>(args)...);
    }
    
    // 큐 상태
    size_t QueueSize() const { return queue_.Size(); }
    bool IsRunning() const { return running_; }
    
private:
    void ProcessMessages() {
        while (running_) {
            auto message = queue_.Pop();
            if (!message) break;
            
            // 모든 핸들러에게 메시지 전달
            for (auto& handler : handlers_) {
                try {
                    handler->HandleMessage(message);
                } catch (const std::exception& e) {
                    std::cerr << "메시지 처리 중 오류 발생: " << e.what() << std::endl;
                }
            }
        }
    }
};

} // namespace bt
