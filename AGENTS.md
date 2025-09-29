# BT MMORPG 서버 - Behavior Tree AI 시스템

## 개요

이 프로젝트는 MMORPG 서버에서 몬스터 AI를 Behavior Tree로 구현하고 테스트하는 시스템입니다. Behavior Tree는 3가지 독립적인 방식으로 구현되어 있으며, 각각 다른 사용자 그룹을 대상으로 합니다.

## 아키텍처

### 1. 네트워킹 계층 (Boost.Asio)
- **플랫폼 독립적**: Windows, Linux, macOS 지원
- **비동기 I/O**: 높은 성능과 확장성
- **멀티스레드**: 워커 스레드 풀 기반

### 2. Behavior Tree 구현 방식

#### 방식 1: C++ 네이티브 구현
- **대상 사용자**: 서버 개발자
- **특징**: 
  - 모든 BT 노드가 C++로 구현
  - 최고 성능
  - 컴파일 타임 최적화
  - 복잡한 로직 구현 가능
- **파일 위치**: `src/bt_native/`
- **사용법**: C++ 코드에서 직접 BT 구성

#### 방식 2: C++ 엔진 + 기획자 인터페이스
- **대상 사용자**: 몬스터 기획자
- **특징**:
  - BT 엔진은 C++로 구현
  - 기획자가 사용할 수 있는 고수준 인터페이스 제공
  - JSON/XML 설정 파일로 BT 구성
  - 시각적 BT 에디터 지원 (향후)
- **파일 위치**: `src/bt_engine/`
- **사용법**: 설정 파일로 BT 구성

#### 방식 3: Lua 스크립트 구현
- **대상 사용자**: 몬스터 기획자
- **특징**:
  - 모든 BT 로직을 Lua로 구현
  - 런타임에 동적 수정 가능
  - 빠른 프로토타이핑
  - 기획자가 직접 스크립트 작성
- **파일 위치**: `src/bt_lua/`, `scripts/bt/`
- **사용법**: Lua 스크립트로 BT 구현

## Behavior Tree 노드 타입

### 기본 노드
- **Action**: 실행 노드 (공격, 이동, 대화 등)
- **Condition**: 조건 확인 노드 (체력 확인, 거리 확인 등)
- **Composite**: 자식 노드들을 관리하는 노드
  - **Sequence**: 순차 실행
  - **Selector**: 하나라도 성공하면 성공
  - **Parallel**: 병렬 실행
  - **Random**: 랜덤 선택

### 고급 노드
- **Decorator**: 자식 노드의 동작을 수정
  - **Repeat**: 반복 실행
  - **Invert**: 결과 반전
  - **Delay**: 지연 실행
  - **Timeout**: 시간 제한
- **Blackboard**: 데이터 공유
- **Memory**: 상태 기억

## 몬스터 AI 시나리오

### 기본 몬스터 (Goblin)
```cpp
// C++ 구현 예시
std::shared_ptr<BehaviorTree> create_goblin_bt() {
    auto tree = std::make_shared<BehaviorTree>("goblin_bt");
    auto root = std::make_shared<BTSelector>("goblin_root");
    
    // 적 발견 및 공격 시퀀스
    auto attack_sequence = std::make_shared<BTSequence>("attack_sequence");
    attack_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
    attack_sequence->add_child(std::make_shared<BTCondition>("in_attack_range", MonsterConditions::is_enemy_in_attack_range));
    attack_sequence->add_child(std::make_shared<BTAction>("attack", MonsterActions::attack));
    
    // 적 추적 시퀀스
    auto chase_sequence = std::make_shared<BTSequence>("chase_sequence");
    chase_sequence->add_child(std::make_shared<BTCondition>("can_see_enemy", MonsterConditions::can_see_enemy));
    chase_sequence->add_child(std::make_shared<BTCondition>("path_clear", MonsterConditions::is_path_clear));
    chase_sequence->add_child(std::make_shared<BTAction>("move_to_target", MonsterActions::move_to_target));
    
    // 적 탐지 시퀀스
    auto detection_sequence = std::make_shared<BTSequence>("detection_sequence");
    detection_sequence->add_child(std::make_shared<BTCondition>("enemy_in_range", MonsterConditions::is_enemy_in_detection_range));
    detection_sequence->add_child(std::make_shared<BTAction>("set_target", MonsterActions::idle));
    
    // 순찰
    auto patrol_action = std::make_shared<BTAction>("patrol", MonsterActions::patrol);
    
    root->add_child(attack_sequence);
    root->add_child(chase_sequence);
    root->add_child(detection_sequence);
    root->add_child(patrol_action);
    
    tree->set_root(root);
    return tree;
}
```

### 보스 몬스터 (Dragon)
- **1단계**: 원거리 공격
- **2단계**: 근접 공격 + 특수 스킬
- **3단계**: 광역 공격 + 소환

### NPC (상인)
- **대화**: 플레이어와 상호작용
- **상점**: 아이템 거래
- **퀘스트**: 퀘스트 제공

## 테스트 시나리오

### 1. 단위 테스트
- 각 BT 노드의 개별 테스트
- 노드 간 상호작용 테스트
- 에러 처리 테스트

