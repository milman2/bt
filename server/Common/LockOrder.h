#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <cstdlib>

namespace bt {

    // 락 순서 정의 (숫자가 낮을수록 먼저 획득)
    enum class LockOrder : int {
        PLAYERS = 1,
        MONSTERS = 2,
        WEBSOCKET = 3,
        SPAWN = 4
    };

    // 스레드별 락 순서 추적
    class LockOrderTracker {
    private:
        static thread_local int last_locked_order_;
        
    public:
        static void SetLastLockedOrder(int order) {
            last_locked_order_ = order;
        }
        
        static int GetLastLockedOrder() {
            return last_locked_order_;
        }
        
        static void Reset() {
            last_locked_order_ = 0;
        }
    };
    

    // 락 순서 검증 매크로
    #define LOCK_IN_ORDER(mutex, order) \
        do { \
            static_assert(static_cast<int>(order) > 0, "Lock order must be positive"); \
            if (bt::LockOrderTracker::GetLastLockedOrder() >= static_cast<int>(order)) { \
                std::cerr << "Lock order violation detected! Expected order > " \
                          << bt::LockOrderTracker::GetLastLockedOrder() \
                          << ", but got " << static_cast<int>(order) \
                          << " in thread " << std::this_thread::get_id() << std::endl; \
                std::abort(); \
            } \
            mutex.lock(); \
            bt::LockOrderTracker::SetLastLockedOrder(static_cast<int>(order)); \
        } while(0)

    // RAII 락 가드 (순서 검증 포함)
    template<LockOrder Order>
    class OrderedLockGuard {
    private:
        std::mutex& mutex_;
        bool locked_;
        
    public:
        explicit OrderedLockGuard(std::mutex& mutex) : mutex_(mutex), locked_(false) {
            try {
                LOCK_IN_ORDER(mutex_, Order);
                locked_ = true;
            } catch (...) {
                // 락 획득 실패 시 예외 전파
                throw;
            }
        }
        
        ~OrderedLockGuard() {
            if (locked_) {
                try {
                    mutex_.unlock();
                } catch (...) {
                    // 소멸자에서는 예외를 던지지 않음
                    std::cerr << "Warning: Exception during mutex unlock in destructor" << std::endl;
                }
            }
        }
        
        // 복사 및 이동 방지
        OrderedLockGuard(const OrderedLockGuard&) = delete;
        OrderedLockGuard& operator=(const OrderedLockGuard&) = delete;
        OrderedLockGuard(OrderedLockGuard&&) = delete;
        OrderedLockGuard& operator=(OrderedLockGuard&&) = delete;
        
        // 락 상태 확인
        bool is_locked() const { return locked_; }
    };

    // 예외 안전한 다중 락 매니저
    class MultiLockManager {
    private:
        std::vector<std::mutex*> locks_;
        std::vector<bool> locked_;
        
    public:
        MultiLockManager() = default;
        
        ~MultiLockManager() {
            unlock_all();
        }
        
        void lock(std::mutex& mutex, LockOrder order) {
            // 순서 검증
            if (!locks_.empty() && static_cast<int>(order) <= LockOrderTracker::GetLastLockedOrder()) {
                std::cerr << "Lock order violation in MultiLockManager!" << std::endl;
                std::abort();
            }
            
            try {
                mutex.lock();
                locks_.push_back(&mutex);
                locked_.push_back(true);
                LockOrderTracker::SetLastLockedOrder(static_cast<int>(order));
            } catch (...) {
                // 락 획득 실패 시 이전 락들 해제
                unlock_all();
                throw;
            }
        }
        
        void unlock_all() {
            // 역순으로 락 해제
            for (int i = locks_.size() - 1; i >= 0; --i) {
                if (locked_[i]) {
                    try {
                        locks_[i]->unlock();
                    } catch (...) {
                        std::cerr << "Warning: Exception during mutex unlock in MultiLockManager" << std::endl;
                    }
                }
            }
            locks_.clear();
            locked_.clear();
            LockOrderTracker::Reset();
        }
        
        // 복사 및 이동 방지
        MultiLockManager(const MultiLockManager&) = delete;
        MultiLockManager& operator=(const MultiLockManager&) = delete;
        MultiLockManager(MultiLockManager&&) = delete;
        MultiLockManager& operator=(MultiLockManager&&) = delete;
    };

    // 편의 매크로
    #define LOCK_PLAYERS(mutex) bt::OrderedLockGuard<bt::LockOrder::PLAYERS> _lock(mutex)
    #define LOCK_MONSTERS(mutex) bt::OrderedLockGuard<bt::LockOrder::MONSTERS> _lock(mutex)
    #define LOCK_WEBSOCKET(mutex) bt::OrderedLockGuard<bt::LockOrder::WEBSOCKET> _lock(mutex)
    #define LOCK_SPAWN(mutex) bt::OrderedLockGuard<bt::LockOrder::SPAWN> _lock(mutex)

} // namespace bt
