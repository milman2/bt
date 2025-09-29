#include "asio_server.h"
#include <iostream>
#include <signal.h>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>

namespace bt {

// 전역 서버 인스턴스 (시그널 핸들링용)
std::unique_ptr<AsioServer> g_server = nullptr;
std::atomic<bool> g_shutdown_requested{false};

// 시그널 핸들러
void signal_handler(int signal) {
    std::cout << "\n시그널 " << signal << " 수신됨. 서버를 종료합니다...\n";
    
    // 종료 요청 플래그 설정
    g_shutdown_requested.store(true);
    
    // 서버 종료 요청
    if (g_server) {
        try {
            g_server->stop();
        } catch (const std::exception& e) {
            std::cerr << "서버 종료 중 오류 발생: " << e.what() << std::endl;
        }
    }
    
    // 강제 종료를 위한 추가 시그널 핸들러 등록
    if (signal == SIGTERM) {
        std::signal(SIGTERM, [](int) {
            std::cout << "\n강제 종료 시그널 수신. 즉시 종료합니다.\n";
            std::exit(1);
        });
    }
}

} // namespace bt

int main(int argc, char* argv[]) {
    using namespace bt;
    
    // 시그널 핸들러 등록
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "=== BT MMORPG 서버 시작 ===\n";
    
    // 서버 설정
    AsioServerConfig config;
    config.port = 8080;
    config.max_clients = 1000;
    config.worker_threads = 4;
    config.debug_mode = true;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            config.port = std::stoi(argv[++i]);
        } else if (arg == "--max-clients" && i + 1 < argc) {
            config.max_clients = std::stoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.worker_threads = std::stoi(argv[++i]);
        } else if (arg == "--debug") {
            config.debug_mode = true;
        } else if (arg == "--help") {
            std::cout << "사용법: " << argv[0] << " [옵션]\n";
            std::cout << "옵션:\n";
            std::cout << "  --port <포트>          서버 포트 (기본값: 8080)\n";
            std::cout << "  --max-clients <수>     최대 클라이언트 수 (기본값: 1000)\n";
            std::cout << "  --threads <수>         워커 스레드 수 (기본값: 4)\n";
            std::cout << "  --debug               디버그 모드 활성화\n";
            std::cout << "  --help                이 도움말 표시\n";
            return 0;
        }
    }
    
    // 서버 생성 및 시작
    g_server = std::make_unique<AsioServer>(config);
    
    if (!g_server->start()) {
        std::cerr << "서버 시작 실패!\n";
        return 1;
    }
    
    std::cout << "서버가 포트 " << config.port << "에서 실행 중입니다.\n";
    std::cout << "종료하려면 Ctrl+C를 누르세요.\n";
    
    // 서버가 실행 중인 동안 대기 (타임아웃 메커니즘 추가)
    auto start_time = std::chrono::steady_clock::now();
    const auto max_runtime = std::chrono::hours(24); // 24시간 최대 실행 시간
    const auto shutdown_timeout = std::chrono::seconds(30); // 30초 종료 타임아웃
    auto shutdown_start_time = std::chrono::steady_clock::now();
    bool shutdown_timeout_reached = false;
    
    while (g_server->is_running() && !g_shutdown_requested.load()) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - start_time;
        
        // 최대 실행 시간 초과 시 자동 종료
        if (elapsed > max_runtime) {
            std::cout << "최대 실행 시간(24시간) 초과. 서버를 자동 종료합니다." << std::endl;
            g_server->stop();
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 종료 요청이 있었지만 서버가 아직 실행 중인 경우
    if (g_shutdown_requested.load() && g_server->is_running()) {
        std::cout << "종료 요청 수신. 서버 종료를 기다리는 중..." << std::endl;
        
        while (g_server->is_running() && !shutdown_timeout_reached) {
            auto current_time = std::chrono::steady_clock::now();
            auto shutdown_elapsed = current_time - shutdown_start_time;
            
            if (shutdown_elapsed > shutdown_timeout) {
                std::cout << "종료 타임아웃(" << shutdown_timeout.count() << "초) 초과. 강제 종료합니다." << std::endl;
                shutdown_timeout_reached = true;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "서버가 정상적으로 종료되었습니다.\n";
    return 0;
}
