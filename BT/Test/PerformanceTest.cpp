#include "BehaviorTreeTests.h"
#include <chrono>
#include <iostream>

namespace bt
{
namespace test
{

    void RunPerformanceTest()
    {
        std::cout << "\n=== BT 라이브러리 성능 테스트 ===\n";
        
        // 복잡한 BT 생성
        auto mock_ai = std::make_shared<MockAIExecutor>("PerformanceTestAI");
        Context context;
        context.SetAI(mock_ai);
        
        auto tree = std::make_shared<Tree>("performance_test_tree");
        
        // 깊은 중첩 구조의 BT 생성
        auto root_selector = std::make_shared<Selector>("root");
        
        // 첫 번째 시퀀스 (공격 시퀀스)
        auto attack_sequence = std::make_shared<Sequence>("attack_sequence");
        for (int i = 0; i < 5; ++i)
        {
            attack_sequence->AddChild(std::make_shared<TestSuccessAction>("attack_action_" + std::to_string(i)));
        }
        root_selector->AddChild(attack_sequence);
        
        // 두 번째 시퀀스 (이동 시퀀스)
        auto move_sequence = std::make_shared<Sequence>("move_sequence");
        for (int i = 0; i < 3; ++i)
        {
            move_sequence->AddChild(std::make_shared<TestSuccessAction>("move_action_" + std::to_string(i)));
        }
        root_selector->AddChild(move_sequence);
        
        // 세 번째 시퀀스 (대기 시퀀스)
        auto wait_sequence = std::make_shared<Sequence>("wait_sequence");
        wait_sequence->AddChild(std::make_shared<TestFailureAction>("wait_action"));
        root_selector->AddChild(wait_sequence);
        
        tree->SetRoot(root_selector);
        
        // 성능 측정
        const int iterations = 100000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i)
        {
            tree->Execute(context);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double total_time_ms = duration.count() / 1000.0;
        double avg_time_us = duration.count() / static_cast<double>(iterations);
        double executions_per_second = iterations / (total_time_ms / 1000.0);
        
        std::cout << "성능 테스트 결과:\n";
        std::cout << "  - 총 실행 횟수: " << iterations << "\n";
        std::cout << "  - 총 실행 시간: " << total_time_ms << " ms\n";
        std::cout << "  - 평균 실행 시간: " << avg_time_us << " μs\n";
        std::cout << "  - 초당 실행 횟수: " << static_cast<int>(executions_per_second) << " executions/sec\n";
        
        // Blackboard 성능 테스트
        std::cout << "\nBlackboard 성능 테스트:\n";
        Blackboard bb;
        
        start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i)
        {
            bb.SetData("key_" + std::to_string(i % 100), i);
            bb.GetDataAs<int>("key_" + std::to_string(i % 100));
            if (i % 10 == 0)
            {
                bb.RemoveData("key_" + std::to_string(i % 100));
            }
        }
        
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        total_time_ms = duration.count() / 1000.0;
        avg_time_us = duration.count() / static_cast<double>(iterations);
        executions_per_second = iterations / (total_time_ms / 1000.0);
        
        std::cout << "  - 총 연산 횟수: " << iterations << "\n";
        std::cout << "  - 총 실행 시간: " << total_time_ms << " ms\n";
        std::cout << "  - 평균 연산 시간: " << avg_time_us << " μs\n";
        std::cout << "  - 초당 연산 횟수: " << static_cast<int>(executions_per_second) << " operations/sec\n";
        
        std::cout << "\n=== 성능 테스트 완료 ===\n";
    }

} // namespace test
} // namespace bt
