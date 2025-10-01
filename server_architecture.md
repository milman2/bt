# BT MMORPG 서버 아키텍처

## 전체 시스템 아키텍처

```mermaid
graph TB
    subgraph "클라이언트 계층"
        Client[AsioTestClient<br/>AI 플레이어]
    end
    
    subgraph "네트워크 계층"
        AsioServer[AsioServer<br/>포트 7000<br/>TCP 소켓]
        WebSocketServer[BeastHttpWebSocketServer<br/>포트 8080<br/>HTTP/WebSocket]
    end
    
    subgraph "메시지 처리 계층"
        MessageProcessor[GameMessageProcessor<br/>메시지 큐 관리]
        NetworkHandler[NetworkMessageHandler<br/>네트워크 메시지 처리]
    end
    
    subgraph "게임 로직 계층"
        PlayerManager[MessageBasedPlayerManager<br/>플레이어 관리]
        MonsterManager[MessageBasedMonsterManager<br/>몬스터 관리]
    end
    
    subgraph "AI 계층"
        BTEngine[Engine<br/>Behavior Tree 엔진]
        BTExecutor[MonsterBTExecutor<br/>AI 실행자]
        Monster[Monster<br/>몬스터 엔티티]
    end
    
    subgraph "데이터 계층"
        Config[설정 파일<br/>monster_spawns.json]
        Logs[로그 시스템<br/>server_*.log]
    end
    
    Client -->|TCP 연결| AsioServer
    Client -->|HTTP/WebSocket| WebSocketServer
    
    AsioServer --> MessageProcessor
    WebSocketServer --> NetworkHandler
    
    MessageProcessor --> PlayerManager
    MessageProcessor --> MonsterManager
    
    MonsterManager --> BTEngine
    BTEngine --> BTExecutor
    BTExecutor --> Monster
    
    PlayerManager --> WebSocketServer
    MonsterManager --> Config
    AsioServer --> Logs
```

## 네트워크 통신 흐름

```mermaid
sequenceDiagram
    participant C as AsioTestClient
    participant AS as AsioServer
    participant AC as AsioClient
    participant PM as PlayerManager
    participant MM as MonsterManager
    participant BT as BTEngine
    
    C->>AS: TCP 연결 요청
    AS->>AC: 클라이언트 연결 수락
    AC->>AS: 연결 완료
    
    C->>AS: PLAYER_JOIN 패킷
    AS->>PM: 플레이어 생성 요청
    PM->>AS: 플레이어 생성 완료
    AS->>C: PLAYER_JOIN_RESPONSE
    
    loop 게임 루프
        C->>AS: 플레이어 이동/공격
        AS->>PM: 플레이어 상태 업데이트
        PM->>AS: 상태 업데이트 완료
        
        MM->>BT: 몬스터 AI 업데이트
        BT->>MM: AI 실행 결과
        MM->>AS: 몬스터 상태 업데이트
        AS->>C: 몬스터 상태 전송
    end
```

## Behavior Tree AI 시스템

```mermaid
graph TD
    subgraph "BT 엔진"
        Engine[Engine<br/>BT 등록/관리]
    end
    
    subgraph "몬스터 BT들"
        GoblinBT[Goblin BT<br/>기본 몬스터]
        OrcBT[Orc BT<br/>전사 몬스터]
        DragonBT[Dragon BT<br/>보스 몬스터]
        SkeletonBT[Skeleton BT<br/>언데드]
        ZombieBT[Zombie BT<br/>언데드]
        MerchantBT[Merchant BT<br/>상인 NPC]
        GuardBT[Guard BT<br/>경비 NPC]
    end
    
    subgraph "몬스터 인스턴스"
        Monster1[Monster 1<br/>dragon_1]
        Monster2[Monster 2<br/>goblin_1]
        Monster3[Monster 3<br/>merchant_1]
    end
    
    Engine --> GoblinBT
    Engine --> OrcBT
    Engine --> DragonBT
    Engine --> SkeletonBT
    Engine --> ZombieBT
    Engine --> MerchantBT
    Engine --> GuardBT
    
    DragonBT --> Monster1
    GoblinBT --> Monster2
    MerchantBT --> Monster3
```

## 서버 컴포넌트 상세 구조

