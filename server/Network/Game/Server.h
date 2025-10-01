#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Game/PacketProtocol.h"

namespace bt
{

    // 전방 선언
    class Client;
    class GameWorld;
    class PacketHandler;
    class MessageBasedMonsterManager;

    // 클라이언트 정보 구조체
    struct ClientInfo
    {
        int                                   socket_fd;
        std::string                           ip_address;
        uint16_t                              port;
        std::chrono::steady_clock::time_point connect_time;
        bool                                  is_authenticated;
        uint32_t                              player_id;
    };

    // MMORPG 서버 클래스
    class Server
    {
    public:
        Server(const ServerConfig& config);
        ~Server();

        // 서버 시작/중지
        bool start();
        void stop();
        bool IsRunning() const { return running_.load(); }

        // 클라이언트 관리
        void AddClient(int socket_fd, const std::string& ip, uint16_t port);
        void RemoveClient(int socket_fd);
        void BroadcastPacket(const Packet& packet, int exclude_socket = -1);
        void SendPacket(int socket_fd, const Packet& packet);
        
        // 월드 상태 브로드캐스팅
        void BroadcastWorldState();
        void StartBroadcastLoop();
        void StopBroadcastLoop();

        // 게임 월드 접근
        GameWorld* GetGameWorld() { return game_world_.get(); }
        
        // 몬스터 매니저 설정
        void SetMonsterManager(std::shared_ptr<MessageBasedMonsterManager> manager);

        // 설정 접근
        const ServerConfig& get_config() const { return config_; }

    private:
        // 서버 소켓 관련
        bool CreateSocket();
        bool BindSocket();
        bool ListenSocket();
        void AcceptConnections();

        // 클라이언트 처리
        void HandleClient(int socket_fd);
        void ProcessPacket(int socket_fd, const Packet& packet);

        // 워커 스레드
        void WorkerThreadFunction();
        
        // 브로드캐스팅 스레드
        void BroadcastLoopThread();

        // 유틸리티 함수
        bool SetSocketNonBlocking(int socket_fd);
        void LogMessage(const std::string& message, bool is_error = false);

    private:
        ServerConfig      config_;
        std::atomic<bool> running_;

        // 소켓 관련
        int                server_socket_;
        struct sockaddr_in server_addr_;

        // 클라이언트 관리
        std::unordered_map<int, std::unique_ptr<ClientInfo>> clients_;
        std::mutex                                           clients_mutex_;

        // 워커 스레드
        std::vector<std::thread>          worker_threads_;
        std::queue<std::function<void()>> task_queue_;
        std::mutex                        task_queue_mutex_;
        std::condition_variable           task_queue_cv_;
        
        // 브로드캐스팅 스레드
        std::thread                        broadcast_thread_;
        std::atomic<bool>                  broadcast_running_;
        std::chrono::steady_clock::time_point last_broadcast_time_;

        // 게임 컴포넌트
        std::unique_ptr<GameWorld>     game_world_;
        std::unique_ptr<PacketHandler> packet_handler_;
        std::shared_ptr<MessageBasedMonsterManager> monster_manager_;

        // 로깅
        std::mutex log_mutex_;
    };

} // namespace bt
