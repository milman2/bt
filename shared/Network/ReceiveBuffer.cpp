#include <algorithm>
#include <iostream>

#include "ReceiveBuffer.h"

namespace bt
{

    ReceiveBuffer::ReceiveBuffer() : head_(nullptr), tail_(nullptr), total_data_size_(0), read_offset_(0) {}

    ReceiveBuffer::~ReceiveBuffer()
    {
        Clear();
    }

    size_t ReceiveBuffer::AppendData(const void* data, size_t size)
    {
        if (size == 0 || !data)
        {
            return 0;
        }

        const uint8_t* src_bytes      = static_cast<const uint8_t*>(data);
        size_t         total_appended = 0;

        while (total_appended < size)
        {
            // 새 노드가 필요하면 추가
            if (!tail_ || tail_->GetFreeSize() == 0)
            {
                _AddNewNode();
            }

            // 현재 노드에 데이터 추가
            size_t remaining = size - total_appended;
            size_t appended  = tail_->AppendData(src_bytes + total_appended, remaining);
            total_appended += appended;
            total_data_size_ += appended;
        }

        return total_appended;
    }

    bool ReceiveBuffer::HasEnoughData(size_t required_size) const
    {
        return total_data_size_ >= required_size;
    }

    size_t ReceiveBuffer::ExtractData(void* dest, size_t size)
    {
        if (size == 0 || total_data_size_ < size || !dest)
        {
            return 0;
        }

        uint8_t* dest_bytes      = static_cast<uint8_t*>(dest);
        size_t   total_extracted = 0;
        size_t   current_offset  = read_offset_;
        auto     current_node    = head_;

        while (total_extracted < size && current_node)
        {
            // 현재 노드에서 읽을 수 있는 데이터 크기
            size_t available_in_node = current_node->used_size - current_offset;
            size_t to_read           = std::min(size - total_extracted, available_in_node);

            if (to_read > 0)
            {
                // 노드에서 데이터 읽기
                std::memcpy(dest_bytes + total_extracted, current_node->data.data() + current_offset, to_read);

                total_extracted += to_read;
                current_offset += to_read;

                // 노드의 모든 데이터를 읽었으면 다음 노드로
                if (current_offset >= current_node->used_size)
                {
                    current_node   = current_node->next;
                    current_offset = 0;
                }
            }
            else
            {
                break;
            }
        }

        // 읽은 데이터만큼 버퍼에서 제거
        if (total_extracted > 0)
        {
            read_offset_ += total_extracted;
            total_data_size_ -= total_extracted;

            // 빈 노드들 정리
            _CleanupEmptyNodes();
        }

        return total_extracted;
    }

    size_t ReceiveBuffer::PeekData(void* dest, size_t size) const
    {
        if (size == 0 || total_data_size_ < size || !dest)
        {
            return 0;
        }

        uint8_t* dest_bytes     = static_cast<uint8_t*>(dest);
        size_t   total_read     = 0;
        size_t   current_offset = read_offset_;
        auto     current_node   = head_;

        while (total_read < size && current_node)
        {
            // 현재 노드에서 읽을 수 있는 데이터 크기
            size_t available_in_node = current_node->used_size - current_offset;
            size_t to_read           = std::min(size - total_read, available_in_node);

            if (to_read > 0)
            {
                // 노드에서 데이터 읽기 (제거하지 않음)
                std::memcpy(dest_bytes + total_read, current_node->data.data() + current_offset, to_read);

                total_read += to_read;
                current_offset += to_read;

                // 노드의 모든 데이터를 읽었으면 다음 노드로
                if (current_offset >= current_node->used_size)
                {
                    current_node   = current_node->next;
                    current_offset = 0;
                }
            }
            else
            {
                break;
            }
        }

        return total_read;
    }

    bool ReceiveBuffer::HasSpace() const
    {
        // 링크드 리스트는 이론적으로 무제한 공간이 있음
        // 하지만 메모리 풀의 제한을 고려
        return true;
    }

    size_t ReceiveBuffer::GetUsedSize() const
    {
        return total_data_size_;
    }

    size_t ReceiveBuffer::GetFreeSize() const
    {
        // 링크드 리스트는 동적으로 확장되므로 무제한
        return SIZE_MAX;
    }

    void ReceiveBuffer::Clear()
    {
        // 모든 노드를 풀에 반환
        auto current = head_;
        while (current)
        {
            auto next = current->next;
            ReceiveBufferPool::Instance().Free(current);
            current = next;
        }

        head_            = nullptr;
        tail_            = nullptr;
        total_data_size_ = 0;
        read_offset_     = 0;
    }

    void ReceiveBuffer::PrintDebugInfo() const
    {
        std::cout << "ReceiveBuffer Debug Info:" << std::endl;
        std::cout << "  Total data size: " << total_data_size_ << " bytes" << std::endl;
        std::cout << "  Read offset: " << read_offset_ << std::endl;

        size_t node_count = 0;
        auto   current    = head_;
        while (current)
        {
            node_count++;
            std::cout << "  Node " << node_count << ": used=" << current->used_size
                      << ", free=" << current->GetFreeSize() << std::endl;
            current = current->next;
        }

        std::cout << "  Total nodes: " << node_count << std::endl;
        std::cout << "  Has enough data for 4 bytes: " << (HasEnoughData(4) ? "yes" : "no") << std::endl;
    }

    void ReceiveBuffer::_AddNewNode()
    {
        auto new_node = ReceiveBufferPool::Instance().Alloc();

        if (!head_)
        {
            // 첫 번째 노드
            head_        = new_node;
            tail_        = new_node;
            read_offset_ = 0;
        }
        else
        {
            // 기존 리스트에 추가
            tail_->next = new_node;
            tail_       = new_node;
        }
    }

    void ReceiveBuffer::_CleanupEmptyNodes()
    {
        // 읽기 위치가 첫 번째 노드를 넘어서면 빈 노드들 제거
        while (head_ && read_offset_ >= head_->used_size)
        {
            read_offset_ -= head_->used_size;

            auto old_head = head_;
            head_         = head_->next;

            // 마지막 노드였으면 tail도 업데이트
            if (!head_)
            {
                tail_ = nullptr;
            }

            // 노드를 풀에 반환
            ReceiveBufferPool::Instance().Free(old_head);
        }
    }

    size_t ReceiveBuffer::_ReadFromNode(std::shared_ptr<BufferNode> node, size_t offset, void* dest, size_t size) const
    {
        if (!node || offset >= node->used_size || size == 0)
        {
            return 0;
        }

        size_t available = node->used_size - offset;
        size_t to_read   = std::min(size, available);

        if (to_read > 0)
        {
            std::memcpy(dest, node->data.data() + offset, to_read);
        }

        return to_read;
    }

} // namespace bt