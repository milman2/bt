#pragma once

#include <memory>
#include <string>
#include <vector>

namespace bt
{

    // Behavior Tree 노드 상태
    enum class BTNodeStatus
    {
        SUCCESS,
        FAILURE,
        RUNNING
    };

    // Behavior Tree 노드 타입
    enum class BTNodeType
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
    class BTContext;

    // Behavior Tree 노드 기본 클래스
    class BTNode
    {
    public:
        BTNode(const std::string& name, BTNodeType type);
        virtual ~BTNode() = default;

        // 노드 실행
        virtual BTNodeStatus execute(BTContext& context) = 0;

        // 노드 정보
        const std::string& get_name() const { return name_; }
        BTNodeType         get_type() const { return type_; }

        // 자식 노드 관리
        void                                        add_child(std::shared_ptr<BTNode> child);
        const std::vector<std::shared_ptr<BTNode>>& get_children() const { return children_; }

        // 노드 상태
        BTNodeStatus get_last_status() const { return last_status_; }
        void         set_last_status(BTNodeStatus status) { last_status_ = status; }

    protected:
        std::string                          name_;
        BTNodeType                           type_;
        std::vector<std::shared_ptr<BTNode>> children_;
        BTNodeStatus                         last_status_ = BTNodeStatus::FAILURE;
    };

} // namespace bt
