#include "packet_handler.h"
#include <iostream>
#include <cstring>

namespace bt {

PacketHandler::PacketHandler() {
    // 기본 핸들러 등록
    register_handler(PacketType::CONNECT_REQUEST, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_connect_request(socket_fd, packet);
                    });
    
    register_handler(PacketType::LOGIN_REQUEST, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_login_request(socket_fd, packet);
                    });
    
    register_handler(PacketType::LOGOUT_REQUEST, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_logout_request(socket_fd, packet);
                    });
    
    register_handler(PacketType::PLAYER_MOVE, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_player_move(socket_fd, packet);
                    });
    
    register_handler(PacketType::PLAYER_CHAT, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_player_chat(socket_fd, packet);
                    });
    
    register_handler(PacketType::DISCONNECT, 
                    [this](int socket_fd, const Packet& packet) {
                        handle_disconnect(socket_fd, packet);
                    });
}

PacketHandler::~PacketHandler() {
}

void PacketHandler::handle_packet(int socket_fd, const Packet& packet) {
    auto it = handlers_.find(static_cast<PacketType>(packet.type));
    if (it != handlers_.end()) {
        it->second(socket_fd, packet);
    } else {
        std::cout << "알 수 없는 패킷 타입: " << packet.type << std::endl;
    }
}

void PacketHandler::register_handler(PacketType type, PacketHandlerFunc handler) {
    handlers_[type] = handler;
}

void PacketHandler::handle_connect_request(int socket_fd, const Packet& packet) {
    std::cout << "연결 요청 수신: 소켓 " << socket_fd << std::endl;
    
    // 연결 응답 전송
    Packet response = create_connect_response(true, "연결 성공");
    // 실제로는 서버 인스턴스에 접근해서 전송해야 함
}

void PacketHandler::handle_login_request(int socket_fd, const Packet& packet) {
    std::cout << "로그인 요청 수신: 소켓 " << socket_fd << std::endl;
    
    // 간단한 로그인 처리 (실제로는 데이터베이스 확인 필요)
    Packet response = create_login_response(true, 1, "로그인 성공");
    // 실제로는 서버 인스턴스에 접근해서 전송해야 함
}

void PacketHandler::handle_logout_request(int socket_fd, const Packet& packet) {
    std::cout << "로그아웃 요청 수신: 소켓 " << socket_fd << std::endl;
}

void PacketHandler::handle_player_move(int socket_fd, const Packet& packet) {
    std::cout << "플레이어 이동 요청 수신: 소켓 " << socket_fd << std::endl;
}

void PacketHandler::handle_player_chat(int socket_fd, const Packet& packet) {
    std::cout << "플레이어 채팅 요청 수신: 소켓 " << socket_fd << std::endl;
}

void PacketHandler::handle_disconnect(int socket_fd, const Packet& packet) {
    std::cout << "연결 해제 요청 수신: 소켓 " << socket_fd << std::endl;
}

// 정적 패킷 생성 함수들
Packet PacketHandler::create_connect_response(bool success, const std::string& message) {
    std::vector<uint8_t> data;
    data.push_back(success ? 1 : 0);
    data.insert(data.end(), message.begin(), message.end());
    return Packet(static_cast<uint16_t>(PacketType::CONNECT_RESPONSE), data);
}

Packet PacketHandler::create_login_response(bool success, uint32_t player_id, const std::string& message) {
    std::vector<uint8_t> data;
    data.push_back(success ? 1 : 0);
    
    // player_id를 바이트로 변환
    for (int i = 0; i < 4; ++i) {
        data.push_back((player_id >> (i * 8)) & 0xFF);
    }
    
    data.insert(data.end(), message.begin(), message.end());
    return Packet(static_cast<uint16_t>(PacketType::LOGIN_RESPONSE), data);
}

Packet PacketHandler::create_error_message(const std::string& error) {
    std::vector<uint8_t> data(error.begin(), error.end());
    return Packet(static_cast<uint16_t>(PacketType::ERROR_MESSAGE), data);
}

Packet PacketHandler::create_chat_message(const std::string& sender, const std::string& message) {
    std::vector<uint8_t> data;
    
    // 송신자 이름 길이와 데이터
    uint16_t sender_len = sender.length();
    data.push_back(sender_len & 0xFF);
    data.push_back((sender_len >> 8) & 0xFF);
    data.insert(data.end(), sender.begin(), sender.end());
    
    // 메시지 데이터
    data.insert(data.end(), message.begin(), message.end());
    
    return Packet(static_cast<uint16_t>(PacketType::CHAT_MESSAGE), data);
}

Packet PacketHandler::create_player_move(uint32_t player_id, float x, float y, float z) {
    std::vector<uint8_t> data;
    
    // player_id
    for (int i = 0; i < 4; ++i) {
        data.push_back((player_id >> (i * 8)) & 0xFF);
    }
    
    // 좌표 (float를 바이트로 변환)
    uint32_t x_bits = *reinterpret_cast<const uint32_t*>(&x);
    uint32_t y_bits = *reinterpret_cast<const uint32_t*>(&y);
    uint32_t z_bits = *reinterpret_cast<const uint32_t*>(&z);
    
    for (int i = 0; i < 4; ++i) {
        data.push_back((x_bits >> (i * 8)) & 0xFF);
        data.push_back((y_bits >> (i * 8)) & 0xFF);
        data.push_back((z_bits >> (i * 8)) & 0xFF);
    }
    
    return Packet(static_cast<uint16_t>(PacketType::PLAYER_MOVE), data);
}

Packet PacketHandler::create_world_update(const std::vector<uint8_t>& world_data) {
    return Packet(static_cast<uint16_t>(PacketType::WORLD_UPDATE), world_data);
}

} // namespace bt
