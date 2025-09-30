#pragma once

#include <chrono>
#include <memory>
#include <string>
#include "EnvironmentInfo.h"
#include "Blackboard.h"

namespace bt
{

    // 전방 선언
    class IInterface;
    class IExecutor;
    class IOwner;

    // Behavior Tree 컨텍스트 (Blackboard)
    class Context
    {
    public:
        Context() : start_time_(std::chrono::steady_clock::now()), execution_count_(0) {}
        ~Context() = default;

        // Blackboard 데이터 관리 (위임)
        void SetData(const std::string& key, const std::any& value) { blackboard_.SetData(key, value); }
        std::any GetData(const std::string& key) const { return blackboard_.GetData(key); }
        bool HasData(const std::string& key) const { return blackboard_.HasData(key); }
        void RemoveData(const std::string& key) { blackboard_.RemoveData(key); }
        void ClearData() { blackboard_.Clear(); }
        size_t GetDataSize() const { return blackboard_.Size(); }
        bool IsDataEmpty() const { return blackboard_.Empty(); }

        // 타입 안전한 데이터 접근 (위임)
        template <typename T>
        T GetDataAs(const std::string& key) const
        {
            return blackboard_.GetDataAs<T>(key);
        }

        template <typename T>
        void SetData(const std::string& key, const T& value)
        {
            blackboard_.SetData(key, value);
        }

        // Interface 참조 (IInterface 인터페이스 사용)
        void SetInterface(const std::string& name, std::shared_ptr<IInterface> interface) { interfaces_[name] = interface; }
        std::shared_ptr<IInterface> GetInterface(const std::string& name) const { return interfaces_.at(name); }

        // Owner 참조 (IOwner 인터페이스 사용)
        void SetOwner(std::shared_ptr<IOwner> owner) { owner_ = owner; }
        std::shared_ptr<IOwner> GetOwner() const { return owner_; }

        // AI 참조 (IExecutor 인터페이스 사용)
        void SetAI(std::shared_ptr<IExecutor> ai) { ai_ = ai; }
        std::shared_ptr<IExecutor> GetAI() const { return ai_; }

        // Blackboard 직접 접근
        Blackboard& GetBlackboard() { return blackboard_; }
        const Blackboard& GetBlackboard() const { return blackboard_; }

        // 실행 시간 관리
        void SetStartTime(std::chrono::steady_clock::time_point time) { start_time_ = time; }
        std::chrono::steady_clock::time_point GetStartTime() const { return start_time_; }

        // 환경 정보 관리
        void                   SetEnvironmentInfo(const EnvironmentInfo* env_info) { environment_info_ = env_info; }
        const EnvironmentInfo* GetEnvironmentInfo() const { return environment_info_; }

        // 실행 컨텍스트 관리
        void IncrementExecutionCount() { execution_count_++; }
        uint64_t GetExecutionCount() const { return execution_count_; }
        void ResetExecutionCount() { execution_count_ = 0; }

        // 현재 실행 중인 노드 추적
        void SetCurrentRunningNode(const std::string& node_name) { current_running_node_ = node_name; }
        const std::string& GetCurrentRunningNode() const { return current_running_node_; }
        void ClearCurrentRunningNode() { current_running_node_.clear(); }

    private:        
        std::unordered_map<std::string, std::shared_ptr<IInterface>> interfaces_;
        std::shared_ptr<IOwner> owner_;
        std::shared_ptr<IExecutor> ai_;
        // 불변값을 갖는 blackboard를 추가하는 것을 고려해보자 => BT의 스크립트 설정값?
        Blackboard blackboard_;
        std::chrono::steady_clock::time_point start_time_;
        const EnvironmentInfo*                    environment_info_ = nullptr;
        uint64_t                                 execution_count_;
        std::string                              current_running_node_;
    };

} // namespace bt
