#include "server.h"
#include <iostream>
#include <signal.h>
#include <csignal>

namespace bt {

// 전역 서버 인스턴스 (시그널 핸들링용)
std::unique_ptr<Server> g_server = nullptr;

// 시그널 핸들러
void signal_handler(int signal) {
    std::cout << "\n시그널 " << signal << " 수신됨. 서버를 종료합니다...\n";
    if (g_server) {
        g_server->stop();
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
    ServerConfig config;
    config.host = "0.0.0.0";
    config.port = 8080;
    config.max_clients = 1000;
    config.worker_threads = 4;
    config.debug_mode = true;
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            config.port = std::stoi(argv[++i]);
        } else if (arg == "--host" && i + 1 < argc) {
            config.host = argv[++i];
        } else if (arg == "--max-clients" && i + 1 < argc) {
            config.max_clients = std::stoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.worker_threads = std::stoi(argv[++i]);
        } else if (arg == "--debug") {
            config.debug_mode = true;
        } else if (arg == "--help") {
            std::cout << "사용법: " << argv[0] << " [옵션]\n";
            std::cout << "옵션:\n";
            std::cout << "  --port <포트>        서버 포트 (기본값: 8080)\n";
            std::cout << "  --host <호스트>      서버 호스트 (기본값: 0.0.0.0)\n";
            std::cout << "  --max-clients <수>   최대 클라이언트 수 (기본값: 1000)\n";
            std::cout << "  --threads <수>       워커 스레드 수 (기본값: 4)\n";
            std::cout << "  --debug             디버그 모드 활성화\n";
            std::cout << "  --help              이 도움말 표시\n";
            return 0;
        }
    }
    
    // 서버 생성 및 시작
    g_server = std::make_unique<Server>(config);
    
    if (!g_server->start()) {
        std::cerr << "서버 시작 실패!\n";
        return 1;
    }
    
    std::cout << "서버가 포트 " << config.port << "에서 실행 중입니다.\n";
    std::cout << "종료하려면 Ctrl+C를 누르세요.\n";
    
    // 서버가 실행 중인 동안 대기
    while (g_server->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "서버가 정상적으로 종료되었습니다.\n";
    return 0;
}
