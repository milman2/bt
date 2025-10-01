#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

namespace bt
{

    /**
     * @brief SendBuffer에서 사용할 버퍼 노드
     */
    struct SendBufferNode
    {
        static constexpr size_t BUFFER_SIZE = 4096; // 4KB 버퍼

        std::array<uint8_t, BUFFER_SIZE> data;
        size_t                           used_size = 0;
        std::shared_ptr<SendBufferNode>  next      = nullptr;

        SendBufferNode() = default;

        /**
         * @brief 노드 초기화
         */
        void Reset()
        {
            used_size = 0;
            next      = nullptr;
        }

        /**
         * @brief 사용 가능한 공간 크기
         */
        size_t GetFreeSize() const { return BUFFER_SIZE - used_size; }

        /**
         * @brief 데이터 추가
         * @param src 소스 데이터
         * @param size 추가할 크기
         * @return 실제로 추가된 크기
         */
        size_t AppendData(const void* src, size_t size)
        {
            size_t available = GetFreeSize();
            size_t to_copy   = std::min(size, available);

            if (to_copy > 0)
            {
                std::memcpy(data.data() + used_size, src, to_copy);
                used_size += to_copy;
            }

            return to_copy;
        }
    };

    /**
     * @brief SendBufferNode 메모리 풀
     */
    class SendBufferPool
    {
    public:
        static SendBufferPool& Instance()
        {
            static SendBufferPool* instance = nullptr;
            if (instance == nullptr) {
                std::lock_guard<std::mutex> lock(instance_mutex_);
                if (instance == nullptr) {
                    instance = new SendBufferPool();
                }
            }
            return *instance;
        }

        /**
         * @brief SendBufferNode 할당
         * @return 할당된 SendBufferNode
         */
        std::shared_ptr<SendBufferNode> Alloc()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!free_nodes_.empty())
            {
                auto node = free_nodes_.back();
                free_nodes_.pop_back();
                node->Reset();
                return node;
            }

            // 새 노드 생성
            return std::make_shared<SendBufferNode>();
        }

        /**
         * @brief SendBufferNode 해제
         * @param node 해제할 노드
         */
        void Free(std::shared_ptr<SendBufferNode> node)
        {
            if (!node)
                return;

            std::lock_guard<std::mutex> lock(mutex_);

            // 풀 크기 제한 (메모리 누수 방지)
            if (free_nodes_.size() < MAX_POOL_SIZE)
            {
                node->Reset();
                free_nodes_.push_back(node);
            }
        }

        /**
         * @brief 풀 상태 정보
         */
        struct PoolStats
        {
            size_t free_count;
            size_t total_allocated;
        };

        PoolStats GetStats() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return {free_nodes_.size(), free_nodes_.size()};
        }

    private:
        SendBufferPool()  = default;
        ~SendBufferPool() = default;

        static constexpr size_t MAX_POOL_SIZE = 100; // 최대 풀 크기

        static std::mutex                        instance_mutex_; // 인스턴스 생성용 뮤텍스
        mutable std::mutex                       mutex_;
        std::vector<std::shared_ptr<SendBufferNode>> free_nodes_;
    };

} // namespace bt
