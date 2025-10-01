#include <iostream>

#include <cmath>

#include "../../../BT/Context.h"
#include "../../TestClient.h"
#include "Patrol.h"

namespace bt
{
    namespace action
    {

        NodeStatus Patrol::Execute(Context& context)
        {
            // AI 클라이언트 가져오기
            auto ai = context.GetAI();
            if (!ai)
            {
                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

            auto client_executor = std::dynamic_pointer_cast<TestClient>(ai);
            if (!client_executor)
            {
                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

            // 몬스터가 탐지되면 공격 모드로 전환
            if (client_executor->HasTarget())
            {
                float distance        = client_executor->GetDistanceToTarget();
                float detection_range = client_executor->GetDetectionRange();

                if (distance <= detection_range)
                {
                    std::cout << "플레이어 " << client_executor->GetName() << " 순찰 중 몬스터 탐지: 거리 " << distance
                              << " <= " << detection_range << " - 공격 모드로 전환" << std::endl;
                    SetLastStatus(NodeStatus::FAILURE);
                    return NodeStatus::FAILURE; // 공격 시퀀스로 전환
                }
            }

            // 순찰점이 있는지 확인
            if (!client_executor->HasPatrolPoints())
            {
                SetLastStatus(NodeStatus::FAILURE);
                return NodeStatus::FAILURE;
            }

            // 다음 순찰점 가져오기
            auto target_point = client_executor->GetNextPatrolPoint();
            auto current_pos  = client_executor->GetPosition();

            // 목표 지점까지의 거리 계산
            float dx       = target_point.x - current_pos.x;
            float dz       = target_point.z - current_pos.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            // 도착 거리 임계값
            const float arrival_threshold = 5.0f;

            if (distance <= arrival_threshold)
            {
                std::cout << "플레이어 " << client_executor->GetName() << " 순찰점 도착: (" << target_point.x << ", "
                          << target_point.y << ", " << target_point.z << ")" << std::endl;

                // 다음 순찰점으로 이동
                client_executor->AdvanceToNextPatrolPoint();

                // 실행 상태 업데이트
                SetRunning(false);
                SetLastStatus(NodeStatus::SUCCESS);
                context.ClearCurrentRunningNode();

                return NodeStatus::SUCCESS;
            }
            else
            {
                // 목표 지점으로 이동
                float move_speed = client_executor->GetMoveSpeed();
                float delta_time = 0.1f; // 고정 델타타임

                // 정규화된 방향 벡터 계산
                float normalized_dx = dx / distance;
                float normalized_dz = dz / distance;

                // 새로운 위치 계산
                float step_size = move_speed * delta_time;
                float new_x     = current_pos.x + normalized_dx * step_size;
                float new_z     = current_pos.z + normalized_dz * step_size;

                client_executor->MoveTo(new_x, current_pos.y, new_z);

                // 실행 상태 업데이트
                SetRunning(true);
                SetLastStatus(NodeStatus::RUNNING);
                context.SetCurrentRunningNode(GetName());

                if (context.GetExecutionCount() % 50 == 0) // 5초마다 로그
                {
                    std::cout << "플레이어 " << client_executor->GetName() << " 순찰 이동 중: (" << new_x << ", "
                              << current_pos.y << ", " << new_z << ") - 거리: " << distance << std::endl;
                }

                return NodeStatus::RUNNING;
            }
        }

    } // namespace action
} // namespace bt
