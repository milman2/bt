#pragma once

#include <functional>
#include <unordered_map>

#include "Game/PacketProtocol.h"

namespace bt
{

    // 패킷 핸들러 함수 타입
    using PacketHandlerFunc = std::function<void(int socket_fd, const Packet& packet)>;

    // 패킷 핸들러 클래스
    class PacketHandler
    {
    public:
        PacketHandler();
        ~PacketHandler();

        // 패킷 처리
        void HandlePacket(int socket_fd, const Packet& packet);

        // 핸들러 등록
        void RegisterHandler(PacketType type, PacketHandlerFunc handler);

        // 패킷 생성은 PacketUtils 네임스페이스의 함수들을 사용

    private:
        // 기본 핸들러들
        void HandleConnectRequest(int socket_fd, const Packet& packet);
        void HandlePlayerMove(int socket_fd, const Packet& packet);
        void HandleDisconnect(int socket_fd, const Packet& packet);

        // 패킷 파싱은 PacketUtils 네임스페이스의 함수들을 사용

    private:
        std::unordered_map<PacketType, PacketHandlerFunc> handlers_;
    };

} // namespace bt
