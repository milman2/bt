# BT MMORPG 서버 - Behavior Tree AI 시스템

C++로 개발된 MMORPG 게임 서버입니다. 몬스터 AI를 Behavior Tree로 구현하고 테스트하는 시스템을 포함합니다.

## 주요 기능

- **멀티플레이어 온라인 게임 서버**
- **몬스터 자동 생성 및 리스폰 시스템**
- **플레이어-몬스터 전투 시스템**
- **환경 인지 기반 몬스터 AI**
- **Behavior Tree 기반 몬스터 AI**
- TCP 소켓 기반 네트워킹 (Boost.Asio)
- 멀티스레드 아키텍처
- 패킷 기반 통신 시스템
- 플레이어 리스폰 시스템

## 요구사항

- C++17 이상
- CMake 3.16 이상
- Boost 라이브러리 (system, thread 컴포넌트)
- Linux 환경 (Ubuntu 20.04+ 권장)
- GCC 9.0 이상 또는 Clang 10.0 이상

### 의존성 설치

#### vcpkg 사용 (권장)
```bash
# vcpkg 설치 (최초 1회)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install

# Boost 라이브러리 설치
./vcpkg install boost-system boost-thread

# 환경 변수 설정
export VCPKG_ROOT=/path/to/vcpkg
```

#### Conan 사용
```bash
# Conan 설치
pip install conan

# 의존성 설치
conan install . --build=missing --output-folder=build

# 빌드
make build-conan
```

#### 시스템 패키지 매니저 사용
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake libboost-all-dev

# CentOS/RHEL
sudo yum install gcc-c++ cmake boost-devel

# macOS (Homebrew)
brew install cmake boost
```

## 빌드

### 자동 빌드 (권장)

#### vcpkg 사용
```bash
# vcpkg 설정 후
make build-vcpkg

# 또는 자동 감지
make build
```

#### Conan 사용
```bash
# Conan 설치 후
make build-conan

# 또는 수동으로
make conan-install
make build
```

#### 시스템 패키지 사용
```bash
# 시스템 패키지 설치 후
make build-system
```

#### 일반 빌드
```bash
# 릴리즈 빌드
make release

# 디버그 빌드
make debug

# 빌드 후 실행
make run
```

### 수동 빌드

```bash
# 빌드 스크립트 사용
./scripts/build.sh Release

# 또는 CMake 직접 사용
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## 실행

### 기본 실행

```bash
make run
```

### 옵션과 함께 실행

```bash
./build/bin/BT_MMORPG_Server --port 8080 --max-clients 1000 --debug
```

### 사용 가능한 옵션

- `--port <포트>`: 서버 포트 (기본값: 8080)
- `--host <호스트>`: 서버 호스트 (기본값: 0.0.0.0)
- `--max-clients <수>`: 최대 클라이언트 수 (기본값: 1000)
- `--threads <수>`: 워커 스레드 수 (기본값: 4)
- `--debug`: 디버그 모드 활성화
- `--help`: 도움말 표시

## 테스트

### 자동화된 테스트

```bash
# 전체 테스트 실행
make test

# Boost.Asio 테스트 실행
make test-asio

# 특정 테스트만 실행
make test-connection    # 연결 테스트
make test-login        # 로그인 테스트
make test-move         # 플레이어 이동 테스트
make test-chat         # 채팅 테스트
make test-stress       # 스트레스 테스트
```

### 테스트 클라이언트 직접 사용

```bash
# 기존 테스트 클라이언트 도움말
./build/bin/TestClient --help

# Boost.Asio 테스트 클라이언트 도움말
./build/bin/AsioTestClient --help

# 전체 테스트 실행
./build/bin/TestClient --test all

# 특정 테스트 실행
./build/bin/TestClient --test connection
./build/bin/TestClient --test login --username myuser --password mypass
./build/bin/TestClient --test stress --stress-connections 50

# 상세 로그와 함께 테스트
./build/bin/TestClient --test all --verbose
```

### 테스트 스크립트 사용

```bash
# 기존 테스트 스크립트
./scripts/test.sh

# Boost.Asio 테스트 스크립트
./scripts/test_asio.sh

# 특정 테스트 실행
./scripts/test.sh --test connection
./scripts/test.sh --test stress --stress-connections 100

# 상세 로그와 함께 테스트
./scripts/test.sh --verbose
```

### 벤치마크 테스트

```bash
# 성능 벤치마크 실행
./scripts/benchmark.sh
```

### 테스트 기능

- **연결 테스트**: 서버 연결 및 연결 해제 테스트
- **플레이어 접속 테스트**: 플레이어 게임 접속 테스트
- **플레이어 이동 테스트**: 플레이어 위치 업데이트 테스트
- **플레이어 공격 테스트**: 플레이어-몬스터 전투 테스트
- **몬스터 AI 테스트**: Behavior Tree 실행 테스트
- **몬스터 업데이트 테스트**: 몬스터 상태 업데이트 테스트
- **스트레스 테스트**: 다중 연결 및 부하 테스트
- **벤치마크 테스트**: 성능 측정 및 보고서 생성

### Behavior Tree AI 시스템

이 프로젝트는 몬스터 AI를 Behavior Tree로 구현하고 테스트하는 시스템입니다.

#### 몬스터 AI 특징

