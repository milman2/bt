#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <any>

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
    class BTNode;
    class BTContext;
    class MonsterAI;
    struct EnvironmentInfo;
    class Monster;

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

    // Behavior Tree 컨텍스트 (Blackboard)
    class BTContext
    {
    public:
        BTContext();
        ~BTContext() = default;

        // 데이터 관리
        void     set_data(const std::string& key, const std::any& value);
        std::any get_data(const std::string& key) const;
        bool     has_data(const std::string& key) const;
        void     remove_data(const std::string& key);

        // 타입 안전한 데이터 접근
        template <typename T>
        T get_data_as(const std::string& key) const
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
        void set_data(const std::string& key, const T& value)
        {
            data_[key] = value;
        }

        // 몬스터 AI 참조
        void                       set_monster_ai(std::shared_ptr<MonsterAI> ai) { monster_ai_ = ai; }
        std::shared_ptr<MonsterAI> get_monster_ai() const { return monster_ai_; }

        // 실행 시간 관리
        void set_start_time(std::chrono::steady_clock::time_point time) { start_time_ = time; }
        std::chrono::steady_clock::time_point get_start_time() const { return start_time_; }

        // 환경 정보 관리
        void                   set_environment_info(const EnvironmentInfo* env_info) { environment_info_ = env_info; }
        const EnvironmentInfo* get_environment_info() const { return environment_info_; }

    private:
        std::unordered_map<std::string, std::any> data_;
        std::shared_ptr<MonsterAI>                monster_ai_;
        std::chrono::steady_clock::time_point     start_time_;
        const EnvironmentInfo*                    environment_info_ = nullptr;
    };

    // Action 노드 (실행 노드)
    class BTAction : public BTNode
    {
    public:
        using ActionFunction = std::function<BTNodeStatus(BTContext&)>;

        BTAction(const std::string& name, ActionFunction func);
        BTNodeStatus execute(BTContext& context) override;

    private:
        ActionFunction action_func_;
    };

    // Condition 노드 (조건 노드)
    class BTCondition : public BTNode
    {
    public:
        using ConditionFunction = std::function<bool(BTContext&)>;

        BTCondition(const std::string& name, ConditionFunction func);
        BTNodeStatus execute(BTContext& context) override;

    private:
        ConditionFunction condition_func_;
    };

    // Sequence 노드 (순차 실행)
    class BTSequence : public BTNode
    {
    public:
        BTSequence(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

    // Selector 노드 (하나라도 성공하면 성공)
    class BTSelector : public BTNode
    {
    public:
        BTSelector(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

    // Parallel 노드 (병렬 실행)
    class BTParallel : public BTNode
    {
    public:
        enum class Policy
        {
            SUCCEED_ON_ONE, // 하나라도 성공하면 성공
            SUCCEED_ON_ALL, // 모두 성공해야 성공
            FAIL_ON_ONE     // 하나라도 실패하면 실패
        };

        BTParallel(const std::string& name, Policy policy = Policy::SUCCEED_ON_ONE);
        BTNodeStatus execute(BTContext& context) override;

    private:
        Policy policy_;
    };

    // Random 노드 (랜덤 선택)
    class BTRandom : public BTNode
    {
    public:
        BTRandom(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

    // Repeat 노드 (반복 실행)
    class BTRepeat : public BTNode
    {
    public:
        BTRepeat(const std::string& name, int count = -1); // -1은 무한 반복
        BTNodeStatus execute(BTContext& context) override;

    private:
        int repeat_count_;
        int current_count_;
    };

    // Invert 노드 (결과 반전)
    class BTInvert : public BTNode
    {
    public:
        BTInvert(const std::string& name);
        BTNodeStatus execute(BTContext& context) override;
    };

    // Delay 노드 (지연 실행)
    class BTDelay : public BTNode
    {
    public:
        BTDelay(const std::string& name, std::chrono::milliseconds delay);
        BTNodeStatus execute(BTContext& context) override;

    private:
        std::chrono::milliseconds             delay_;
        std::chrono::steady_clock::time_point start_time_;
        bool                                  started_;
    };

    // Timeout 노드 (시간 제한)
    class BTTimeout : public BTNode
    {
    public:
        BTTimeout(const std::string& name, std::chrono::milliseconds timeout);
        BTNodeStatus execute(BTContext& context) override;

    private:
        std::chrono::milliseconds timeout_;
    };

    // Behavior Tree 클래스
    class BehaviorTree
    {
    public:
        BehaviorTree(const std::string& name);
        ~BehaviorTree() = default;

        // 트리 구성
        void                    set_root(std::shared_ptr<BTNode> root);
        std::shared_ptr<BTNode> get_root() const { return root_; }

        // 트리 실행
        BTNodeStatus execute(BTContext& context);

        // 트리 정보
        const std::string& get_name() const { return name_; }

    private:
        std::string             name_;
        std::shared_ptr<BTNode> root_;
    };

    // Behavior Tree 엔진
    class BehaviorTreeEngine
    {
    public:
        BehaviorTreeEngine();
        ~BehaviorTreeEngine();

        // 트리 관리
        void                          register_tree(const std::string& name, std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> get_tree(const std::string& name);
        void                          unregister_tree(const std::string& name);

        // 트리 실행
        BTNodeStatus execute_tree(const std::string& name, BTContext& context);

        // 몬스터 AI 관리
        void register_monster_ai(std::shared_ptr<MonsterAI> ai);
        void unregister_monster_ai(std::shared_ptr<MonsterAI> ai);
        void update_all_monsters(float delta_time);

        // 통계
        size_t get_registered_trees() const { return trees_.size(); }
        size_t get_active_monsters() const { return monster_ais_.size(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<BehaviorTree>> trees_;
        std::vector<std::shared_ptr<MonsterAI>>                        monster_ais_;
        mutable std::mutex                                             trees_mutex_;
        mutable std::mutex                                             monsters_mutex_;
    };

    // 몬스터 AI 클래스
    class MonsterAI : public std::enable_shared_from_this<MonsterAI>
    {
    public:
        MonsterAI(const std::string& name, const std::string& bt_name);
        ~MonsterAI() = default;

        // AI 업데이트
        void update(float delta_time);

        // Behavior Tree 설정
        void                          set_behavior_tree(std::shared_ptr<BehaviorTree> tree);
        std::shared_ptr<BehaviorTree> get_behavior_tree() const { return behavior_tree_; }

        // 컨텍스트 접근
        BTContext&       get_context() { return context_; }
        const BTContext& get_context() const { return context_; }

        // 몬스터 정보
        const std::string& GetName() const { return name_; }
        const std::string& GetBTName() const { return bt_name_; }

        // 몬스터 참조 설정
        void                     set_monster(std::shared_ptr<Monster> monster) { monster_ = monster; }
        std::shared_ptr<Monster> get_monster() const { return monster_; }

        // 상태 관리
        bool is_active() const { return active_.load(); }
        void set_active(bool active) { active_.store(active); }

    private:
        std::string                           name_;
        std::string                           bt_name_;
        std::shared_ptr<BehaviorTree>         behavior_tree_;
        BTContext                             context_;
        std::shared_ptr<Monster>              monster_;
        std::atomic<bool>                     active_;
        std::chrono::steady_clock::time_point last_update_time_;
    };

} // namespace bt
