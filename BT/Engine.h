#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Tree.h"
#include "Context.h"

namespace bt
{

    // 전방 선언
    class Context;

    // Behavior Tree 엔진
    class Engine
    {
    public:
        Engine() {}
        ~Engine() {}

        // 트리 관리
        void RegisterTree(const std::string& name, std::shared_ptr<Tree> tree)
        {
            std::lock_guard<std::mutex> lock(trees_mutex_);
            trees_[name] = tree;
        }
        
        std::shared_ptr<Tree> GetTree(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(trees_mutex_);
            auto it = trees_.find(name);
            if (it != trees_.end())
            {
                return it->second;
            }
            return nullptr;
        }
        
        void UnregisterTree(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(trees_mutex_);
            trees_.erase(name);
        }

        // 트리 실행
        NodeStatus ExecuteTree(const std::string& name, Context& context)
        {
            auto tree = GetTree(name);
            if (tree)
            {
                return tree->Execute(context);
            }
            return NodeStatus::FAILURE;
        }

        // 통계
        size_t GetRegisteredTrees() const { return trees_.size(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<Tree>> trees_;
        mutable std::mutex                                             trees_mutex_;
    };

} // namespace bt
