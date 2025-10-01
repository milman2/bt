#include "ReceiveBuffer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <algorithm>

namespace bt {

ReceiveBuffer::ReceiveBuffer(size_t buffer_size)
    : buffer_(buffer_size)
    , read_pos_(0)
    , write_pos_(0)
    , data_size_(0)
{
}

bool ReceiveBuffer::HasCompletePacket() const
{
    if (data_size_ < sizeof(uint32_t))
    {
        return false; // 패킷 크기를 읽을 수 없음
    }
    
    // 패킷 크기 읽기
    uint32_t packet_size = 0;
    size_t temp_read_pos = read_pos_;
    
    // 패킷 크기 읽기 (링버퍼 경계 고려)
    if (temp_read_pos + sizeof(uint32_t) <= buffer_.size())
    {
        // 경계를 넘지 않는 경우
        std::memcpy(&packet_size, buffer_.data() + temp_read_pos, sizeof(uint32_t));
    }
    else
    {
        // 경계를 넘는 경우
        size_t first_part = buffer_.size() - temp_read_pos;
        size_t second_part = sizeof(uint32_t) - first_part;
        
        uint8_t temp_buffer[sizeof(uint32_t)];
        std::memcpy(temp_buffer, buffer_.data() + temp_read_pos, first_part);
        std::memcpy(temp_buffer + first_part, buffer_.data(), second_part);
        std::memcpy(&packet_size, temp_buffer, sizeof(uint32_t));
    }
    
    // 전체 패킷이 있는지 확인
    return data_size_ >= packet_size;
}

uint32_t ReceiveBuffer::GetNextPacketSize() const
{
    if (data_size_ < sizeof(uint32_t))
    {
        return 0; // 패킷 크기를 읽을 수 없음
    }
    
    uint32_t packet_size = 0;
    size_t temp_read_pos = read_pos_;
    
    // 패킷 크기 읽기 (링버퍼 경계 고려)
    if (temp_read_pos + sizeof(uint32_t) <= buffer_.size())
    {
        // 경계를 넘지 않는 경우
        std::memcpy(&packet_size, buffer_.data() + temp_read_pos, sizeof(uint32_t));
    }
    else
    {
        // 경계를 넘는 경우
        size_t first_part = buffer_.size() - temp_read_pos;
        size_t second_part = sizeof(uint32_t) - first_part;
        
        uint8_t temp_buffer[sizeof(uint32_t)];
        std::memcpy(temp_buffer, buffer_.data() + temp_read_pos, first_part);
        std::memcpy(temp_buffer + first_part, buffer_.data(), second_part);
        std::memcpy(&packet_size, temp_buffer, sizeof(uint32_t));
    }
    
    return packet_size;
}

bool ReceiveBuffer::ExtractNextPacket(uint16_t& packet_type, std::vector<uint8_t>& packet_data)
{
    if (!HasCompletePacket())
    {
        return false;
    }
    
    uint32_t packet_size = GetNextPacketSize();
    if (packet_size < MIN_PACKET_SIZE)
    {
        return false; // 잘못된 패킷 크기
    }
    
    // 패킷 크기 제거
    RemoveFromBuffer(sizeof(uint32_t));
    
    // 패킷 타입 읽기
    uint8_t type_buffer[sizeof(uint16_t)];
    ReadFromBuffer(type_buffer, sizeof(uint16_t));
    packet_type = *reinterpret_cast<uint16_t*>(type_buffer);
    
    // 패킷 데이터 읽기
    size_t data_size = packet_size - sizeof(uint32_t) - sizeof(uint16_t);
    packet_data.resize(data_size);
    if (data_size > 0)
    {
        ReadFromBuffer(packet_data.data(), data_size);
    }
    
    return true;
}

bool ReceiveBuffer::HasSpace() const
{
    return data_size_ < buffer_.size();
}

size_t ReceiveBuffer::GetUsedSize() const
{
    return data_size_;
}

size_t ReceiveBuffer::GetFreeSize() const
{
    return buffer_.size() - data_size_;
}

void ReceiveBuffer::Clear()
{
    read_pos_ = 0;
    write_pos_ = 0;
    data_size_ = 0;
}

void ReceiveBuffer::PrintDebugInfo() const
{
    std::cout << "ReceiveBuffer Debug Info:" << std::endl;
    std::cout << "  Buffer size: " << buffer_.size() << std::endl;
    std::cout << "  Read pos: " << read_pos_ << std::endl;
    std::cout << "  Write pos: " << write_pos_ << std::endl;
    std::cout << "  Data size: " << data_size_ << std::endl;
    std::cout << "  Free size: " << GetFreeSize() << std::endl;
    std::cout << "  Has complete packet: " << (HasCompletePacket() ? "yes" : "no") << std::endl;
    if (HasCompletePacket())
    {
        std::cout << "  Next packet size: " << GetNextPacketSize() << std::endl;
    }
}

size_t ReceiveBuffer::ReadFromBuffer(void* dest, size_t size)
{
    if (size == 0 || data_size_ == 0)
    {
        return 0;
    }
    
    size_t bytes_to_read = std::min(size, data_size_);
    
    if (read_pos_ + bytes_to_read <= buffer_.size())
    {
        // 경계를 넘지 않는 경우
        std::memcpy(dest, buffer_.data() + read_pos_, bytes_to_read);
    }
    else
    {
        // 경계를 넘는 경우
        size_t first_part = buffer_.size() - read_pos_;
        size_t second_part = bytes_to_read - first_part;
        
        std::memcpy(dest, buffer_.data() + read_pos_, first_part);
        std::memcpy(static_cast<uint8_t*>(dest) + first_part, buffer_.data(), second_part);
    }
    
    read_pos_ = (read_pos_ + bytes_to_read) % buffer_.size();
    data_size_ -= bytes_to_read;
    
    return bytes_to_read;
}

size_t ReceiveBuffer::WriteToBuffer(const void* src, size_t size)
{
    if (size == 0 || !HasSpace())
    {
        return 0;
    }
    
    size_t bytes_to_write = std::min(size, GetFreeSize());
    
    if (write_pos_ + bytes_to_write <= buffer_.size())
    {
        // 경계를 넘지 않는 경우
        std::memcpy(buffer_.data() + write_pos_, src, bytes_to_write);
    }
    else
    {
        // 경계를 넘는 경우
        size_t first_part = buffer_.size() - write_pos_;
        size_t second_part = bytes_to_write - first_part;
        
        std::memcpy(buffer_.data() + write_pos_, src, first_part);
        std::memcpy(buffer_.data(), static_cast<const uint8_t*>(src) + first_part, second_part);
    }
    
    write_pos_ = (write_pos_ + bytes_to_write) % buffer_.size();
    data_size_ += bytes_to_write;
    
    return bytes_to_write;
}

void ReceiveBuffer::RemoveFromBuffer(size_t size)
{
    if (size == 0 || data_size_ == 0)
    {
        return;
    }
    
    size_t bytes_to_remove = std::min(size, data_size_);
    read_pos_ = (read_pos_ + bytes_to_remove) % buffer_.size();
    data_size_ -= bytes_to_remove;
}

bool ReceiveBuffer::EnsureSpace(size_t needed)
{
    if (GetFreeSize() >= needed)
    {
        return true; // 이미 충분한 공간이 있음
    }
    
    // 공간이 부족한 경우, 오래된 데이터를 제거
    size_t bytes_to_remove = needed - GetFreeSize();
    if (bytes_to_remove > data_size_)
    {
        bytes_to_remove = data_size_; // 모든 데이터 제거
    }
    
    RemoveFromBuffer(bytes_to_remove);
    
    return GetFreeSize() >= needed;
}


} // namespace bt
