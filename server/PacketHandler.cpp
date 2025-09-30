#include <iostream>

#include <cstring>

#include "PacketHandler.h"

namespace bt
{

    PacketHandler::PacketHandler()
    {
        // 기본 핸들러 등록
        register_handler(PacketType::CONNECT_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_connect_request(socket_fd, packet);
                         });

        register_handler(PacketType::LOGIN_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_login_request(socket_fd, packet);
                         });

        register_handler(PacketType::LOGOUT_REQUEST,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_logout_request(socket_fd, packet);
                         });

        register_handler(PacketType::PLAYER_MOVE,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_player_move(socket_fd, packet);
                         });

        register_handler(PacketType::PLAYER_CHAT,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_player_chat(socket_fd, packet);
                         });

        register_handler(PacketType::DISCONNECT,
                         [this](int socket_fd, const Packet& packet)
                         {
                             handle_disconnect(socket_fd, packet);
                         });
    }

    PacketHandler::~PacketHandler() {}

    void PacketHandler::handle_packet(int socket_fd, const Packet& packet)
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

    void PacketHandler::register_handler(PacketType type, PacketHandlerFunc handler)
    {
        handlers_[type] = handler;
    }

    void PacketHandler::handle_connect_request(int socket_fd, const Packet& packet)
    {
        std::cout << "연결 요청 수신: 소켓 " << socket_fd << std::endl;

        // 연결 응답 전송
        Packet response = PacketUtils::create_connect_response(true, "연결 성공");
        // 실제로는 서버 인스턴스에 접근해서 전송해야 함
    }

    void PacketHandler::handle_login_request(int socket_fd, const Packet& packet)
    {
        std::cout << "로그인 요청 수신: 소켓 " << socket_fd << std::endl;

        // 간단한 로그인 처리 (실제로는 데이터베이스 확인 필요)
        Packet response = PacketUtils::create_login_response(true, 1, "로그인 성공");
        // 실제로는 서버 인스턴스에 접근해서 전송해야 함
    }

    void PacketHandler::handle_logout_request(int socket_fd, const Packet& packet)
    {
        std::cout << "로그아웃 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::handle_player_move(int socket_fd, const Packet& packet)
    {
        std::cout << "플레이어 이동 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::handle_player_chat(int socket_fd, const Packet& packet)
    {
        std::cout << "플레이어 채팅 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    void PacketHandler::handle_disconnect(int socket_fd, const Packet& packet)
    {
        std::cout << "연결 해제 요청 수신: 소켓 " << socket_fd << std::endl;
    }

    // 패킷 생성 함수들은 PacketUtils 네임스페이스의 함수들을 사용

} // namespace bt