- **자동 생성**: 서버 시작 시 설정된 위치에 몬스터 자동 스폰
- **리스폰 시스템**: 몬스터가 죽으면 설정된 시간 후 자동 리스폰
- **환경 인지**: 주변 플레이어 탐지, 거리 계산, 시야 확보 여부 확인
- **전투 시스템**: 플레이어와 몬스터 간 실시간 전투
- **플레이어 리스폰**: 플레이어 사망 시 자동 리스폰

#### 3가지 구현 방식

1. **C++ 네이티브 구현**: 서버 개발자용
   - 모든 BT 노드가 C++로 구현
   - 최고 성능과 컴파일 타임 최적화

2. **C++ 엔진 + 기획자 인터페이스**: 몬스터 기획자용
   - BT 엔진은 C++로 구현
   - JSON/XML 설정 파일로 BT 구성

3. **Lua 스크립트 구현**: 몬스터 기획자용
   - 모든 BT 로직을 Lua로 구현
   - 런타임에 동적 수정 가능

#### 몬스터 AI 동작 예시

```
고블린 AI:
1. 적 탐지 (시야 범위 내 플레이어 확인)
2. 적 추적 (플레이어에게 이동)
3. 공격 (공격 범위 내에서 공격 실행)
4. 순찰 (적이 없을 때 기본 행동)
```

자세한 내용은 [AGENTS.md](AGENTS.md)를 참조하세요.

## 프로젝트 구조

```
bt/
├── src/                    # 소스 코드
│   ├── main.cpp           # 메인 함수
│   └── server.cpp         # 서버 구현
├── include/               # 헤더 파일
│   ├── server.h          # 서버 클래스
│   ├── asio_server.h     # Boost.Asio 서버 클래스
│   ├── game_world.h      # 게임 월드
│   ├── packet_handler.h  # 패킷 처리
│   ├── player.h          # 플레이어 클래스
│   ├── behavior_tree.h   # Behavior Tree 엔진
│   ├── monster_ai.h      # 몬스터 AI
│   ├── test_client.h     # 테스트 클라이언트
│   └── asio_test_client.h # Boost.Asio 테스트 클라이언트
├── config/               # 설정 파일
│   └── server.conf       # 서버 설정
├── scripts/              # 빌드/실행/테스트 스크립트
│   ├── build.sh          # 빌드 스크립트
│   ├── run.sh            # 실행 스크립트
│   ├── test.sh           # 테스트 스크립트
│   ├── test_asio.sh      # Boost.Asio 테스트 스크립트
│   ├── benchmark.sh      # 벤치마크 스크립트
│   └── install_dependencies.sh # 의존성 설치 스크립트
├── tests/                # 테스트 코드
├── build/                # 빌드 출력 (자동 생성)
├── CMakeLists.txt        # CMake 설정
├── Makefile             # Make 설정
├── AGENTS.md            # Behavior Tree AI 시스템 문서
└── README.md            # 이 파일
```

## 개발

### 코드 스타일

- C++17 표준 사용
- 네임스페이스 `bt` 사용
- 헤더 가드 사용 (`#pragma once`)
- RAII 원칙 준수

### 디버깅

```bash
# 디버그 모드로 빌드
make debug

# GDB로 디버깅
gdb ./build/bin/BT_MMORPG_Server

# Valgrind로 메모리 체크
valgrind --leak-check=full ./build/bin/BT_MMORPG_Server

# 테스트 실행
make test

# 특정 테스트 실행
make test-connection
```

## 설정

서버 설정은 `config/server.conf` 파일에서 수정할 수 있습니다.

주요 설정 항목:
- 서버 포트 및 호스트
- 최대 클라이언트 수
- 워커 스레드 수
- 로깅 설정
- 게임 설정

## 네트워킹

서버는 TCP 소켓을 사용하여 클라이언트와 통신합니다.

### 패킷 구조

```
[4바이트 크기][2바이트 타입][데이터]
```

### 주요 패킷 타입

- **연결 관련**: CONNECT_REQUEST, CONNECT_RESPONSE
- **플레이어 관련**: PLAYER_JOIN, PLAYER_MOVE, PLAYER_ATTACK
- **몬스터 관련**: MONSTER_UPDATE, MONSTER_ACTION, MONSTER_DEATH
- **AI 관련**: BT_EXECUTE, BT_RESULT, BT_DEBUG

## 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다.

## 기여

1. 이 저장소를 포크합니다
2. 기능 브랜치를 생성합니다 (`git checkout -b feature/새기능`)
3. 변경사항을 커밋합니다 (`git commit -am '새 기능 추가'`)
4. 브랜치에 푸시합니다 (`git push origin feature/새기능`)
5. Pull Request를 생성합니다

## 문제 해결

### 빌드 오류

- CMake 버전 확인: `cmake --version`
- 컴파일러 버전 확인: `g++ --version`
- 의존성 설치: `sudo apt-get install build-essential cmake`

### 실행 오류

- 포트 사용 중: 다른 포트 사용 또는 기존 프로세스 종료
- 권한 오류: 실행 권한 확인 `chmod +x build/bin/BT_MMORPG_Server`
- 메모리 부족: 시스템 리소스 확인

## 향후 계획

- [ ] 데이터베이스 연동
- [ ] 암호화 통신
- [ ] 웹 관리 인터페이스
- [ ] 모니터링 시스템
- [ ] 로드 밸런싱
- [ ] 클러스터링 지원