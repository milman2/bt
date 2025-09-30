#pragma once

#include <any>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace bt
{

    // 전방 선언
    struct EnvironmentInfo;
    class IBTExecutor;

    // Behavior Tree 컨텍스트 (Blackboard)
    class BTContext
    {
    public:
        BTContext() : start_time_(std::chrono::steady_clock::now()) {}
        ~BTContext() = default;

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

        // AI 참조 (IBTExecutor 인터페이스 사용)
        void SetAI(std::shared_ptr<IBTExecutor> ai) { ai_ = ai; }
        std::shared_ptr<IBTExecutor> GetAI() const { return ai_; }

        // 실행 시간 관리
        void SetStartTime(std::chrono::steady_clock::time_point time) { start_time_ = time; }
        std::chrono::steady_clock::time_point GetStartTime() const { return start_time_; }

        // 환경 정보 관리
        void                   SetEnvironmentInfo(const EnvironmentInfo* env_info) { environment_info_ = env_info; }
        const EnvironmentInfo* GetEnvironmentInfo() const { return environment_info_; }

    private:
        std::unordered_map<std::string, std::any> data_;
        std::shared_ptr<IBTExecutor>                     ai_;
        std::chrono::steady_clock::time_point     start_time_;
        const EnvironmentInfo*                    environment_info_ = nullptr;
    };

} // namespace bt
