#pragma once

#include <memory>

#include <cstdint>
#include <cstring>

#include "SendBufferPool.h"

namespace bt
{

    /**
     * @brief 링크드 리스트 기반 패킷 전송 버퍼
     *
     * 이 클래스는 네트워크로 전송할 데이터를 안전하게 저장하고
     * 순차적으로 전송할 수 있도록 도와줍니다.
     *
     * 특징:
     * - 링크드 리스트 기반 동적 크기 버퍼
     * - 메모리 풀을 통한 효율적인 노드 관리
     * - 순차적 데이터 전송 지원
     * - 대용량 데이터 처리 지원
     */
    class SendBuffer
    {
    public:
        /**
         * @brief 생성자
         */
        SendBuffer();

        /**
         * @brief 소멸자
         */
        ~SendBuffer();

        // 복사 생성자와 대입 연산자 금지
        SendBuffer(const SendBuffer&)            = delete;
        SendBuffer& operator=(const SendBuffer&) = delete;

        // 이동 생성자와 대입 연산자 허용
        SendBuffer(SendBuffer&&)            = default;
        SendBuffer& operator=(SendBuffer&&) = default;

        /**
         * @brief 전송할 데이터를 버퍼에 추가
         * @param data 추가할 데이터
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
         * @brief 지정된 크기의 데이터를 추출 (전송용)
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
        std::shared_ptr<SendBufferNode> head_;            ///< 첫 번째 노드
        std::shared_ptr<SendBufferNode> tail_;            ///< 마지막 노드
        size_t                          total_data_size_; ///< 전체 데이터 크기
        size_t                          read_offset_;     ///< 현재 읽기 위치 (첫 번째 노드 기준)

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
        size_t _ReadFromNode(std::shared_ptr<SendBufferNode> node, size_t offset, void* dest, size_t size) const;
    };

} // namespace bt

/*
사용 예시:

1. 전송할 데이터 추가:
   SendBuffer buffer;
   
   std::string message = "Hello World";
   buffer.AppendData(message.c_str(), message.length());

2. 데이터 전송:
   uint8_t temp_buffer[4096];
   while (buffer.HasEnoughData(1)) {
       size_t bytes_to_send = std::min(buffer.GetUsedSize(), sizeof(temp_buffer));
       size_t bytes_extracted = buffer.ExtractData(temp_buffer, bytes_to_send);
       
       // 소켓으로 전송
       boost::asio::write(socket, boost::asio::buffer(temp_buffer, bytes_extracted));
   }
*/
