#pragma once

#include "../Node.h"
#include "../Tree.h"
#include "../Context.h"
#include "../Engine.h"
#include "../Control/Sequence.h"
#include "../Control/Selector.h"
#include "TestNodes.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

namespace bt
{
namespace test
{

    // 테스트 결과 구조체
    struct TestResult
    {
        std::string test_name;
        bool passed;
        std::string error_message;
        
        TestResult(const std::string& name, bool pass, const std::string& error = "")
            : test_name(name), passed(pass), error_message(error) {}
    };

    // 테스트 스위트 클래스
    class BehaviorTreeTestSuite
    {
    public:
        BehaviorTreeTestSuite() = default;
        ~BehaviorTreeTestSuite() = default;

        // 모든 테스트 실행
        std::vector<TestResult> RunAllTests();
        
        // 개별 테스트들
        TestResult TestBasicNodeExecution();
        TestResult TestSequenceNode();
        TestResult TestSelectorNode();
        TestResult TestRunningStatePersistence();
        TestResult TestComplexBehaviorTree();
        TestResult TestTreeInitialization();
        TestResult TestContextManagement();
        TestResult TestEngineRegistration();
        TestResult TestBlackboardFunctionality();
        TestResult TestEnvironmentInfoFunctionality();
        
        // 헬퍼 함수들
        std::shared_ptr<MockAIExecutor> CreateMockAI(const std::string& name = "TestAI");
        std::shared_ptr<Tree> CreateSimpleTree();
        std::shared_ptr<Tree> CreateComplexTree();
        
        // 테스트 결과 출력
        void PrintTestResults(const std::vector<TestResult>& results);
        
    private:
        // 테스트 헬퍼
        bool AssertEqual(const std::string& test_name, NodeStatus expected, NodeStatus actual);
        bool AssertEqual(const std::string& test_name, bool expected, bool actual);
        bool AssertEqual(const std::string& test_name, int expected, int actual);
        bool AssertEqual(const std::string& test_name, const std::string& expected, const std::string& actual);
        bool AssertEqual(const std::string& test_name, size_t expected, size_t actual);
        bool AssertEqual(const std::string& test_name, float expected, float actual);
        bool AssertTrue(const std::string& test_name, bool condition);
        bool AssertFalse(const std::string& test_name, bool condition);
    };

    // 테스트 실행 함수
    void RunBehaviorTreeTests();
    void RunPerformanceTest();

} // namespace test
} // namespace bt
