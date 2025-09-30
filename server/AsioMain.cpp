#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <csignal>
#include <signal.h>

#include "AsioServer.h"
#include "BehaviorTree.h"
#include "../shared/logger.h"
#include "MonsterAI.h"
#include "Player.h"
#include "PlayerManager.h"

namespace bt
{

    // 전역 서버 인스턴스 (시그널 핸들링용)
    std::unique_ptr<AsioServer> g_server = nullptr;

    // 시그널 핸들러
    void signal_handler(int signal)
    {
        std::cout << "\n시그널 " << signal << " 수신됨. 서버를 종료합니다...\n";
        if (g_server)
        {
            g_server->stop();
        }
        
        // 5초 후 강제 종료
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "강제 종료합니다...\n";
            std::exit(0);
        }).detach();
    }

} // namespace bt

int main(int argc, char* argv[])
{
    using namespace bt;

    // 로그 폴더 생성
    std::filesystem::create_directories("logs");

    // 로거 초기화
    auto              now    = std::chrono::system_clock::now();
    auto              time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "logs/server_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".log";
    Logger::getInstance().setLogFile(ss.str());
    Logger::getInstance().setLogLevel(LogLevel::INFO);

    // 시그널 핸들러 등록
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    LOG_INFO("=== BT MMORPG 서버 (Boost.Asio) 시작 ===");

    // 서버 설정
    AsioServerConfig config;
    config.host           = "0.0.0.0";
    config.port           = 7000;
    config.max_clients    = 1000;
    config.worker_threads = 4;
    config.debug_mode     = true;

    // 명령행 인수 처리
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc)
        {
            config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--host" && i + 1 < argc)
        {
            config.host = argv[++i];
        }
        else if (arg == "--max-clients" && i + 1 < argc)
        {
            config.max_clients = std::stoul(argv[++i]);
        }
        else if (arg == "--threads" && i + 1 < argc)
        {
            config.worker_threads = std::stoul(argv[++i]);
        }
        else if (arg == "--debug")
        {
            config.debug_mode = true;
        }
        else if (arg == "--help")
        {
            std::cout << "사용법: " << argv[0] << " [옵션]\n";
            std::cout << "옵션:\n";
            std::cout << "  --port <포트>        서버 포트 (기본값: 7000)\n";
            std::cout << "  --host <호스트>      서버 호스트 (기본값: 0.0.0.0)\n";
            std::cout << "  --max-clients <수>   최대 클라이언트 수 (기본값: 1000)\n";
            std::cout << "  --threads <수>       워커 스레드 수 (기본값: 4)\n";
            std::cout << "  --debug             디버그 모드 활성화\n";
            std::cout << "  --help              이 도움말 표시\n";
            return 0;
        }
    }

    // 서버 생성 및 시작
    g_server = std::make_unique<AsioServer>(config);

    if (!g_server->start())
    {
        LOG_ERROR("서버 시작 실패!");
        return 1;
    }

    LOG_INFO("서버가 포트 " + std::to_string(config.port) + "에서 실행 중입니다.");
    LOG_INFO("종료하려면 Ctrl+C를 누르세요.");

    // Behavior Tree 엔진 초기화 및 몬스터 AI 등록
    auto bt_engine       = g_server->get_bt_engine();
    auto monster_manager = g_server->get_monster_manager();
    auto player_manager  = g_server->get_player_manager();

    if (bt_engine && monster_manager && player_manager)
    {
        // 몬스터별 Behavior Tree 등록
        bt_engine->register_tree("goblin_bt", MonsterBTs::create_goblin_bt());
        bt_engine->register_tree("orc_bt", MonsterBTs::create_orc_bt());
        bt_engine->register_tree("dragon_bt", MonsterBTs::create_dragon_bt());
        bt_engine->register_tree("skeleton_bt", MonsterBTs::create_skeleton_bt());
        bt_engine->register_tree("zombie_bt", MonsterBTs::create_zombie_bt());
        bt_engine->register_tree("merchant_bt", MonsterBTs::create_merchant_bt());
        bt_engine->register_tree("guard_bt", MonsterBTs::create_guard_bt());

        LOG_INFO("Behavior Tree 엔진 초기화 완료");
        LOG_INFO("등록된 BT: " + std::to_string(bt_engine->get_registered_trees()) + "개");

        // MonsterManager에 Behavior Tree 엔진 설정
        // bt_engine은 포인터이므로 shared_ptr로 변환할 수 없음
        // 대신 AsioServer에서 직접 설정하도록 수정 필요

        // 몬스터 스폰 설정 파일 로드
        std::string config_file = "config/monster_spawns.json";
        if (monster_manager->load_spawn_configs_from_file(config_file))
        {
            LOG_INFO("몬스터 스폰 설정 파일 로드 성공: " + config_file);
        }
        else
        {
            LOG_WARNING("몬스터 스폰 설정 파일 로드 실패: " + config_file);
            LOG_INFO("기본 설정으로 폴백합니다.");

            // 폴백: 기본 설정 추가 (플레이어 근처에 스폰)
            MonsterSpawnConfig goblin_spawn;
            goblin_spawn.type         = MonsterType::GOBLIN;
            goblin_spawn.name         = "goblin_default";
            goblin_spawn.position     = {5.0f, 0.0f, 5.0f, 0.0f}; // 플레이어 매우 근처
            goblin_spawn.respawn_time = 30.0f;
            goblin_spawn.max_count    = 3;
            goblin_spawn.spawn_radius = 3.0f;
            monster_manager->add_spawn_config(goblin_spawn);

            MonsterSpawnConfig orc_spawn;
            orc_spawn.type         = MonsterType::ORC;
            orc_spawn.name         = "orc_default";
            orc_spawn.position     = {10.0f, 0.0f, 10.0f, 0.0f}; // 플레이어 매우 근처
            orc_spawn.respawn_time = 60.0f;
            orc_spawn.max_count    = 2;
            orc_spawn.spawn_radius = 5.0f;
            monster_manager->add_spawn_config(orc_spawn);
        }

        // 자동 스폰 시작
        monster_manager->start_auto_spawn();

        // 실제 클라이언트 연결 시 플레이어가 생성됩니다
    }

    // 서버가 실행 중인 동안 대기
    while (g_server->is_running())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 매니저들 업데이트
        if (monster_manager)
        {
            static int update_count = 0;
            update_count++;
            if (update_count % 100 == 0)
            { // 10초마다 로그 출력
                LOG_DEBUG("asio_main.cpp에서 MonsterManager::update 호출 (카운트: " + std::to_string(update_count) +
                          ")");
            }
            monster_manager->update(0.1f); // 100ms 간격으로 업데이트
        }

        if (player_manager)
        {
            player_manager->update(0.1f); // 100ms 간격으로 업데이트
        }

        // Behavior Tree 엔진 업데이트
        if (bt_engine)
        {
            bt_engine->update_all_monsters(0.1f); // 100ms 간격으로 업데이트
        }

        // 통계 정보 출력 (10초마다)
        static auto last_stats_time = std::chrono::steady_clock::now();
        auto        now             = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count() >= 10)
        {
            LOG_INFO("연결된 클라이언트: " + std::to_string(g_server->get_connected_clients()) +
                     ", 전송된 패킷: " + std::to_string(g_server->get_total_packets_sent()) +
                     ", 수신된 패킷: " + std::to_string(g_server->get_total_packets_received()));
            if (monster_manager)
            {
                LOG_INFO("활성 몬스터: " + std::to_string(monster_manager->get_monster_count()) + "마리");
            }
            if (player_manager)
            {
                LOG_INFO("활성 플레이어: " + std::to_string(player_manager->get_player_count()) + "명");
            }
            last_stats_time = now;
        }
    }

    LOG_INFO("서버가 정상적으로 종료되었습니다.");
    return 0;
}
