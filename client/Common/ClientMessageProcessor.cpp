#include "ClientMessageProcessor.h"
#include <iostream>
#include <chrono>

namespace bt
{

    ClientMessageProcessor::ClientMessageProcessor() : running_(false)
    {
    }

    ClientMessageProcessor::~ClientMessageProcessor()
    {
        Stop();
    }

    void ClientMessageProcessor::Start()
    {
        if (running_.load())
            return;

        running_.store(true);
        processor_thread_ = std::thread(&ClientMessageProcessor::ProcessMessages, this);
        
        std::cout << "클라이언트 메시지 프로세서 시작됨" << std::endl;
    }

    void ClientMessageProcessor::Stop()
    {
        if (!running_.load())
            return;

        running_.store(false);
        queue_cv_.notify_all();

        if (processor_thread_.joinable())
        {
            processor_thread_.join();
        }

        std::cout << "클라이언트 메시지 프로세서 중지됨" << std::endl;
    }

    void ClientMessageProcessor::SendMessage(std::shared_ptr<ClientMessage> message)
    {
        if (!running_.load() || !message)
            return;

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            message_queue_.push(message);
        }
        queue_cv_.notify_one();
    }

    void ClientMessageProcessor::RegisterHandler(ClientMessageType type, std::shared_ptr<IClientMessageHandler> handler)
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_[type] = handler;
        
        std::cout << "클라이언트 메시지 핸들러 등록: " << static_cast<int>(type) << std::endl;
    }

    size_t ClientMessageProcessor::GetQueueSize() const
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return message_queue_.size();
    }

    void ClientMessageProcessor::ProcessMessages()
    {
        std::cout << "클라이언트 메시지 처리 스레드 시작" << std::endl;

        while (running_.load())
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // 메시지가 올 때까지 대기
            queue_cv_.wait(lock, [this] { return !message_queue_.empty() || !running_.load(); });

            if (!running_.load())
                break;

            // 메시지 처리
            while (!message_queue_.empty())
            {
                auto message = message_queue_.front();
                message_queue_.pop();
                lock.unlock();

                HandleMessage(message);

                lock.lock();
            }
        }

        std::cout << "클라이언트 메시지 처리 스레드 종료" << std::endl;
    }

    void ClientMessageProcessor::HandleMessage(std::shared_ptr<ClientMessage> message)
    {
        if (!message)
            return;

        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find(message->GetType());
        if (it != handlers_.end() && it->second)
        {
            try
            {
                it->second->HandleMessage(message);
            }
            catch (const std::exception& e)
            {
                std::cerr << "클라이언트 메시지 처리 오류: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "클라이언트 메시지 핸들러를 찾을 수 없음: " << static_cast<int>(message->GetType()) << std::endl;
        }
    }

} // namespace bt
