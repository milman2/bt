#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "AsioTestClient.h"

namespace bt
{

    void print_usage(const char* program_name)
    {
        std::cout << "BT MMORPG 서버 테스트 클라이언트 (Boost.Asio)\n";
        std::cout << "사용법: " << program_name << " [옵션]\n\n";
        std::cout << "옵션:\n";
        std::cout << "  --host <호스트>        서버 호스트 (기본값: 127.0.0.1)\n";
        std::cout << "  --port <포트>          서버 포트 (기본값: 7000)\n";
        std::cout << "  --test <테스트명>      실행할 테스트\n";
        std::cout << "    연결 테스트: connection\n";
        std::cout << "    플레이어 접속 테스트: join\n";
        std::cout << "    플레이어 이동 테스트: move\n";
        std::cout << "    플레이어 공격 테스트: attack\n";
        std::cout << "    BT 실행 테스트: bt\n";
        std::cout << "    몬스터 업데이트 테스트: update\n";
        std::cout << "    연결해제 테스트: disconnect\n";
        std::cout << "    전체 테스트: all\n";
        std::cout << "    스트레스 테스트: stress\n";
        std::cout << "  --stress-connections <수>  스트레스 테스트 연결 수 (기본값: 10)\n";
        std::cout << "  --stress-duration <초>     스트레스 테스트 지속 시간 (기본값: 30)\n";
        std::cout << "  --player-name <이름>    플레이어 이름 (기본값: test_player)\n";
        std::cout << "  --target-id <ID>        공격 대상 ID (기본값: 1)\n";
        std::cout << "  --bt-name <이름>        BT 이름 (기본값: goblin_bt)\n";
        std::cout << "  --verbose              상세 로그 출력\n";
        std::cout << "  --help                 이 도움말 표시\n\n";
        std::cout << "예시:\n";
        std::cout << "  " << program_name << " --test all\n";
        std::cout << "  " << program_name << " --test stress --stress-connections 50\n";
        std::cout << "  " << program_name << " --test join --player-name myplayer\n";
        std::cout << "  " << program_name << " --test attack --target-id 1\n";
    }

} // namespace bt

int main(int argc, char* argv[])
{
    using namespace bt;

    // 기본 설정
    AsioClientConfig config;
    std::string      test_type          = "all";
    std::string      player_name        = "test_player";
    uint32_t         target_id          = 1;
    std::string      bt_name            = "goblin_bt";
    int              stress_connections = 10;
    int              stress_duration    = 30;
    bool             verbose            = false;

    // 명령행 인수 파싱
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--host" && i + 1 < argc)
        {
            config.server_host = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            config.server_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--test" && i + 1 < argc)
        {
            test_type = argv[++i];
        }
        else if (arg == "--stress-connections" && i + 1 < argc)
        {
            stress_connections = std::stoi(argv[++i]);
        }
        else if (arg == "--stress-duration" && i + 1 < argc)
        {
            stress_duration = std::stoi(argv[++i]);
        }
        else if (arg == "--player-name" && i + 1 < argc)
        {
            player_name = argv[++i];
        }
        else if (arg == "--target-id" && i + 1 < argc)
        {
            target_id = std::stoi(argv[++i]);
        }
        else if (arg == "--bt-name" && i + 1 < argc)
        {
            bt_name = argv[++i];
        }
        else if (arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg == "--help")
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            std::cerr << "알 수 없는 옵션: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "=== BT MMORPG 서버 테스트 클라이언트 (Boost.Asio) ===\n";
    std::cout << "서버: " << config.server_host << ":" << config.server_port << "\n";
    std::cout << "테스트: " << test_type << "\n";
    if (verbose)
    {
        std::cout << "상세 로그: 활성화\n";
    }
    std::cout << "\n";

    // 테스트 클라이언트 생성
    AsioTestClient client(config);
    client.SetVerbose(verbose);

    bool test_result = false;

    try
    {
        if (test_type == "connection")
        {
            test_result = client.TestConnection();
        }
        else if (test_type == "join")
        {
            test_result = client.TestPlayerJoin(player_name);
        }
        else if (test_type == "move")
        {
            test_result = client.TestPlayerMove(100.0f, 200.0f, 300.0f);
        }
        else if (test_type == "attack")
        {
            test_result = client.TestPlayerAttack(target_id);
        }
        else if (test_type == "bt")
        {
            test_result = client.TestBTExecute(bt_name);
        }
        else if (test_type == "update")
        {
            test_result = client.TestMonsterUpdate();
        }
        else if (test_type == "disconnect")
        {
            test_result = client.TestDisconnect();
        }
        else if (test_type == "all")
        {
            test_result = client.RunAutomatedTest();
        }
        else if (test_type == "stress")
        {
            test_result = client.RunStressTest(stress_connections, stress_duration);
        }
        else
        {
            std::cerr << "알 수 없는 테스트 타입: " << test_type << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "테스트 실행 중 오류 발생: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== 테스트 완료 ===\n";
    std::cout << "결과: " << (test_result ? "성공" : "실패") << std::endl;

    return test_result ? 0 : 1;
}
