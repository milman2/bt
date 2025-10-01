#pragma once

#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace bt {

    // 읽기 전용 뷰 템플릿 클래스
    template<typename Key, typename Value>
    class ReadOnlyView {
    private:
        const std::unordered_map<Key, Value>& container_;
        std::shared_lock<std::shared_mutex> lock_;
        
    public:
        ReadOnlyView(const std::unordered_map<Key, Value>& container, std::shared_mutex& mutex)
            : container_(container), lock_(mutex) {}
        
        // 반복자 지원
        auto begin() const { return container_.begin(); }
        auto end() const { return container_.end(); }
        auto cbegin() const { return container_.cbegin(); }
        auto cend() const { return container_.cend(); }
        
        // 컨테이너 인터페이스
        size_t size() const { return container_.size(); }
        bool empty() const { return container_.empty(); }
        
        // 요소 접근
        auto find(const Key& key) const { return container_.find(key); }
        auto at(const Key& key) const { return container_.at(key); }
        
        // 범위 기반 for 루프 지원
        const std::unordered_map<Key, Value>& get_container() const { return container_; }
    };

    // 성능 최적화된 컬렉션 래퍼
    template<typename Key, typename Value>
    class OptimizedCollection {
    private:
        std::unordered_map<Key, Value> container_;
        mutable std::shared_mutex mutex_;
        
    public:
        // 쓰기 작업
        void insert(const Key& key, const Value& value) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            container_[key] = value;
        }
        
        void erase(const Key& key) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            container_.erase(key);
        }
        
        void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            container_.clear();
        }
        
        // 읽기 전용 뷰 생성
        ReadOnlyView<Key, Value> get_read_only_view() const {
            return ReadOnlyView<Key, Value>(container_, const_cast<std::shared_mutex&>(mutex_));
        }
        
        // 안전한 읽기 작업
        Value get(const Key& key) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = container_.find(key);
            return (it != container_.end()) ? it->second : Value{};
        }
        
        bool contains(const Key& key) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return container_.find(key) != container_.end();
        }
        
        size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return container_.size();
        }
        
        bool empty() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return container_.empty();
        }
        
        // 벡터로 변환 (읽기 전용)
        std::vector<Value> to_vector() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Value> result;
            result.reserve(container_.size());
            for (const auto& [key, value] : container_) {
                result.push_back(value);
            }
            return result;
        }
        
        // 범위 기반 for 루프 지원을 위한 반복자
        auto begin() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return container_.begin();
        }
        
        auto end() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return container_.end();
        }
        
        // 조건부 필터링 (읽기 전용)
        template<typename Predicate>
        std::vector<Value> filter(Predicate pred) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Value> result;
            for (const auto& [key, value] : container_) {
                if (pred(value)) {
                    result.push_back(value);
                }
            }
            return result;
        }
        
        // 조건부 카운팅 (읽기 전용)
        template<typename Predicate>
        size_t count_if(Predicate pred) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            size_t count = 0;
            for (const auto& [key, value] : container_) {
                if (pred(value)) {
                    count++;
                }
            }
            return count;
        }
    };

    // 편의 타입 별칭
    template<typename Value>
    using OptimizedMap = OptimizedCollection<uint32_t, Value>;
    
    template<typename Value>
    using OptimizedStringMap = OptimizedCollection<std::string, Value>;

} // namespace bt
