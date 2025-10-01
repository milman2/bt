#include <iostream>

#include <cstring>

#include "PacketHandler.h"

namespace bt
{

    PacketHandler::PacketHandler()
    {
        // 기본 핸들러 등록
        RegisterHandler(PacketType::CONNECT_REQ,
                        [this](int socket_fd, const Packet& packet)
                        {
                            HandleConnectRequest(socket_fd, packet);
                        });

        RegisterHandler(PacketType::PLAYER_MOVE_REQ,
                        [this](int socket_fd, const Packet& packet)
                        {
                            HandlePlayerMove(socket_fd, packet);
                        });

        RegisterHandler(PacketType::DISCONNECT_EVT,
                        [this](int socket_fd, const Packet& packet)
                        {
                            HandleDisconnect(socket_fd, packet);
                        });
    }

    PacketHandler::~PacketHandler() {}

    void PacketHandler::HandlePacket(int socket_fd, const Packet& packet)
    {
        auto it = handlers_.find(static_cast<PacketType>(packet.type));
        if (it != handlers_.end())
        {
            it->second(socket_fd, packet);
        }
        else
        {
            std::cout << "알 수 없는 패킷 타입: " << packet.type << std::endl;
        }
    }

    void PacketHandler::RegisterHandler(PacketType type, PacketHandlerFunc handler)
    {
        handlers_[type] = handler;
    }

    void PacketHandler::HandleConnectRequest(int socket_fd, const Packet& packet)
    {
        std::cout << "연결 요청 수신: 소켓 " << socket_fd << std::endl;

        // 연결 응답 전송
        Packet response = PacketUtils::CreateConnectRes(true, "연결 성공");
        // 실제로는 서버 인스턴스에 접근해서 전송해야 함
    }

    void PacketHandler::HandlePlayerMove(int socket_fd, const Packet& packet)
    {
        std::cout << "플레이어 이동 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::HandleDisconnect(int socket_fd, const Packet& packet)
    {
        std::cout << "연결 해제 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    // 패킷 생성 함수들은 PacketUtils 네임스페이스의 함수들을 사용

} // namespace bt
