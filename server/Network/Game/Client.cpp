#include "Client.h"
#include "Server.h"

namespace bt
{

    // Client 구현
    Client::Client(boost::asio::io_context& io_context, Server* server)
        : io_context_(io_context)
        , socket_(io_context)
        , server_(server)
        , connected_(false)
        , expected_packet_size_(0)
        , sending_(false)
    {
        connect_time_ = boost::chrono::steady_clock::now();
    }

    Client::~Client()
    {
        Stop();
    }

    void Client::Start()
    {
        connected_.store(true);
        ReceivePacket();
    }

    void Client::Stop()
    {
        if (connected_.load())
        {
            connected_.store(false);

            boost::system::error_code ec;
            socket_.close(ec);

            if (server_)
            {
                server_->RemoveClient(shared_from_this());
            }
        }
    }

    void Client::SendPacket(const Packet& packet)
    {
        if (!connected_.load())
        {
            return;
        }

        // 전송 큐에 패킷 추가
        {
            boost::lock_guard<boost::mutex> lock(send_queue_mutex_);
            send_queue_.push(packet);
        }

        // 현재 전송 중이 아니면 전송 시작
        if (!sending_.load())
        {
            StartSending();
        }
    }

    void Client::ReceivePacket()
    {
        if (!connected_.load())
        {
            return;
        }

        // 패킷 크기 수신
        boost::asio::async_read(socket_,
                                boost::asio::buffer(&expected_packet_size_, sizeof(expected_packet_size_)),
                                [this](const boost::system::error_code& error, size_t bytes_transferred)
                                {
                                    HandlePacketSize(error, bytes_transferred);
                                });
    }

    void Client::HandlePacketSize(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error && bytes_transferred == sizeof(expected_packet_size_))
        {
            // 패킷 데이터 수신
            packet_buffer_.resize(expected_packet_size_ - sizeof(uint32_t));

            boost::asio::async_read(socket_,
                                    boost::asio::buffer(packet_buffer_),
                                    [this](const boost::system::error_code& error, size_t bytes_transferred)
                                    {
                                        HandlePacketData(error, bytes_transferred);
                                    });
        }
        else
        {
            HandleDisconnect();
        }
    }

    void Client::HandlePacketData(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error && bytes_transferred == packet_buffer_.size())
        {
            // 패킷 파싱
            Packet packet;
            packet.size = expected_packet_size_;
            packet.type = *reinterpret_cast<uint16_t*>(packet_buffer_.data());
            packet.data.assign(packet_buffer_.begin() + sizeof(uint16_t), packet_buffer_.end());

            std::cout << "Client: 패킷 수신 완료 - 타입=" << packet.type << ", 크기=" << packet.size
                      << ", 데이터크기=" << packet.data.size() << std::endl;

            // 서버에 패킷 전달
            if (server_)
            {
                server_->ProcessPacket(shared_from_this(), packet);
            }

            // 다음 패킷 수신
            ReceivePacket();
        }
        else
        {
            std::cout << "Client: 패킷 수신 오류 - error=" << error.message()
                      << ", bytes_transferred=" << bytes_transferred << ", expected=" << packet_buffer_.size()
                      << std::endl;
            HandleDisconnect();
        }
    }

    void Client::StartSending()
    {
        if (!connected_.load())
        {
            return;
        }

        boost::lock_guard<boost::mutex> lock(send_queue_mutex_);
        if (send_queue_.empty())
        {
            sending_ = false;
            return;
        }

        sending_ = true;
        const Packet& packet = send_queue_.front();

        try
        {
            // 패킷 크기 + 타입 + 데이터 전송
            uint32_t             total_size = sizeof(uint32_t) + sizeof(uint16_t) + packet.data.size();
            std::vector<uint8_t> send_buffer(total_size);

            // 패킷 크기
            *reinterpret_cast<uint32_t*>(send_buffer.data()) = total_size;
            // 패킷 타입
            *reinterpret_cast<uint16_t*>(send_buffer.data() + sizeof(uint32_t)) = packet.type;
            // 패킷 데이터
            std::copy(
                packet.data.begin(), packet.data.end(), send_buffer.begin() + sizeof(uint32_t) + sizeof(uint16_t));

            // 비동기 전송
            boost::asio::async_write(socket_,
                                    boost::asio::buffer(send_buffer),
                                    [this](const boost::system::error_code& error, size_t bytes_transferred)
                                    {
                                        HandleSend(error, bytes_transferred);
                                    });
        }
        catch (const std::exception& e)
        {
            // 전송 실패 시 연결 종료
            HandleDisconnect();
        }
    }

    void Client::HandleSend(const boost::system::error_code& error, size_t /* bytes_transferred */)
    {
        if (!error)
        {
            boost::lock_guard<boost::mutex> lock(send_queue_mutex_);
            send_queue_.pop();

            if (!send_queue_.empty())
            {
                // 다음 패킷 전송
                StartSending();
            }
            else
            {
                sending_ = false;
            }
        }
        else
        {
            HandleDisconnect();
        }
    }

    void Client::HandleDisconnect()
    {
        if (connected_.load())
        {
            connected_.store(false);

            if (server_)
            {
                server_->RemoveClient(shared_from_this());
            }
        }
    }

    std::string Client::GetIPAddress() const
    {
        try
        {
            return socket_.remote_endpoint().address().to_string();
        }
        catch (...)
        {
            return "unknown";
        }
    }

    uint16_t Client::GetPort() const
    {
        try
        {
            return socket_.remote_endpoint().port();
        }
        catch (...)
        {
            return 0;
        }
    }

} // namespace bt