### 2. 통합 테스트
- 몬스터 AI 전체 동작 테스트
- 다중 몬스터 동시 테스트
- 플레이어와의 상호작용 테스트

### 3. 성능 테스트
- 대량 몬스터 AI 처리 성능
- 메모리 사용량 측정
- CPU 사용률 측정

### 4. 스트레스 테스트
- 1000+ 몬스터 동시 AI 처리
- 장시간 실행 안정성
- 메모리 누수 검사

## 구현 계획

### Phase 1: 네트워킹 계층
- [x] Boost.Asio 기반 서버 구현
- [x] 플랫폼 독립적 클라이언트 구현
- [x] 기본 통신 프로토콜

### Phase 2: Behavior Tree 엔진
- [ ] C++ 네이티브 BT 구현
- [ ] C++ 엔진 + 기획자 인터페이스
- [ ] Lua 스크립트 BT 구현

### Phase 3: 몬스터 AI 구현
- [ ] 기본 몬스터 AI (Goblin)
- [ ] 보스 몬스터 AI (Dragon)
- [ ] NPC AI (상인)

### Phase 4: 테스트 시스템
- [ ] 단위 테스트
- [ ] 통합 테스트
- [ ] 성능 테스트
- [ ] 스트레스 테스트

### Phase 5: 도구 및 편의 기능
- [ ] BT 시각화 도구
- [ ] BT 에디터
- [ ] 디버깅 도구
- [ ] 성능 모니터링

## 사용법

### 개발자용 (C++ 네이티브)
```cpp
// 몬스터 AI 구현
auto goblin_bt = create_goblin_bt();

// 몬스터 스폰 설정
MonsterSpawnConfig goblin_spawn;
goblin_spawn.type = MonsterType::GOBLIN;
goblin_spawn.name = "goblin_1";
goblin_spawn.position = {100.0f, 0.0f, 100.0f, 0.0f};
goblin_spawn.respawn_time = 30.0f;
goblin_spawn.max_count = 3;
goblin_spawn.spawn_radius = 10.0f;

// 자동 스폰 시작
monster_manager->add_spawn_config(goblin_spawn);
monster_manager->start_auto_spawn();
```

### 기획자용 (설정 파일)
```json
{
    "name": "goblin_ai",
    "root": {
        "type": "selector",
        "children": [
            {
                "type": "sequence",
                "children": [
                    {"type": "condition", "name": "has_target"},
                    {"type": "condition", "name": "in_attack_range"},
                    {"type": "action", "name": "attack"}
                ]
            },
            {"type": "action", "name": "patrol"}
        ]
    }
}
```

### 기획자용 (Lua 스크립트)
```lua
-- goblin_ai.lua
local bt = require("behavior_tree")

function create_goblin_ai()
    return bt.selector({
        -- 적 발견 및 공격
        bt.sequence({
            bt.condition("can_see_enemy"),
            bt.condition("in_attack_range"),
            bt.action("attack")
        }),
        -- 적 추적
        bt.sequence({
            bt.condition("can_see_enemy"),
            bt.condition("path_clear"),
            bt.action("move_to_target")
        }),
        -- 적 탐지
        bt.sequence({
            bt.condition("enemy_in_range"),
            bt.action("set_target")
        }),
        -- 순찰
        bt.action("patrol")
    })
end
```

## 새로운 기능

### 몬스터 자동 생성 시스템
- **서버 시작 시 자동 스폰**: 설정된 위치에 몬스터 자동 생성
- **리스폰 시스템**: 몬스터 사망 후 설정된 시간 후 자동 리스폰
- **스폰 설정**: 위치, 반경, 최대 수, 리스폰 시간 등 세밀한 제어

### 플레이어 시스템
- **플레이어 생성**: 이름과 위치로 플레이어 생성
- **전투 시스템**: 플레이어와 몬스터 간 실시간 전투
- **리스폰 시스템**: 플레이어 사망 시 자동 리스폰
- **HP/MP 관리**: 체력과 마나 시스템

### 환경 인지 시스템
- **주변 탐지**: 플레이어와 몬스터의 주변 환경 인지
- **거리 계산**: 공격 범위, 탐지 범위 등 거리 기반 판단
- **시야 확보**: 장애물 고려한 시야 계산
- **경로 탐색**: 이동 가능한 경로 확인

## 성능 목표

- **몬스터당 AI 처리 시간**: < 1ms
- **동시 처리 몬스터 수**: 1000+
- **메모리 사용량**: 몬스터당 < 1KB
- **CPU 사용률**: < 10% (1000 몬스터 기준)
- **플레이어당 처리 시간**: < 0.5ms
- **전투 처리 지연**: < 50ms

## 확장성

- **새로운 노드 타입**: 플러그인 방식으로 추가
- **새로운 몬스터**: 설정 파일 또는 스크립트로 추가
- **새로운 AI 패턴**: 기존 노드 조합으로 구현
- **멀티서버**: 분산 AI 처리 지원

## 디버깅 및 모니터링

- **BT 실행 추적**: 각 노드의 실행 상태 로깅
- **성능 프로파일링**: AI 처리 시간 측정
- **메모리 모니터링**: 메모리 사용량 추적
- **실시간 디버깅**: 런타임 BT 수정

## 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다.
