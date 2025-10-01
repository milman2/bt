#pragma once

#include <atomic>
#include <chrono>

#include "../Context.h"
#include "../IExecutor.h"
#include "../Node.h"

namespace bt
{
    namespace test
    {

        // 테스트용 Mock AI Executor
        class MockAIExecutor : public IExecutor
        {
        public:
            MockAIExecutor(const std::string& name) : name_(name), health_(100)
            {
                position_.x        = 0;
                position_.y        = 0;
                position_.z        = 0;
                position_.rotation = 0;
            }

            // IExecutor 인터페이스 구현
            void                  Update(float delta_time) override {}
            void                  SetBehaviorTree(std::shared_ptr<Tree> tree) override { behavior_tree_ = tree; }
            std::shared_ptr<Tree> GetBehaviorTree() const override { return behavior_tree_; }
            Context&              GetContext() override { return context_; }
            const Context&        GetContext() const override { return context_; }
            const std::string&    GetName() const override { return name_; }
            const std::string&    GetBTName() const override
            {
                static const std::string bt_name = "test_bt";
                return bt_name;
            }
            bool IsActive() const override { return active_; }
            void SetActive(bool active) override { active_ = active; }

            // 테스트용 상태
            int  GetHealth() const { return health_; }
            void SetHealth(int health) { health_ = health; }
            void TakeDamage(int damage) { health_ -= damage; }

            struct Position
            {
                float x, y, z, rotation;
            };
            Position GetPosition() const { return position_; }
            void     SetPosition(float x, float y, float z, float r) { position_ = {x, y, z, r}; }
            void     MoveTo(float x, float y, float z, float r) { position_ = {x, y, z, r}; }

            bool     HasTarget() const { return target_id_ != 0; }
            uint32_t GetTargetID() const { return target_id_; }
            void     SetTarget(uint32_t id) { target_id_ = id; }
            void     ClearTarget() { target_id_ = 0; }

            float GetDistanceToTarget() const { return distance_to_target_; }
            void  SetDistanceToTarget(float distance) { distance_to_target_ = distance; }

            // 테스트용 카운터
            std::atomic<int> action_count_{0};
            std::atomic<int> condition_count_{0};

        private:
            std::string           name_;
            std::shared_ptr<Tree> behavior_tree_;
            Context               context_;
            bool                  active_             = true;
            int                   health_             = 100;
            Position              position_           = {0, 0, 0, 0};
            uint32_t              target_id_          = 0;
            float                 distance_to_target_ = 0.0f;
        };

        // 테스트용 Action 노드 - 즉시 SUCCESS 반환
        class TestSuccessAction : public Node
        {
        public:
            TestSuccessAction(const std::string& name) : Node(name, NodeType::ACTION) {}

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->action_count_++;
                }

                SetLastStatus(NodeStatus::SUCCESS);
                return NodeStatus::SUCCESS;
            }
        };

        // 테스트용 Action 노드 - 즉시 FAILURE 반환
        class TestFailureAction : public Node
        {
        public:
            TestFailureAction(const std::string& name) : Node(name, NodeType::ACTION) {}

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->action_count_++;
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }
        };

        // 테스트용 Action 노드 - RUNNING 상태를 여러 틱 동안 유지
        class TestRunningAction : public Node
        {
        public:
            TestRunningAction(const std::string& name, int required_ticks = 3)
                : Node(name, NodeType::ACTION), required_ticks_(required_ticks), current_tick_(0)
            {
            }

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->action_count_++;
                }

                current_tick_++;

                if (current_tick_ >= required_ticks_)
                {
                    current_tick_ = 0; // 리셋
                    SetRunning(false);
                    SetLastStatus(NodeStatus::SUCCESS);
                    return NodeStatus::SUCCESS;
                }
                else
                {
                    SetRunning(true);
                    SetLastStatus(NodeStatus::RUNNING);
                    return NodeStatus::RUNNING;
                }
            }

            void Initialize() override
            {
                Node::Initialize();
                current_tick_ = 0;
            }

        private:
            int required_ticks_;
            int current_tick_;
        };

        // 테스트용 Condition 노드 - 체력 확인
        class TestHealthCondition : public Node
        {
        public:
            TestHealthCondition(const std::string& name, int min_health = 50)
                : Node(name, NodeType::CONDITION), min_health_(min_health)
            {
            }

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->condition_count_++;

                    if (mock_ai->GetHealth() >= min_health_)
                    {
                        SetLastStatus(NodeStatus::SUCCESS);
                        return NodeStatus::SUCCESS;
                    }
                    else
                    {
                        SetLastStatus(NodeStatus::FAILURE);
                        return NodeStatus::FAILURE;
                    }
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

        private:
            int min_health_;
        };

        // 테스트용 Condition 노드 - 타겟 존재 확인
        class TestHasTargetCondition : public Node
        {
        public:
            TestHasTargetCondition(const std::string& name) : Node(name, NodeType::CONDITION) {}

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->condition_count_++;

                    if (mock_ai->HasTarget())
                    {
                        SetLastStatus(NodeStatus::SUCCESS);
                        return NodeStatus::SUCCESS;
                    }
                    else
                    {
                        SetLastStatus(NodeStatus::FAILURE);
                        return NodeStatus::FAILURE;
                    }
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }
        };

        // 테스트용 Condition 노드 - 공격 범위 확인
        class TestInRangeCondition : public Node
        {
        public:
            TestInRangeCondition(const std::string& name, float max_range = 5.0f)
                : Node(name, NodeType::CONDITION), max_range_(max_range)
            {
            }

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->condition_count_++;

                    if (mock_ai->GetDistanceToTarget() <= max_range_)
                    {
                        SetLastStatus(NodeStatus::SUCCESS);
                        return NodeStatus::SUCCESS;
                    }
                    else
                    {
                        SetLastStatus(NodeStatus::FAILURE);
                        return NodeStatus::FAILURE;
                    }
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

        private:
            float max_range_;
        };

        // 테스트용 Action 노드 - 공격 시뮬레이션
        class TestAttackAction : public Node
        {
        public:
            TestAttackAction(const std::string& name, int damage = 20) : Node(name, NodeType::ACTION), damage_(damage)
            {
            }

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->action_count_++;

                    // 공격 시뮬레이션 (즉시 완료)
                    SetLastStatus(NodeStatus::SUCCESS);
                    return NodeStatus::SUCCESS;
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

        private:
            int damage_;
        };

        // 테스트용 Action 노드 - 이동 시뮬레이션 (RUNNING 상태)
        class TestMoveAction : public Node
        {
        public:
            TestMoveAction(const std::string& name, int required_ticks = 5)
                : Node(name, NodeType::ACTION), required_ticks_(required_ticks), current_tick_(0)
            {
            }

            NodeStatus Execute(Context& context) override
            {
                auto ai = context.GetAI();
                if (auto mock_ai = std::dynamic_pointer_cast<MockAIExecutor>(ai))
                {
                    mock_ai->action_count_++;

                    current_tick_++;

                    if (current_tick_ >= required_ticks_)
                    {
                        // 이동 완료
                        current_tick_ = 0;
                        SetRunning(false);
                        SetLastStatus(NodeStatus::SUCCESS);
                        return NodeStatus::SUCCESS;
                    }
                    else
                    {
                        // 이동 중
                        SetRunning(true);
                        SetLastStatus(NodeStatus::RUNNING);
                        return NodeStatus::RUNNING;
                    }
                }

                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

            void Initialize() override
            {
                Node::Initialize();
                current_tick_ = 0;
            }

        private:
            int required_ticks_;
            int current_tick_;
        };

    } // namespace test
} // namespace bt
