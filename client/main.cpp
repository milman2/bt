#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "TestClient.h"

namespace bt
{

    void print_usage(const char* program_name)
    {
        std::cout << "BT MMORPG AI 플레이어 클라이언트 (Boost.Asio)\n";
        std::cout << "사용법: " << program_name << " [옵션]\n\n";
        std::cout << "옵션:\n";
        std::cout << "  --host <호스트>        서버 호스트 (기본값: 127.0.0.1)\n";
        std::cout << "  --port <포트>          서버 포트 (기본값: 7000)\n";
        std::cout << "  --name <이름>          플레이어 이름 (기본값: AI_Player)\n";
        std::cout << "  --spawn-x <x>          스폰 X 좌표 (기본값: 0.0)\n";
        std::cout << "  --spawn-z <z>          스폰 Z 좌표 (기본값: 0.0)\n";
        std::cout << "  --patrol-radius <반경>  순찰 반경 (기본값: 50.0)\n";
        std::cout << "  --detection-range <범위> 탐지 범위 (기본값: 30.0)\n";
        std::cout << "  --attack-range <범위>   공격 범위 (기본값: 5.0)\n";
        std::cout << "  --move-speed <속도>     이동 속도 (기본값: 3.0)\n";
        std::cout << "  --health <체력>         최대 체력 (기본값: 100)\n";
        std::cout << "  --damage <데미지>       공격력 (기본값: 20)\n";
        std::cout << "  --duration <초>         실행 시간 (기본값: 0 = 무제한)\n";
        std::cout << "  --verbose              상세 로그 출력\n";
        std::cout << "  --help                 이 도움말 표시\n\n";
        std::cout << "예시:\n";
        std::cout << "  " << program_name << " --name Warrior --spawn-x 100 --spawn-z 100\n";
        std::cout << "  " << program_name << " --patrol-radius 100 --detection-range 50 --verbose\n";
        std::cout << "  " << program_name << " --duration 300 --name Hunter\n";
    }

} // namespace bt

int main(int argc, char* argv[])
{
    using namespace bt;

    // 기본 설정
    PlayerAIConfig config;
    int duration = 0;  // 0 = 무제한
    bool verbose = false;

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
        else if (arg == "--name" && i + 1 < argc)
        {
            config.player_name = argv[++i];
        }
        else if (arg == "--spawn-x" && i + 1 < argc)
        {
            config.spawn_x = std::stof(argv[++i]);
        }
        else if (arg == "--spawn-z" && i + 1 < argc)
        {
            config.spawn_z = std::stof(argv[++i]);
        }
        else if (arg == "--patrol-radius" && i + 1 < argc)
        {
            config.patrol_radius = std::stof(argv[++i]);
        }
        else if (arg == "--detection-range" && i + 1 < argc)
        {
            config.detection_range = std::stof(argv[++i]);
        }
        else if (arg == "--attack-range" && i + 1 < argc)
        {
            config.attack_range = std::stof(argv[++i]);
        }
        else if (arg == "--move-speed" && i + 1 < argc)
        {
            config.move_speed = std::stof(argv[++i]);
        }
        else if (arg == "--health" && i + 1 < argc)
        {
            config.health = std::stoi(argv[++i]);
        }
        else if (arg == "--damage" && i + 1 < argc)
        {
            config.damage = std::stoi(argv[++i]);
        }
        else if (arg == "--duration" && i + 1 < argc)
        {
            duration = std::stoi(argv[++i]);
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

    std::cout << "=== BT MMORPG AI 플레이어 클라이언트 (Boost.Asio) ===\n";
    std::cout << "서버: " << config.server_host << ":" << config.server_port << "\n";
    std::cout << "플레이어: " << config.player_name << "\n";
    std::cout << "스폰 위치: (" << config.spawn_x << ", 0, " << config.spawn_z << ")\n";
    std::cout << "순찰 반경: " << config.patrol_radius << "\n";
    std::cout << "탐지 범위: " << config.detection_range << "\n";
    std::cout << "공격 범위: " << config.attack_range << "\n";
    std::cout << "이동 속도: " << config.move_speed << "\n";
    std::cout << "체력: " << config.health << " / 공격력: " << config.damage << "\n";
    if (duration > 0)
    {
        std::cout << "실행 시간: " << duration << "초\n";
    }
    else
    {
        std::cout << "실행 시간: 무제한\n";
    }
    if (verbose)
    {
        std::cout << "상세 로그: 활성화\n";
    }
    std::cout << "\n";

    // AI 플레이어 클라이언트 생성 (shared_ptr로 생성)
    auto client = std::make_shared<TestClient>(config);
    client->SetVerbose(verbose);
    
    // shared_from_this() 사용을 위해 context에 AI 설정
    client->SetContextAI();

    try
    {
        // 서버 연결
        std::cout << "서버에 연결 중...\n";
        if (!client->Connect())
        {
            std::cerr << "서버 연결 실패!\n";
            return 1;
        }

        std::cout << "AI 시작...\n";
        client->StartAI();

        // AI 루프 실행
        auto start_time = std::chrono::steady_clock::now();
        auto last_update = start_time;
        
        std::cout << "AI 플레이어가 동작 중입니다. 종료하려면 Ctrl+C를 누르세요.\n\n";

        while (client->IsConnected())
        {
            auto current_time = std::chrono::steady_clock::now();
            auto delta_time = std::chrono::duration<float>(current_time - last_update).count();
            last_update = current_time;

            // AI 업데이트
            client->UpdateAI(delta_time);
            
            // 디버깅: 연결 상태 확인
            static int loop_count = 0;
            loop_count++;
            if (loop_count % 100 == 0)
            {
                std::cout << "메인 루프 실행 중... (카운트: " << loop_count << ", 연결상태: " << (client->IsConnected() ? "true" : "false") << ")" << std::endl;
            }

            // 실행 시간 체크
            if (duration > 0)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                if (elapsed >= duration)
                {
                    std::cout << "설정된 실행 시간(" << duration << "초)이 완료되었습니다.\n";
                    break;
                }
            }

            // 짧은 대기 (CPU 사용량 조절)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "AI 중지...\n";
        client->StopAI();
    }
    catch (const std::exception& e)
    {
        std::cerr << "AI 실행 중 오류 발생: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "알 수 없는 오류가 발생했습니다.\n";
        return 1;
    }

    std::cout << "\n=== AI 플레이어 종료 ===\n";
    std::cout << "플레이어: " << config.player_name << "\n";
    std::cout << "정상 종료되었습니다.\n";

    return 0;
}