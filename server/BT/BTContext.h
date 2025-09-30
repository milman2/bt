#pragma once

#include <any>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace bt
{

    // 전방 선언
    class MonsterBTExecutor;
    struct EnvironmentInfo;

    // Behavior Tree 컨텍스트 (Blackboard)
    class BTContext
    {
    public:
        BTContext();
        ~BTContext() = default;

        // 데이터 관리
        void     SetData(const std::string& key, const std::any& value);
        std::any GetData(const std::string& key) const;
        bool     HasData(const std::string& key) const;
        void     RemoveData(const std::string& key);

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

        // 몬스터 AI 참조
        void                              SetMonsterAI(std::shared_ptr<MonsterBTExecutor> ai) { monster_ai_ = ai; }
        std::shared_ptr<MonsterBTExecutor> GetMonsterAI() const { return monster_ai_; }

        // 실행 시간 관리
        void SetStartTime(std::chrono::steady_clock::time_point time) { start_time_ = time; }
        std::chrono::steady_clock::time_point GetStartTime() const { return start_time_; }

        // 환경 정보 관리
        void                   SetEnvironmentInfo(const EnvironmentInfo* env_info) { environment_info_ = env_info; }
        const EnvironmentInfo* GetEnvironmentInfo() const { return environment_info_; }

    private:
        std::unordered_map<std::string, std::any> data_;
        std::shared_ptr<MonsterBTExecutor>        monster_ai_;
        std::chrono::steady_clock::time_point     start_time_;
        const EnvironmentInfo*                    environment_info_ = nullptr;
    };

} // namespace bt
