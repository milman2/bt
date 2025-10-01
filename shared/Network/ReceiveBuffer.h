#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <boost/asio.hpp>

namespace bt {

/**
 * @brief 링버퍼를 사용한 패킷 수신 버퍼 클래스
 * 
 * 고정된 크기의 내부 버퍼를 사용하여 패킷 데이터를 링버퍼 형식으로 관리합니다.
 * TCP 스트림에서 분할된 패킷을 안전하게 처리할 수 있습니다.
 */
class ReceiveBuffer
{
public:
    /**
     * @brief 생성자
     * @param buffer_size 내부 버퍼 크기 (기본값: 64KB)
     */
    explicit ReceiveBuffer(size_t buffer_size = 65536);
    
    /**
     * @brief 소멸자
     */
    ~ReceiveBuffer() = default;
    
    // 복사 생성자와 대입 연산자 비활성화
    ReceiveBuffer(const ReceiveBuffer&) = delete;
    ReceiveBuffer& operator=(const ReceiveBuffer&) = delete;
    
    // 이동 생성자와 대입 연산자 허용
    ReceiveBuffer(ReceiveBuffer&&) = default;
    ReceiveBuffer& operator=(ReceiveBuffer&&) = default;
    
    /**
     * @brief Boost.Asio 호환 소켓에서 데이터를 읽어서 버퍼에 추가
     * @param socket Boost.Asio 호환 소켓 객체 (read_some 메서드가 있어야 함)
     * @return 읽은 바이트 수 (0이면 데이터 없음, -1이면 오류)
     */
    template<typename SocketType>
    int ReadFromSocket(SocketType& socket);
    
    /**
     * @brief 완전한 패킷이 있는지 확인
     * @return true면 완전한 패킷이 있음
     */
    bool HasCompletePacket() const;
    
    /**
     * @brief 다음 패킷의 크기를 반환
     * @return 패킷 크기 (0이면 패킷 크기를 읽을 수 없음)
     */
    uint32_t GetNextPacketSize() const;
    
    /**
     * @brief 다음 패킷을 추출
     * @param packet_type 패킷 타입을 저장할 변수
     * @param packet_data 패킷 데이터를 저장할 벡터
     * @return true면 패킷 추출 성공
     */
    bool ExtractNextPacket(uint16_t& packet_type, std::vector<uint8_t>& packet_data);
    
    /**
     * @brief 버퍼에 사용 가능한 공간이 있는지 확인
     * @return true면 공간이 있음
     */
    bool HasSpace() const;
    
    /**
     * @brief 버퍼 상태 정보 반환
     * @return 사용 중인 바이트 수
     */
    size_t GetUsedSize() const;
    
    /**
     * @brief 버퍼 상태 정보 반환
     * @return 사용 가능한 바이트 수
     */
    size_t GetFreeSize() const;
    
    /**
     * @brief 버퍼 초기화
     */
    void Clear();
    
    /**
     * @brief 디버그 정보 출력
     */
    void PrintDebugInfo() const;
    
    /**
     * @brief 버퍼에 데이터 쓰기 (외부에서 사용)
     * @param src 소스 데이터
     * @param size 쓸 크기
     * @return 실제로 쓴 크기
     */
    size_t WriteToBuffer(const void* src, size_t size);

private:
    /**
     * @brief 버퍼에서 데이터를 읽기
     * @param dest 대상 버퍼
     * @param size 읽을 크기
     * @return 실제로 읽은 크기
     */
    size_t ReadFromBuffer(void* dest, size_t size);
    
    /**
     * @brief 버퍼에서 데이터 제거
     * @param size 제거할 크기
     */
    void RemoveFromBuffer(size_t size);
    
    /**
     * @brief 버퍼 공간 확보
     * @param needed 필요한 공간 크기
     * @return true면 공간 확보 성공
     */
    bool EnsureSpace(size_t needed);

private:
    std::vector<uint8_t> buffer_;    ///< 내부 버퍼
    size_t read_pos_;                ///< 읽기 위치
    size_t write_pos_;               ///< 쓰기 위치
    size_t data_size_;               ///< 현재 데이터 크기
    static constexpr size_t MIN_PACKET_SIZE = sizeof(uint32_t) + sizeof(uint16_t); ///< 최소 패킷 크기
};

// 템플릿 구현
template<typename SocketType>
int ReceiveBuffer::ReadFromSocket(SocketType& socket)
{
    if (!HasSpace())
    {
        return 0; // 공간이 없음
    }
    
    // 사용 가능한 공간 계산
    size_t available_space = GetFreeSize();
    if (available_space == 0)
    {
        return 0;
    }
    
    // 소켓에서 읽기
    boost::system::error_code ec;
    size_t bytes_read = socket.read_some(
        boost::asio::buffer(buffer_.data() + write_pos_, available_space), ec);
    
    if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again)
    {
        return 0; // 데이터 없음
    }
    
    if (ec)
    {
        return -1; // 오류
    }
    
    if (bytes_read > 0)
    {
        write_pos_ = (write_pos_ + bytes_read) % buffer_.size();
        data_size_ += bytes_read;
    }
    
    return static_cast<int>(bytes_read);
}

} // namespace bt

/*
사용 예시:

Boost.Asio 호환 소켓 사용:
   boost::asio::ip::tcp::socket socket(io_context);
   ReceiveBuffer buffer(65536);
   int bytes_read = buffer.ReadFromSocket(socket);

요구사항:
- SocketType은 read_some(buffer, error_code) 메서드를 가져야 함
- boost::asio::buffer()와 호환되어야 함
- boost::system::error_code를 사용해야 함
*/
