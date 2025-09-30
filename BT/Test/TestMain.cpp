#include "BehaviorTreeTests.h"
#include "PerformanceTest.cpp"
#include <iostream>
#include <chrono>

int main()
{
    std::cout << "Behavior Tree Unit Test Suite\n";
    std::cout << "=============================\n\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try
    {
        bt::test::RunBehaviorTreeTests();
        bt::test::RunPerformanceTest();
    }
    catch (const std::exception& e)
    {
        std::cerr << "테스트 실행 중 예외 발생: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "알 수 없는 예외가 발생했습니다." << std::endl;
        return 1;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n테스트 실행 시간: " << duration.count() << "ms\n";
    
    return 0;
}
