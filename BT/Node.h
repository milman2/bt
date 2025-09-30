#pragma once

#include <memory>
#include <string>
#include <vector>

namespace bt
{

    // Behavior Tree 노드 상태
    enum class NodeStatus
    {
        SUCCESS,
        FAILURE,
        RUNNING
    };

    // Behavior Tree 노드 타입
    enum class NodeType
    {
        ACTION,
        CONDITION,
        SEQUENCE,
        SELECTOR,
        PARALLEL,
        RANDOM,
        REPEAT,
        INVERT,
        DELAY,
        TIMEOUT,
        BLACKBOARD
    };

    // 전방 선언
    class Context;

    // Behavior Tree 노드 기본 클래스
    class Node
    {
    public:
        Node(const std::string& name, NodeType type) : name_(name), type_(type), is_running_(false) {}
        virtual ~Node() = default;

        // 노드 실행
        virtual NodeStatus Execute(Context& context) = 0;

        // 노드 정보
        const std::string& GetName() const { return name_; }
        NodeType         GetType() const { return type_; }

        // 자식 노드 관리
        void AddChild(std::shared_ptr<Node> child) { children_.push_back(child); }
        const std::vector<std::shared_ptr<Node>>& GetChildren() const { return children_; }

        // 노드 상태
        NodeStatus GetLastStatus() const { return last_status_; }
        void         SetLastStatus(NodeStatus status) { last_status_ = status; }
        
        // 실행 상태 관리
        bool IsRunning() const { return is_running_; }
        void SetRunning(bool running) { is_running_ = running; }
        
        // 노드 초기화 (새로운 실행 시작 시)
        virtual void Initialize() { is_running_ = false; }
        
        // 노드 정리 (실행 완료 시)
        virtual void Cleanup() { is_running_ = false; }

    protected:
        std::string                          name_;
        NodeType                           type_;
        std::vector<std::shared_ptr<Node>> children_;
        NodeStatus                         last_status_ = NodeStatus::FAILURE;
        bool                               is_running_ = false;
    };

} // namespace bt
