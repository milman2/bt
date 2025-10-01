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

#include "Game/PacketProtocol.h"

namespace bt
{

    // 전방 선언
    class Server;

    // Asio 기반 클라이언트 클래스
    class Client : public boost::enable_shared_from_this<Client>
    {
    public:
        Client(boost::asio::io_context& io_context, Server* server);
        ~Client();

        // 연결 관리
        void Start();
        void Stop();
        bool IsConnected() const { return connected_.load(); }

        // 패킷 송수신
        void SendPacket(const Packet& packet);
        void ReceivePacket();

        // 소켓 접근
        boost::asio::ip::tcp::socket&       Socket() { return socket_; }
        const boost::asio::ip::tcp::socket& Socket() const { return socket_; }

        // 클라이언트 정보
        std::string                             GetIPAddress() const;
        uint16_t                                GetPort() const;
        boost::chrono::steady_clock::time_point GetConnectTime() const { return connect_time_; }

    private:
        // 패킷 처리
        void HandlePacketSize(const boost::system::error_code& error, size_t bytes_transferred);
        void HandlePacketData(const boost::system::error_code& error, size_t bytes_transferred);
        void StartSending();
        void HandleSend(const boost::system::error_code& error, size_t bytes_transferred);

        // 연결 관리
        void HandleDisconnect();

    private:
        boost::asio::io_context&     io_context_;
        boost::asio::ip::tcp::socket socket_;
        Server*                      server_;

        std::atomic<bool>                       connected_;
        boost::chrono::steady_clock::time_point connect_time_;

        // 패킷 수신 버퍼
        boost::array<uint8_t, 4096> receive_buffer_;
        uint32_t                    expected_packet_size_;
        std::vector<uint8_t>        packet_buffer_;

        // 패킷 전송 큐
        std::queue<Packet> send_queue_;
        boost::mutex       send_queue_mutex_;
        std::atomic<bool>  sending_;
    };

} // namespace bt
