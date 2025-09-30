#pragma once

#include <any>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace bt
{

    // Behavior Tree의 Blackboard (데이터 저장소)
    class Blackboard
    {
    public:
        Blackboard() = default;
        ~Blackboard() = default;

        // 데이터 설정
        void SetData(const std::string& key, const std::any& value) 
        { 
            data_[key] = value; 
        }

        // 데이터 조회
        std::any GetData(const std::string& key) const 
        { 
            auto it = data_.find(key);
            if (it != data_.end())
            {
                return it->second;
            }
            return std::any();
        }

        // 데이터 존재 여부 확인
        bool HasData(const std::string& key) const 
        { 
            return data_.find(key) != data_.end(); 
        }

        // 데이터 삭제
        void RemoveData(const std::string& key) 
        { 
            data_.erase(key); 
        }

        // 모든 데이터 삭제
        void Clear() 
        { 
            data_.clear(); 
        }

        // 데이터 개수
        size_t Size() const 
        { 
            return data_.size(); 
        }

        // 비어있는지 확인
        bool Empty() const 
        { 
            return data_.empty(); 
        }

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
                catch (const std::bad_any_cast& e)
                {
                    // 타입 변환 실패 시 기본값 반환
                    if constexpr (std::is_default_constructible_v<T>)
                    {
                        return T{};
                    }
                    else
                    {
                        throw std::runtime_error("Blackboard: 타입 변환 실패 - " + std::string(e.what()));
                    }
                }
            }
            
            // 키가 없으면 기본값 반환
            if constexpr (std::is_default_constructible_v<T>)
            {
                return T{};
            }
            else
            {
                throw std::runtime_error("Blackboard: 키 '" + key + "'를 찾을 수 없음");
            }
        }

        // 타입 안전한 데이터 설정
        template <typename T>
        void SetData(const std::string& key, const T& value)
        {
            data_[key] = value;
        }

        // 키 목록 조회
        std::vector<std::string> GetKeys() const
        {
            std::vector<std::string> keys;
            keys.reserve(data_.size());
            for (const auto& pair : data_)
            {
                keys.push_back(pair.first);
            }
            return keys;
        }

        // 특정 타입의 데이터만 조회
        template <typename T>
        std::vector<std::pair<std::string, T>> GetDataOfType() const
        {
            std::vector<std::pair<std::string, T>> result;
            for (const auto& pair : data_)
            {
                try
                {
                    T value = std::any_cast<T>(pair.second);
                    result.emplace_back(pair.first, value);
                }
                catch (const std::bad_any_cast&)
                {
                    // 타입이 맞지 않으면 무시
                }
            }
            return result;
        }

        // 디버깅용: 모든 데이터 출력
        void PrintAllData() const
        {
            std::cout << "=== Blackboard Contents ===" << std::endl;
            for (const auto& pair : data_)
            {
                std::cout << "Key: " << pair.first << " (Type: " << pair.second.type().name() << ")" << std::endl;
            }
            std::cout << "=========================" << std::endl;
        }

        // 복사 생성자 및 할당 연산자
        Blackboard(const Blackboard& other) = default;
        Blackboard& operator=(const Blackboard& other) = default;

        // 이동 생성자 및 할당 연산자
        Blackboard(Blackboard&& other) noexcept = default;
        Blackboard& operator=(Blackboard&& other) noexcept = default;

    private:
        std::unordered_map<std::string, std::any> data_;
    };

} // namespace bt
