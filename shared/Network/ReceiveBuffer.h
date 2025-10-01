#pragma once

#include <memory>

#include <cstdint>
#include <cstring>

#include "ReceiveBufferPool.h"

namespace bt
{

    /**
     * @brief 링크드 리스트 기반 패킷 수신 버퍼
     *
     * 이 클래스는 네트워크에서 수신된 데이터를 안전하게 저장하고
     * 완전한 패킷을 추출할 수 있도록 도와줍니다.
     *
     * 특징:
     * - 링크드 리스트 기반 동적 크기 버퍼
     * - 메모리 풀을 통한 효율적인 노드 관리
     * - 패킷 경계를 고려한 데이터 관리
     * - 부분 패킷 처리 지원
     */
    class ReceiveBuffer
    {
    public:
        /**
         * @brief 생성자
         */
        ReceiveBuffer();

        /**
         * @brief 소멸자
         */
        ~ReceiveBuffer();

        // 복사 생성자와 대입 연산자 금지
        ReceiveBuffer(const ReceiveBuffer&)            = delete;
        ReceiveBuffer& operator=(const ReceiveBuffer&) = delete;

        // 이동 생성자와 대입 연산자 허용
        ReceiveBuffer(ReceiveBuffer&&)            = default;
        ReceiveBuffer& operator=(ReceiveBuffer&&) = default;

        /**
         * @brief 외부에서 읽은 데이터를 버퍼에 추가
         * @param data 읽은 데이터
         * @param size 데이터 크기
         * @return 실제로 추가된 바이트 수
         */
        size_t AppendData(const void* data, size_t size);

        /**
         * @brief 지정된 크기의 데이터가 있는지 확인
         * @param required_size 필요한 데이터 크기
         * @return true면 충분한 데이터가 있음
         */
        bool HasEnoughData(size_t required_size) const;

        /**
         * @brief 지정된 크기의 데이터를 추출
         * @param dest 대상 버퍼
         * @param size 추출할 크기
         * @return 실제로 추출된 크기
         */
        size_t ExtractData(void* dest, size_t size);

        /**
         * @brief 데이터를 읽기만 하고 버퍼에서 제거하지 않음 (peek)
         * @param dest 대상 버퍼
         * @param size 읽을 크기
         * @return 실제로 읽은 크기
         */
        size_t PeekData(void* dest, size_t size) const;

        /**
         * @brief 버퍼에 사용 가능한 공간이 있는지 확인
         * @return true면 공간이 있음
         */
        bool HasSpace() const;

        /**
         * @brief 현재 사용 중인 데이터 크기
         * @return 사용 중인 바이트 수
         */
        size_t GetUsedSize() const;

        /**
         * @brief 사용 가능한 공간 크기
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

    private:
        std::shared_ptr<BufferNode> head_;            ///< 첫 번째 노드
        std::shared_ptr<BufferNode> tail_;            ///< 마지막 노드
        size_t                      total_data_size_; ///< 전체 데이터 크기
        size_t                      read_offset_;     ///< 현재 읽기 위치 (첫 번째 노드 기준)

        /**
         * @brief 새 노드 추가
         */
        void _AddNewNode();

        /**
         * @brief 빈 노드들 정리
         */
        void _CleanupEmptyNodes();

        /**
         * @brief 노드에서 데이터 읽기
         * @param node 읽을 노드
         * @param offset 노드 내 오프셋
         * @param dest 대상 버퍼
         * @param size 읽을 크기
         * @return 실제로 읽은 크기
         */
        size_t _ReadFromNode(std::shared_ptr<BufferNode> node, size_t offset, void* dest, size_t size) const;
    };

} // namespace bt

/*
사용 예시:

1. 소켓에서 데이터 읽기:
   boost::asio::ip::tcp::socket socket(io_context);
   ReceiveBuffer buffer;

   uint8_t temp_buffer[4096];
   boost::system::error_code ec;
   size_t bytes_read = socket.read_some(boost::asio::buffer(temp_buffer), ec);

   if (bytes_read > 0) {
       buffer.AppendData(temp_buffer, bytes_read);
   }

2. 패킷 크기 확인 및 데이터 추출:
   // 패킷 크기 peek (4바이트)
   if (buffer.HasEnoughData(4)) {
       uint32_t packet_size;
       buffer.PeekData(&packet_size, 4);

       // 전체 패킷 데이터 확인
       if (buffer.HasEnoughData(packet_size)) {
           // 패킷 크기 추출
           buffer.ExtractData(&packet_size, 4);

           // 패킷 데이터 추출
           std::vector<uint8_t> packet_data(packet_size - 4);
           buffer.ExtractData(packet_data.data(), packet_size - 4);
           // 패킷 처리
       }
   }
*/