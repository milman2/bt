#include "BehaviorTreeTests.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

namespace bt
{
namespace test
{

    std::vector<TestResult> BehaviorTreeTestSuite::RunAllTests()
    {
        std::vector<TestResult> results;
        
        std::cout << "=== Behavior Tree Unit Tests 시작 ===\n\n";
        
        // 기본 노드 테스트
        results.push_back(TestBasicNodeExecution());
        
        // 복합 노드 테스트
        results.push_back(TestSequenceNode());
        results.push_back(TestSelectorNode());
        
        // RUNNING 상태 테스트
        results.push_back(TestRunningStatePersistence());
        
        // 복잡한 BT 테스트
        results.push_back(TestComplexBehaviorTree());
        
        // 트리 관리 테스트
        results.push_back(TestTreeInitialization());
        results.push_back(TestContextManagement());
        results.push_back(TestEngineRegistration());
        results.push_back(TestBlackboardFunctionality());
        results.push_back(TestEnvironmentInfoFunctionality());
        
        return results;
    }

    TestResult BehaviorTreeTestSuite::TestBasicNodeExecution()
    {
        std::cout << "테스트: 기본 노드 실행\n";
        
        try
        {
            auto mock_ai = CreateMockAI("BasicTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            // SUCCESS 액션 테스트
            auto success_action = std::make_shared<TestSuccessAction>("success_test");
            NodeStatus status = success_action->Execute(context);
            
            if (!AssertEqual("SUCCESS 액션", NodeStatus::SUCCESS, status))
                return TestResult("TestBasicNodeExecution", false, "SUCCESS 액션 실패");
            
            // FAILURE 액션 테스트
            auto failure_action = std::make_shared<TestFailureAction>("failure_test");
            status = failure_action->Execute(context);
            
            if (!AssertEqual("FAILURE 액션", NodeStatus::FAILURE, status))
                return TestResult("TestBasicNodeExecution", false, "FAILURE 액션 실패");
            
            // RUNNING 액션 테스트
            auto running_action = std::make_shared<TestRunningAction>("running_test", 2);
            status = running_action->Execute(context);
            
            if (!AssertEqual("RUNNING 액션 첫 실행", NodeStatus::RUNNING, status))
                return TestResult("TestBasicNodeExecution", false, "RUNNING 액션 첫 실행 실패");
            
            // 두 번째 실행
            status = running_action->Execute(context);
            
            if (!AssertEqual("RUNNING 액션 두 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestBasicNodeExecution", false, "RUNNING 액션 두 번째 실행 실패");
            
            // 카운터 확인 (SUCCESS 1번 + FAILURE 1번 + RUNNING 2번 = 4번)
            if (!AssertEqual("액션 카운터", 4, mock_ai->action_count_.load()))
                return TestResult("TestBasicNodeExecution", false, "액션 카운터 불일치");
            
            std::cout << "  ✓ 기본 노드 실행 테스트 통과\n";
            return TestResult("TestBasicNodeExecution", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestBasicNodeExecution", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestSequenceNode()
    {
        std::cout << "테스트: Sequence 노드\n";
        
        try
        {
            auto mock_ai = CreateMockAI("SequenceTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            // SUCCESS 시퀀스 테스트
            auto sequence = std::make_shared<Sequence>("test_sequence");
            sequence->AddChild(std::make_shared<TestSuccessAction>("action1"));
            sequence->AddChild(std::make_shared<TestSuccessAction>("action2"));
            
            NodeStatus status = sequence->Execute(context);
            
            if (!AssertEqual("SUCCESS 시퀀스", NodeStatus::SUCCESS, status))
                return TestResult("TestSequenceNode", false, "SUCCESS 시퀀스 실패");
            
            // FAILURE 시퀀스 테스트
            auto fail_sequence = std::make_shared<Sequence>("fail_sequence");
            fail_sequence->AddChild(std::make_shared<TestSuccessAction>("action1"));
            fail_sequence->AddChild(std::make_shared<TestFailureAction>("action2"));
            fail_sequence->AddChild(std::make_shared<TestSuccessAction>("action3")); // 실행되지 않아야 함
            
            status = fail_sequence->Execute(context);
            
            if (!AssertEqual("FAILURE 시퀀스", NodeStatus::FAILURE, status))
                return TestResult("TestSequenceNode", false, "FAILURE 시퀀스 실패");
            
            // RUNNING 시퀀스 테스트
            auto running_sequence = std::make_shared<Sequence>("running_sequence");
            running_sequence->AddChild(std::make_shared<TestSuccessAction>("action1"));
            running_sequence->AddChild(std::make_shared<TestRunningAction>("running_action", 2));
            running_sequence->AddChild(std::make_shared<TestSuccessAction>("action3"));
            
            status = running_sequence->Execute(context);
            
            if (!AssertEqual("RUNNING 시퀀스 첫 실행", NodeStatus::RUNNING, status))
                return TestResult("TestSequenceNode", false, "RUNNING 시퀀스 첫 실행 실패");
            
            // 두 번째 실행
            status = running_sequence->Execute(context);
            
            if (!AssertEqual("RUNNING 시퀀스 두 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestSequenceNode", false, "RUNNING 시퀀스 두 번째 실행 실패");
            
            std::cout << "  ✓ Sequence 노드 테스트 통과\n";
            return TestResult("TestSequenceNode", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestSequenceNode", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestSelectorNode()
    {
        std::cout << "테스트: Selector 노드\n";
        
        try
        {
            auto mock_ai = CreateMockAI("SelectorTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            // SUCCESS 셀렉터 테스트
            auto selector = std::make_shared<Selector>("test_selector");
            selector->AddChild(std::make_shared<TestFailureAction>("action1"));
            selector->AddChild(std::make_shared<TestSuccessAction>("action2"));
            selector->AddChild(std::make_shared<TestSuccessAction>("action3")); // 실행되지 않아야 함
            
            NodeStatus status = selector->Execute(context);
            
            if (!AssertEqual("SUCCESS 셀렉터", NodeStatus::SUCCESS, status))
                return TestResult("TestSelectorNode", false, "SUCCESS 셀렉터 실패");
            
            // FAILURE 셀렉터 테스트
            auto fail_selector = std::make_shared<Selector>("fail_selector");
            fail_selector->AddChild(std::make_shared<TestFailureAction>("action1"));
            fail_selector->AddChild(std::make_shared<TestFailureAction>("action2"));
            
            status = fail_selector->Execute(context);
            
            if (!AssertEqual("FAILURE 셀렉터", NodeStatus::FAILURE, status))
                return TestResult("TestSelectorNode", false, "FAILURE 셀렉터 실패");
            
            // RUNNING 셀렉터 테스트
            auto running_selector = std::make_shared<Selector>("running_selector");
            running_selector->AddChild(std::make_shared<TestFailureAction>("action1"));
            running_selector->AddChild(std::make_shared<TestRunningAction>("running_action", 2));
            running_selector->AddChild(std::make_shared<TestSuccessAction>("action3"));
            
            status = running_selector->Execute(context);
            
            if (!AssertEqual("RUNNING 셀렉터 첫 실행", NodeStatus::RUNNING, status))
                return TestResult("TestSelectorNode", false, "RUNNING 셀렉터 첫 실행 실패");
            
            // 두 번째 실행
            status = running_selector->Execute(context);
            
            if (!AssertEqual("RUNNING 셀렉터 두 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestSelectorNode", false, "RUNNING 셀렉터 두 번째 실행 실패");
            
            std::cout << "  ✓ Selector 노드 테스트 통과\n";
            return TestResult("TestSelectorNode", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestSelectorNode", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestRunningStatePersistence()
    {
        std::cout << "테스트: RUNNING 상태 지속성\n";
        
        try
        {
            auto mock_ai = CreateMockAI("RunningTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            // RUNNING 액션 생성 (5틱 필요)
            auto running_action = std::make_shared<TestRunningAction>("long_running", 5);
            
            // 첫 번째 실행
            NodeStatus status = running_action->Execute(context);
            if (!AssertEqual("첫 번째 실행", NodeStatus::RUNNING, status))
                return TestResult("TestRunningStatePersistence", false, "첫 번째 실행 실패");
            
            if (!AssertTrue("RUNNING 상태 확인", running_action->IsRunning()))
                return TestResult("TestRunningStatePersistence", false, "RUNNING 상태 확인 실패");
            
            // 연속 실행 (4번 더)
            for (int i = 0; i < 4; ++i)
            {
                status = running_action->Execute(context);
                if (i < 3) // 마지막 실행 전까지는 RUNNING
                {
                    if (!AssertEqual("연속 실행 " + std::to_string(i+2), NodeStatus::RUNNING, status))
                        return TestResult("TestRunningStatePersistence", false, "연속 실행 " + std::to_string(i+2) + " 실패");
                }
                else // 마지막 실행은 SUCCESS
                {
                    if (!AssertEqual("최종 실행", NodeStatus::SUCCESS, status))
                        return TestResult("TestRunningStatePersistence", false, "최종 실행 실패");
                }
            }
            
            if (!AssertFalse("RUNNING 상태 해제 확인", running_action->IsRunning()))
                return TestResult("TestRunningStatePersistence", false, "RUNNING 상태 해제 확인 실패");
            
            // 카운터 확인 (5번 실행)
            if (!AssertEqual("실행 카운터", 5, mock_ai->action_count_.load()))
                return TestResult("TestRunningStatePersistence", false, "실행 카운터 불일치");
            
            std::cout << "  ✓ RUNNING 상태 지속성 테스트 통과\n";
            return TestResult("TestRunningStatePersistence", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestRunningStatePersistence", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestComplexBehaviorTree()
    {
        std::cout << "테스트: 복잡한 Behavior Tree\n";
        
        try
        {
            auto mock_ai = CreateMockAI("ComplexTestAI");
            mock_ai->SetHealth(80);
            mock_ai->SetTarget(123);
            mock_ai->SetDistanceToTarget(3.0f);
            
            Context context;
            context.SetAI(mock_ai);
            
            // 복잡한 BT 생성 (몬스터 AI 시뮬레이션)
            auto root = std::make_shared<Selector>("monster_root");
            
            // 공격 시퀀스
            auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
            attack_sequence->AddChild(std::make_shared<TestHealthCondition>("health_check", 50));
            attack_sequence->AddChild(std::make_shared<TestHasTargetCondition>("has_target"));
            attack_sequence->AddChild(std::make_shared<TestInRangeCondition>("in_range", 5.0f));
            attack_sequence->AddChild(std::make_shared<TestAttackAction>("attack"));
            
            // 이동 시퀀스
            auto move_sequence = std::make_shared<Sequence>("move_sequence");
            move_sequence->AddChild(std::make_shared<TestHealthCondition>("health_check2", 50));
            move_sequence->AddChild(std::make_shared<TestHasTargetCondition>("has_target2"));
            move_sequence->AddChild(std::make_shared<TestMoveAction>("move_to_target", 3));
            
            // 순찰 시퀀스
            auto patrol_sequence = std::make_shared<Sequence>("patrol_sequence");
            patrol_sequence->AddChild(std::make_shared<TestHealthCondition>("health_check3", 50));
            patrol_sequence->AddChild(std::make_shared<TestSuccessAction>("patrol"));
            
            root->AddChild(attack_sequence);
            root->AddChild(move_sequence);
            root->AddChild(patrol_sequence);
            
            auto tree = std::make_shared<Tree>("complex_monster_bt");
            tree->SetRoot(root);
            
            // 첫 번째 실행 - 공격 시퀀스가 실행되어야 함
            NodeStatus status = tree->Execute(context);
            if (!AssertEqual("첫 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestComplexBehaviorTree", false, "첫 번째 실행 실패");
            
            // 타겟을 범위 밖으로 이동
            mock_ai->SetDistanceToTarget(10.0f);
            
            // 두 번째 실행 - 이동 시퀀스가 실행되어야 함 (RUNNING)
            status = tree->Execute(context);
            if (!AssertEqual("두 번째 실행", NodeStatus::RUNNING, status))
                return TestResult("TestComplexBehaviorTree", false, "두 번째 실행 실패");
            
            // 연속 실행 (이동 완료까지)
            for (int i = 0; i < 2; ++i)
            {
                status = tree->Execute(context);
            }
            
            if (!AssertEqual("이동 완료", NodeStatus::SUCCESS, status))
                return TestResult("TestComplexBehaviorTree", false, "이동 완료 실패");
            
            // 타겟 제거
            mock_ai->ClearTarget();
            
            // 세 번째 실행 - 순찰 시퀀스가 실행되어야 함
            status = tree->Execute(context);
            if (!AssertEqual("세 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestComplexBehaviorTree", false, "세 번째 실행 실패");
            
            // 체력이 낮아짐
            mock_ai->SetHealth(30);
            
            // 네 번째 실행 - 모든 시퀀스가 실패해야 함
            status = tree->Execute(context);
            if (!AssertEqual("네 번째 실행", NodeStatus::FAILURE, status))
                return TestResult("TestComplexBehaviorTree", false, "네 번째 실행 실패");
            
            std::cout << "  ✓ 복잡한 Behavior Tree 테스트 통과\n";
            return TestResult("TestComplexBehaviorTree", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestComplexBehaviorTree", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestTreeInitialization()
    {
        std::cout << "테스트: Tree 초기화\n";
        
        try
        {
            auto mock_ai = CreateMockAI("InitTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            auto tree = std::make_shared<Tree>("init_test_tree");
            
            // 빈 트리 테스트
            NodeStatus status = tree->Execute(context);
            if (!AssertEqual("빈 트리", NodeStatus::FAILURE, status))
                return TestResult("TestTreeInitialization", false, "빈 트리 테스트 실패");
            
            // RUNNING 액션으로 트리 구성
            auto running_action = std::make_shared<TestRunningAction>("init_running", 2);
            tree->SetRoot(running_action);
            
            // 첫 번째 실행
            status = tree->Execute(context);
            if (!AssertEqual("초기화 후 첫 실행", NodeStatus::RUNNING, status))
                return TestResult("TestTreeInitialization", false, "초기화 후 첫 실행 실패");
            
            if (!AssertTrue("트리 RUNNING 상태", tree->IsRunning()))
                return TestResult("TestTreeInitialization", false, "트리 RUNNING 상태 확인 실패");
            
            // 두 번째 실행
            status = tree->Execute(context);
            if (!AssertEqual("초기화 후 두 번째 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestTreeInitialization", false, "초기화 후 두 번째 실행 실패");
            
            if (!AssertFalse("트리 완료 상태", tree->IsRunning()))
                return TestResult("TestTreeInitialization", false, "트리 완료 상태 확인 실패");
            
            std::cout << "  ✓ Tree 초기화 테스트 통과\n";
            return TestResult("TestTreeInitialization", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestTreeInitialization", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestContextManagement()
    {
        std::cout << "테스트: Context 관리\n";
        
        try
        {
            auto mock_ai = CreateMockAI("ContextTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            // 실행 카운트 테스트
            if (!AssertEqual("초기 실행 카운트", 0, static_cast<int>(context.GetExecutionCount())))
                return TestResult("TestContextManagement", false, "초기 실행 카운트 실패");
            
            context.IncrementExecutionCount();
            context.IncrementExecutionCount();
            
            if (!AssertEqual("증가된 실행 카운트", 2, static_cast<int>(context.GetExecutionCount())))
                return TestResult("TestContextManagement", false, "증가된 실행 카운트 실패");
            
            // 현재 실행 중인 노드 테스트
            if (!AssertTrue("초기 실행 노드 비어있음", context.GetCurrentRunningNode().empty()))
                return TestResult("TestContextManagement", false, "초기 실행 노드 확인 실패");
            
            context.SetCurrentRunningNode("test_node");
            
            if (!AssertEqual("설정된 실행 노드", std::string("test_node"), context.GetCurrentRunningNode()))
                return TestResult("TestContextManagement", false, "설정된 실행 노드 확인 실패");
            
            context.ClearCurrentRunningNode();
            
            if (!AssertTrue("클리어된 실행 노드", context.GetCurrentRunningNode().empty()))
                return TestResult("TestContextManagement", false, "클리어된 실행 노드 확인 실패");
            
            // 데이터 관리 테스트
            context.SetData("test_key", std::string("test_value"));
            
            if (!AssertTrue("데이터 존재 확인", context.HasData("test_key")))
                return TestResult("TestContextManagement", false, "데이터 존재 확인 실패");
            
            std::string value = context.GetDataAs<std::string>("test_key");
            if (!AssertEqual("데이터 값 확인", std::string("test_value"), value))
                return TestResult("TestContextManagement", false, "데이터 값 확인 실패");
            
            context.RemoveData("test_key");
            
            if (!AssertFalse("데이터 제거 확인", context.HasData("test_key")))
                return TestResult("TestContextManagement", false, "데이터 제거 확인 실패");
            
            std::cout << "  ✓ Context 관리 테스트 통과\n";
            return TestResult("TestContextManagement", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestContextManagement", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestEngineRegistration()
    {
        std::cout << "테스트: Engine 등록\n";
        
        try
        {
            Engine engine;
            
            // 빈 엔진 테스트
            if (!AssertEqual("초기 등록된 트리 수", 0, static_cast<int>(engine.GetRegisteredTrees())))
                return TestResult("TestEngineRegistration", false, "초기 등록된 트리 수 실패");
            
            // 트리 등록 테스트
            auto tree1 = std::make_shared<Tree>("test_tree_1");
            auto tree2 = std::make_shared<Tree>("test_tree_2");
            
            engine.RegisterTree("tree1", tree1);
            engine.RegisterTree("tree2", tree2);
            
            if (!AssertEqual("등록 후 트리 수", 2, static_cast<int>(engine.GetRegisteredTrees())))
                return TestResult("TestEngineRegistration", false, "등록 후 트리 수 실패");
            
            // 트리 가져오기 테스트
            auto retrieved_tree = engine.GetTree("tree1");
            if (!AssertTrue("트리 가져오기", retrieved_tree != nullptr))
                return TestResult("TestEngineRegistration", false, "트리 가져오기 실패");
            
            if (!AssertEqual("트리 이름 확인", std::string("test_tree_1"), retrieved_tree->GetName()))
                return TestResult("TestEngineRegistration", false, "트리 이름 확인 실패");
            
            // 존재하지 않는 트리 테스트
            auto non_existent = engine.GetTree("non_existent");
            if (!AssertTrue("존재하지 않는 트리", non_existent == nullptr))
                return TestResult("TestEngineRegistration", false, "존재하지 않는 트리 확인 실패");
            
            // 트리 실행 테스트
            auto mock_ai = CreateMockAI("EngineTestAI");
            Context context;
            context.SetAI(mock_ai);
            
            auto success_action = std::make_shared<TestSuccessAction>("engine_test_action");
            tree1->SetRoot(success_action);
            
            NodeStatus status = engine.ExecuteTree("tree1", context);
            if (!AssertEqual("엔진 트리 실행", NodeStatus::SUCCESS, status))
                return TestResult("TestEngineRegistration", false, "엔진 트리 실행 실패");
            
            // 트리 등록 해제 테스트
            engine.UnregisterTree("tree1");
            
            if (!AssertEqual("등록 해제 후 트리 수", 1, static_cast<int>(engine.GetRegisteredTrees())))
                return TestResult("TestEngineRegistration", false, "등록 해제 후 트리 수 실패");
            
            std::cout << "  ✓ Engine 등록 테스트 통과\n";
            return TestResult("TestEngineRegistration", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestEngineRegistration", false, std::string("예외 발생: ") + e.what());
        }
    }

    // 헬퍼 함수들
    std::shared_ptr<MockAIExecutor> BehaviorTreeTestSuite::CreateMockAI(const std::string& name)
    {
        return std::make_shared<MockAIExecutor>(name);
    }

    std::shared_ptr<Tree> BehaviorTreeTestSuite::CreateSimpleTree()
    {
        auto tree = std::make_shared<Tree>("simple_tree");
        auto root = std::make_shared<Selector>("simple_root");
        root->AddChild(std::make_shared<TestSuccessAction>("simple_action"));
        tree->SetRoot(root);
        return tree;
    }

    std::shared_ptr<Tree> BehaviorTreeTestSuite::CreateComplexTree()
    {
        auto tree = std::make_shared<Tree>("complex_tree");
        auto root = std::make_shared<Selector>("complex_root");
        
        auto sequence = std::make_shared<Sequence>("complex_sequence");
        sequence->AddChild(std::make_shared<TestHealthCondition>("health", 50));
        sequence->AddChild(std::make_shared<TestRunningAction>("running", 3));
        sequence->AddChild(std::make_shared<TestSuccessAction>("success"));
        
        root->AddChild(sequence);
        root->AddChild(std::make_shared<TestFailureAction>("fallback"));
        
        tree->SetRoot(root);
        return tree;
    }

    void BehaviorTreeTestSuite::PrintTestResults(const std::vector<TestResult>& results)
    {
        std::cout << "\n=== 테스트 결과 ===\n";
        
        int passed = 0;
        int failed = 0;
        
        for (const auto& result : results)
        {
            if (result.passed)
            {
                std::cout << "✓ " << result.test_name << " - 통과\n";
                passed++;
            }
            else
            {
                std::cout << "✗ " << result.test_name << " - 실패: " << result.error_message << "\n";
                failed++;
            }
        }
        
        std::cout << "\n총 " << results.size() << "개 테스트 중 " << passed << "개 통과, " << failed << "개 실패\n";
        
        if (failed == 0)
        {
            std::cout << "🎉 모든 테스트가 통과했습니다!\n";
        }
        else
        {
            std::cout << "⚠️  " << failed << "개의 테스트가 실패했습니다.\n";
        }
    }

    // Assert 헬퍼 함수들
    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, NodeStatus expected, NodeStatus actual)
    {
        if (expected == actual)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 " << static_cast<int>(expected) 
                  << ", 실제 " << static_cast<int>(actual) << "\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, bool expected, bool actual)
    {
        if (expected == actual)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 " << (expected ? "true" : "false") 
                  << ", 실제 " << (actual ? "true" : "false") << "\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, int expected, int actual)
    {
        if (expected == actual)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 " << expected 
                  << ", 실제 " << actual << "\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, const std::string& expected, const std::string& actual)
    {
        if (expected == actual)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 '" << expected 
                  << "', 실제 '" << actual << "'\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, size_t expected, size_t actual)
    {
        if (expected == actual)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 " << expected 
                  << ", 실제 " << actual << "\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertEqual(const std::string& test_name, float expected, float actual)
    {
        const float epsilon = 0.0001f;
        if (std::abs(expected - actual) < epsilon)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 " << expected 
                  << ", 실제 " << actual << "\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertTrue(const std::string& test_name, bool condition)
    {
        if (condition)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 true, 실제 false\n";
        return false;
    }

    bool BehaviorTreeTestSuite::AssertFalse(const std::string& test_name, bool condition)
    {
        if (!condition)
            return true;
        
        std::cout << "    ✗ " << test_name << ": 예상 false, 실제 true\n";
        return false;
    }

    TestResult BehaviorTreeTestSuite::TestBlackboardFunctionality()
    {
        std::cout << "테스트: Blackboard 기능\n";
        
        try
        {
            // Blackboard 직접 테스트
            Blackboard bb;
            
            // 기본 데이터 설정/조회
            bb.SetData("health", 100);
            bb.SetData("name", std::string("TestPlayer"));
            bb.SetData("level", 5);
            bb.SetData("is_alive", true);
            
            if (!AssertEqual("데이터 개수", size_t(4), bb.Size()))
                return TestResult("TestBlackboardFunctionality", false, "데이터 개수 불일치");
            
            if (!AssertTrue("데이터 존재 확인", bb.HasData("health")))
                return TestResult("TestBlackboardFunctionality", false, "데이터 존재 확인 실패");
            
            // 타입 안전한 데이터 조회
            int health = bb.GetDataAs<int>("health");
            if (!AssertEqual("정수 데이터 조회", 100, health))
                return TestResult("TestBlackboardFunctionality", false, "정수 데이터 조회 실패");
            
            std::string name = bb.GetDataAs<std::string>("name");
            if (!AssertEqual("문자열 데이터 조회", std::string("TestPlayer"), name))
                return TestResult("TestBlackboardFunctionality", false, "문자열 데이터 조회 실패");
            
            bool is_alive = bb.GetDataAs<bool>("is_alive");
            if (!AssertTrue("불린 데이터 조회", is_alive))
                return TestResult("TestBlackboardFunctionality", false, "불린 데이터 조회 실패");
            
            // 존재하지 않는 데이터 조회 (기본값)
            int missing = bb.GetDataAs<int>("missing_key");
            if (!AssertEqual("기본값 반환", 0, missing))
                return TestResult("TestBlackboardFunctionality", false, "기본값 반환 실패");
            
            // 데이터 삭제
            bb.RemoveData("level");
            if (!AssertEqual("데이터 삭제 후 개수", size_t(3), bb.Size()))
                return TestResult("TestBlackboardFunctionality", false, "데이터 삭제 실패");
            
            if (!AssertFalse("삭제된 데이터 존재 확인", bb.HasData("level")))
                return TestResult("TestBlackboardFunctionality", false, "삭제된 데이터 존재 확인 실패");
            
            // 키 목록 조회
            auto keys = bb.GetKeys();
            if (!AssertEqual("키 개수", size_t(3), keys.size()))
                return TestResult("TestBlackboardFunctionality", false, "키 개수 불일치");
            
            // 특정 타입 데이터 조회
            auto int_data = bb.GetDataOfType<int>();
            if (!AssertEqual("정수 타입 데이터 개수", size_t(1), int_data.size()))
                return TestResult("TestBlackboardFunctionality", false, "정수 타입 데이터 개수 불일치");
            
            // Context를 통한 Blackboard 테스트
            Context context;
            context.SetData("context_health", 200);
            context.SetData("context_name", std::string("ContextPlayer"));
            
            if (!AssertEqual("Context 데이터 개수", size_t(2), context.GetDataSize()))
                return TestResult("TestBlackboardFunctionality", false, "Context 데이터 개수 불일치");
            
            int context_health = context.GetDataAs<int>("context_health");
            if (!AssertEqual("Context 정수 데이터", 200, context_health))
                return TestResult("TestBlackboardFunctionality", false, "Context 정수 데이터 실패");
            
            // Blackboard 직접 접근
            Blackboard& context_bb = context.GetBlackboard();
            context_bb.SetData("direct_access", 300);
            
            if (!AssertEqual("직접 접근 데이터", 300, context_bb.GetDataAs<int>("direct_access")))
                return TestResult("TestBlackboardFunctionality", false, "직접 접근 데이터 실패");
            
            // 모든 데이터 삭제
            bb.Clear();
            if (!AssertTrue("모든 데이터 삭제", bb.Empty()))
                return TestResult("TestBlackboardFunctionality", false, "모든 데이터 삭제 실패");
            
            std::cout << "  ✓ Blackboard 기능 테스트 통과\n";
            return TestResult("TestBlackboardFunctionality", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestBlackboardFunctionality", false, std::string("예외 발생: ") + e.what());
        }
    }

    TestResult BehaviorTreeTestSuite::TestEnvironmentInfoFunctionality()
    {
        std::cout << "테스트: EnvironmentInfo 기능\n";
        
        try
        {
            EnvironmentInfo env_info;
            
            // 초기 상태 확인
            if (!AssertTrue("초기 상태 - 시야 확보", env_info.has_line_of_sight))
                return TestResult("TestEnvironmentInfoFunctionality", false, "초기 시야 확보 상태 실패");
            
            if (!AssertFalse("초기 상태 - 적 없음", env_info.HasEnemy()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "초기 적 상태 실패");
            
            // 주변 플레이어 추가
            env_info.nearby_players.push_back(1001);
            env_info.nearby_players.push_back(1002);
            
            if (!AssertTrue("주변 플레이어 있음", env_info.HasNearbyPlayers()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "주변 플레이어 확인 실패");
            
            if (!AssertEqual("주변 플레이어 개수", size_t(2), env_info.nearby_players.size()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "주변 플레이어 개수 실패");
            
            // 주변 몬스터 추가
            env_info.nearby_monsters.push_back(2001);
            env_info.nearby_monsters.push_back(2002);
            env_info.nearby_monsters.push_back(2003);
            
            if (!AssertTrue("주변 몬스터 있음", env_info.HasNearbyMonsters()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "주변 몬스터 확인 실패");
            
            if (!AssertEqual("주변 몬스터 개수", size_t(3), env_info.nearby_monsters.size()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "주변 몬스터 개수 실패");
            
            // 가장 가까운 적 설정
            env_info.nearest_enemy_id = 2001;
            env_info.nearest_enemy_distance = 5.5f;
            
            if (!AssertTrue("적 있음", env_info.HasEnemy()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "적 존재 확인 실패");
            
            if (!AssertEqual("적 거리", 5.5f, env_info.nearest_enemy_distance))
                return TestResult("TestEnvironmentInfoFunctionality", false, "적 거리 실패");
            
            // 거리 기반 판단
            if (!AssertTrue("공격 범위 내", env_info.IsEnemyInRange(10.0f)))
                return TestResult("TestEnvironmentInfoFunctionality", false, "공격 범위 내 판단 실패");
            
            if (!AssertFalse("공격 범위 밖", env_info.IsEnemyInRange(3.0f)))
                return TestResult("TestEnvironmentInfoFunctionality", false, "공격 범위 밖 판단 실패");
            
            // 적 타입 확인
            if (!AssertTrue("가장 가까운 적이 몬스터", env_info.IsNearestEnemyMonster()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "가장 가까운 적이 몬스터 확인 실패");
            
            if (!AssertFalse("가장 가까운 적이 플레이어 아님", env_info.IsNearestEnemyPlayer()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "가장 가까운 적이 플레이어 확인 실패");
            
            // 플레이어를 가장 가까운 적으로 설정
            env_info.nearest_enemy_id = 1001;
            env_info.nearest_enemy_distance = 3.0f;
            
            if (!AssertTrue("가장 가까운 적이 플레이어", env_info.IsNearestEnemyPlayer()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "가장 가까운 적이 플레이어 확인 실패");
            
            if (!AssertFalse("가장 가까운 적이 몬스터 아님", env_info.IsNearestEnemyMonster()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "가장 가까운 적이 몬스터 확인 실패");
            
            // 장애물 추가
            env_info.obstacles.push_back(3001);
            env_info.obstacles.push_back(3002);
            
            if (!AssertTrue("장애물 있음", env_info.HasObstacles()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "장애물 확인 실패");
            
            if (!AssertEqual("장애물 개수", size_t(2), env_info.obstacles.size()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "장애물 개수 실패");
            
            // 시야 차단
            env_info.has_line_of_sight = false;
            
            if (!AssertFalse("시야 차단됨", env_info.has_line_of_sight))
                return TestResult("TestEnvironmentInfoFunctionality", false, "시야 차단 확인 실패");
            
            // 모든 데이터 클리어
            env_info.Clear();
            
            if (!AssertFalse("클리어 후 - 주변 플레이어 없음", env_info.HasNearbyPlayers()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "클리어 후 주변 플레이어 확인 실패");
            
            if (!AssertFalse("클리어 후 - 주변 몬스터 없음", env_info.HasNearbyMonsters()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "클리어 후 주변 몬스터 확인 실패");
            
            if (!AssertFalse("클리어 후 - 장애물 없음", env_info.HasObstacles()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "클리어 후 장애물 확인 실패");
            
            if (!AssertFalse("클리어 후 - 적 없음", env_info.HasEnemy()))
                return TestResult("TestEnvironmentInfoFunctionality", false, "클리어 후 적 확인 실패");
            
            if (!AssertTrue("클리어 후 - 시야 확보", env_info.has_line_of_sight))
                return TestResult("TestEnvironmentInfoFunctionality", false, "클리어 후 시야 확보 확인 실패");
            
            std::cout << "  ✓ EnvironmentInfo 기능 테스트 통과\n";
            return TestResult("TestEnvironmentInfoFunctionality", true);
        }
        catch (const std::exception& e)
        {
            return TestResult("TestEnvironmentInfoFunctionality", false, std::string("예외 발생: ") + e.what());
        }
    }

    // 메인 테스트 실행 함수
    void RunBehaviorTreeTests()
    {
        BehaviorTreeTestSuite test_suite;
        auto results = test_suite.RunAllTests();
        test_suite.PrintTestResults(results);
    }

} // namespace test
} // namespace bt
