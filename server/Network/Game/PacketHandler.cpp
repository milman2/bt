#include <iostream>

#include <cstring>

#include "PacketHandler.h"

namespace bt
{

    PacketHandler::PacketHandler()
    {
        // 기본 핸들러 등록
        RegisterHandler(PacketType::CONNECT_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             HandleConnectRequest(socket_fd, packet);
                         });

        RegisterHandler(PacketType::LOGIN_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             HandleLoginRequest(socket_fd, packet);
                         });

        RegisterHandler(PacketType::LOGOUT_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             HandleLogoutRequest(socket_fd, packet);
                         });

        RegisterHandler(PacketType::PLAYER_MOVE,
                         [this](int socket_fd, const Packet& packet)
                         {
                             HandlePlayerMove(socket_fd, packet);
                         });

        RegisterHandler(PacketType::PLAYER_CHAT,
                         [this](int socket_fd, const Packet& packet)
                         {
                             HandlePlayerChat(socket_fd, packet);
                         });

        RegisterHandler(PacketType::DISCONNECT,
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
        Packet response = PacketUtils::create_connect_response(true, "연결 성공");
        // 실제로는 서버 인스턴스에 접근해서 전송해야 함
    }

    void PacketHandler::HandleLoginRequest(int socket_fd, const Packet& packet)
    {
        std::cout << "로그인 요청 수신: 소켓 " << socket_fd << std::endl;

        // 간단한 로그인 처리 (실제로는 데이터베이스 확인 필요)
        Packet response = PacketUtils::create_login_response(true, 1, "로그인 성공");
        // 실제로는 서버 인스턴스에 접근해서 전송해야 함
    }

    void PacketHandler::HandleLogoutRequest(int socket_fd, const Packet& packet)
    {
        std::cout << "로그아웃 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::HandlePlayerMove(int socket_fd, const Packet& packet)
    {
        std::cout << "플레이어 이동 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::HandlePlayerChat(int socket_fd, const Packet& packet)
    {
        std::cout << "플레이어 채팅 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::HandleDisconnect(int socket_fd, const Packet& packet)
    {
        std::cout << "연결 해제 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    // 패킷 생성 함수들은 PacketUtils 네임스페이스의 함수들을 사용

} // namespace bt
