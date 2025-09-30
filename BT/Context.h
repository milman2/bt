#pragma once

#include <any>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include "EnvironmentInfo.h"

namespace bt
{

    // 전방 선언
    class IExecutor;

    // Behavior Tree 컨텍스트 (Blackboard)
    class Context
    {
    public:
        Context() : start_time_(std::chrono::steady_clock::now()), execution_count_(0) {}
        ~Context() = default;

        // 데이터 관리
        void SetData(const std::string& key, const std::any& value) { data_[key] = value; }
        
        std::any GetData(const std::string& key) const 
        { 
            auto it = data_.find(key);
            if (it != data_.end())
            {
                return it->second;
            }
            return std::any{};
        }
        
        bool HasData(const std::string& key) const { return data_.find(key) != data_.end(); }
        
        void RemoveData(const std::string& key) { data_.erase(key); }

        // 타입 안전한 데이터 접근
        template <typename T>
        T GetDataAs(const std::string& key) const
        {
            auto it = data_.find(key);
            if (it != data_.end())
            {
                try
                {
                    return std::any_cast<T>(it->second);
                }
                catch (const std::bad_any_cast&)
                {
                    return T{};
                }
            }
            return T{};
        }

        template <typename T>
        void SetData(const std::string& key, const T& value)
        {
            data_[key] = value;
        }

        // AI 참조 (IExecutor 인터페이스 사용)
        void SetAI(std::shared_ptr<IExecutor> ai) { ai_ = ai; }
        std::shared_ptr<IExecutor> GetAI() const { return ai_; }

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
        std::unordered_map<std::string, std::any> data_;
        std::shared_ptr<IExecutor>                     ai_;
        std::chrono::steady_clock::time_point     start_time_;
        const EnvironmentInfo*                    environment_info_ = nullptr;
        uint64_t                                 execution_count_;
        std::string                              current_running_node_;
    };

} // namespace bt