```mermaid
graph LR
    subgraph "AsioServer"
        AS[AsioServer<br/>메인 서버]
        AC[AsioClient<br/>클라이언트 관리]
        PP[PacketProtocol<br/>패킷 정의]
    end
    
    subgraph "BeastHttpWebSocketServer"
        BHS[BeastHttpWebSocketServer<br/>HTTP/WebSocket 서버]
        WS[WebSocket Sessions<br/>실시간 연결]
        API[HTTP API<br/>REST 엔드포인트]
    end
    
    subgraph "MessageBasedMonsterManager"
        MMM[MessageBasedMonsterManager<br/>몬스터 매니저]
        Spawn[Auto Spawn System<br/>자동 스폰]
        Config[Config Loader<br/>설정 로더]
    end
    
    subgraph "MessageBasedPlayerManager"
        MPM[MessageBasedPlayerManager<br/>플레이어 매니저]
        Player[Player Entities<br/>플레이어 엔티티]
    end
    
    AS --> AC
    AS --> PP
    BHS --> WS
    BHS --> API
    MMM --> Spawn
    MMM --> Config
    MPM --> Player
```

## 데이터 흐름 다이어그램

```mermaid
flowchart TD
    Start([서버 시작]) --> Init[컴포넌트 초기화]
    Init --> BTInit[BT 엔진 초기화]
    BTInit --> ConfigLoad[설정 파일 로드]
    ConfigLoad --> SpawnStart[자동 스폰 시작]
    SpawnStart --> Listen[클라이언트 연결 대기]
    
    Listen --> ClientConnect{클라이언트 연결?}
    ClientConnect -->|Yes| Accept[연결 수락]
    ClientConnect -->|No| Listen
    
    Accept --> PlayerJoin[플레이어 참여]
    PlayerJoin --> GameLoop[게임 루프 시작]
    
    GameLoop --> UpdateMonsters[몬스터 AI 업데이트]
    UpdateMonsters --> UpdatePlayers[플레이어 상태 업데이트]
    UpdatePlayers --> SendUpdates[상태 전송]
    SendUpdates --> GameLoop
    
    GameLoop -->|종료| Stop([서버 종료])
```

## 스레드 구조

```mermaid
graph TB
    subgraph "메인 스레드"
        Main[메인 스레드<br/>서버 초기화 및 관리]
    end
    
    subgraph "워커 스레드 (4개)"
        Worker1[워커 스레드 1<br/>Boost.Asio I/O]
        Worker2[워커 스레드 2<br/>Boost.Asio I/O]
        Worker3[워커 스레드 3<br/>Boost.Asio I/O]
        Worker4[워커 스레드 4<br/>Boost.Asio I/O]
    end
    
    subgraph "메시지 처리 스레드"
        MessageThread[메시지 처리 스레드<br/>GameMessageProcessor]
    end
    
    subgraph "몬스터 AI 스레드"
        AIThread[몬스터 AI 스레드<br/>Behavior Tree 실행]
    end
    
    Main --> Worker1
    Main --> Worker2
    Main --> Worker3
    Main --> Worker4
    Main --> MessageThread
    Main --> AIThread
```

## 패킷 처리 흐름

```mermaid
sequenceDiagram
    participant C as Client
    participant AS as AsioServer
    participant AC as AsioClient
    participant PP as ProcessPacket
    participant HJ as HandlePlayerJoin
    participant PM as PlayerManager
    
    C->>AS: 패킷 전송
    AS->>AC: 패킷 수신
    AC->>AS: 패킷 파싱 완료
    AS->>PP: ProcessPacket 호출
    
    alt PLAYER_JOIN 패킷
        PP->>HJ: HandlePlayerJoin 호출
        HJ->>PM: 플레이어 생성 요청
        PM->>HJ: 플레이어 생성 완료
        HJ->>AS: 응답 패킷 생성
        AS->>C: PLAYER_JOIN_RESPONSE 전송
    else 다른 패킷
        PP->>AS: 해당 핸들러 호출
        AS->>C: 응답 전송
    end
```

## 설정 파일 구조

```mermaid
graph TD
    subgraph "monster_spawns.json"
        Config[설정 파일]
        Spawn1[goblin_1<br/>위치: 100,100<br/>auto_spawn: false]
        Spawn2[goblin_2<br/>위치: 200,200<br/>auto_spawn: false]
        Spawn3[dragon_1<br/>위치: 500,500<br/>auto_spawn: true]
        Spawn4[merchant_1<br/>위치: 0,0<br/>auto_spawn: true]
    end
    
    Config --> Spawn1
    Config --> Spawn2
    Config --> Spawn3
    Config --> Spawn4
    
    Spawn1 --> Monster1[Goblin 몬스터]
    Spawn2 --> Monster2[Goblin 몬스터]
    Spawn3 --> Monster3[Dragon 몬스터]
    Spawn4 --> Monster4[Merchant NPC]
```

이 다이어그램들을 통해 BT MMORPG 서버의 전체 아키텍처를 시각적으로 이해할 수 있습니다. 각 컴포넌트 간의 관계와 데이터 흐름을 명확하게 보여줍니다.
