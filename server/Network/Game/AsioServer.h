#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketProtocol.h"
#include "BT/BehaviorTreeEngine.h"

namespace bt
{

    // 전방 선언
    class AsioClient;
    class BehaviorTreeEngine;
    class MonsterBTExecutor;
    class MonsterManager;
    class PlayerManager;
    class RestApiServer;
    class SimpleWebSocketServer;

    // Asio 서버 전용 설정 구조체 (공통 ServerConfig 확장)
    struct AsioServerConfig
    {
        std::string                 host            = "0.0.0.0";
        uint16_t                    port            = 7000;
        size_t                      max_clients     = 1000;
        size_t                      worker_threads  = 4;
        bool                        debug_mode      = false;
        size_t                      max_packet_size = 4096;
        boost::chrono::milliseconds connection_timeout{30000}; // 30초
    };

    // 클라이언트 정보 구조체
    struct AsioClientInfo
    {
        boost::shared_ptr<AsioClient>           client;
        std::string                             ip_address;
        uint16_t                                port;
        boost::chrono::steady_clock::time_point connect_time;
        bool                                    is_authenticated;
        uint32_t                                player_id;
        std::string                             client_type; // "player", "monster", "tester"
    };

    // 서버 헬스 정보 구조체
    struct ServerHealthInfo
    {
        bool     is_healthy;
        size_t   connected_clients;
        size_t   total_packets_sent;
        size_t   total_packets_received;
        size_t   worker_threads;
        size_t   max_clients;
        uint64_t uptime_seconds;
    };

    // Asio 기반 서버 클래스
    class AsioServer
    {
    public:
        AsioServer(const AsioServerConfig& config);
        ~AsioServer();

        // 서버 시작/중지
        bool start();
        void stop();
        bool is_running() const { return running_.load(); }

        // 클라이언트 관리
        void add_client(boost::shared_ptr<AsioClient> client);
        void remove_client(boost::shared_ptr<AsioClient> client);
        void broadcast_packet(const Packet& packet, boost::shared_ptr<AsioClient> exclude_client = nullptr);
        void send_packet(boost::shared_ptr<AsioClient> client, const Packet& packet);

        // Behavior Tree 엔진 접근
        BehaviorTreeEngine* get_bt_engine() { return bt_engine_.get(); }

        // 매니저 접근
        std::shared_ptr<MonsterManager> get_monster_manager() const { return monster_manager_; }
        std::shared_ptr<PlayerManager>  get_player_manager() const { return player_manager_; }

        // 웹 서버 접근
        std::shared_ptr<RestApiServer> get_rest_api_server() const { return rest_api_server_; }

        // WebSocket 서버 접근
        std::shared_ptr<SimpleWebSocketServer> get_websocket_server() const { return websocket_server_; }

        // 설정 접근
        const AsioServerConfig& get_config() const { return config_; }

        // 통계 정보
        size_t get_connected_clients() const;
        size_t get_total_packets_sent() const { return total_packets_sent_.load(); }
        size_t get_total_packets_received() const { return total_packets_received_.load(); }

        // 헬스체크 및 모니터링
        bool             is_healthy() const;
        ServerHealthInfo get_health_info() const;

    private:
        // 서버 초기화
        void start_accept();
        void handle_accept(boost::shared_ptr<AsioClient> client, const boost::system::error_code& error);

        // 워커 스레드
        void worker_thread_function();

        // 패킷 처리

    public:
        void process_packet(boost::shared_ptr<AsioClient> client, const Packet& packet);

    private:
        // 응답 전송 함수들
        void send_connect_response(boost::shared_ptr<AsioClient> client);
        void send_monster_spawn_response(boost::shared_ptr<AsioClient> client, bool success);
        void send_monster_update_response(boost::shared_ptr<AsioClient> client, bool success);
        void send_bt_execute_response(boost::shared_ptr<AsioClient> client, bool success);
        void send_error_response(boost::shared_ptr<AsioClient> client, const std::string& error_message);

        // 로깅
        void log_message(const std::string& message, bool is_error = false);

    private:
        AsioServerConfig  config_;
        std::atomic<bool> running_;

        // Boost.Asio 관련
        boost::asio::io_context        io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::thread_group            worker_threads_;

        // 클라이언트 관리
        std::unordered_map<boost::shared_ptr<AsioClient>, AsioClientInfo> clients_;
        mutable boost::mutex                                              clients_mutex_;

        // Behavior Tree 엔진
        std::unique_ptr<BehaviorTreeEngine> bt_engine_;

        // 몬스터 및 플레이어 매니저
        std::shared_ptr<MonsterManager> monster_manager_;
        std::shared_ptr<PlayerManager>  player_manager_;

        // 웹 서버
        std::shared_ptr<RestApiServer> rest_api_server_;

        // WebSocket 서버
        std::shared_ptr<SimpleWebSocketServer> websocket_server_;

        // 통계
        std::atomic<size_t> total_packets_sent_;
        std::atomic<size_t> total_packets_received_;

        // 서버 시작 시간 (헬스체크용)
        std::chrono::steady_clock::time_point server_start_time_;

        // 로깅
        mutable boost::mutex log_mutex_;
    };

    // Asio 기반 클라이언트 클래스
    class AsioClient : public boost::enable_shared_from_this<AsioClient>
    {
    public:
        AsioClient(boost::asio::io_context& io_context, AsioServer* server);
        ~AsioClient();

        // 연결 관리
        void start();
        void stop();
        bool is_connected() const { return connected_.load(); }

        // 패킷 송수신
        void send_packet(const Packet& packet);
        void receive_packet();

        // 소켓 접근
        boost::asio::ip::tcp::socket&       socket() { return socket_; }
        const boost::asio::ip::tcp::socket& socket() const { return socket_; }

        // 클라이언트 정보
        std::string                             get_ip_address() const;
        uint16_t                                get_port() const;
        boost::chrono::steady_clock::time_point get_connect_time() const { return connect_time_; }

    private:
        // 패킷 처리
        void handle_packet_size(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_packet_data(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_send(const boost::system::error_code& error, size_t bytes_transferred);

        // 연결 관리
        void handle_disconnect();

    private:
        boost::asio::io_context&     io_context_;
        boost::asio::ip::tcp::socket socket_;
        AsioServer*                  server_;

        std::atomic<bool>                       connected_;
        boost::chrono::steady_clock::time_point connect_time_;

        // 패킷 수신 버퍼
        boost::array<uint8_t, 4096> receive_buffer_;
        uint32_t                    expected_packet_size_;
        std::vector<uint8_t>        packet_buffer_;

        // 패킷 전송 큐
        std::queue<Packet> send_queue_;
        boost::mutex       send_queue_mutex_;
        bool               sending_;
    };

} // namespace bt
