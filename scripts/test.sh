#!/bin/bash

# BT MMORPG 서버 테스트 스크립트

set -e

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 함수 정의
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 기본 설정
SERVER_BINARY="build/bin/BT_MMORPG_Server"
CLIENT_BINARY="build/bin/TestClient"
SERVER_PORT=8080
TEST_TYPE="all"
VERBOSE=false
STRESS_CONNECTIONS=10
STRESS_DURATION=30

# 서버 프로세스 ID
SERVER_PID=0

# 함수 정의
cleanup() {
    if [ $SERVER_PID -ne 0 ]; then
        print_info "서버 프로세스 종료 중... (PID: $SERVER_PID)"
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=0
    fi
}

# 시그널 핸들러 등록
trap cleanup EXIT INT TERM

start_server() {
    print_info "서버 시작 중..."
    
    if [ ! -f "$SERVER_BINARY" ]; then
        print_error "서버 실행 파일을 찾을 수 없습니다: $SERVER_BINARY"
        print_info "먼저 빌드를 실행하세요: make build"
        exit 1
    fi
    
    # 서버를 백그라운드에서 시작
    $SERVER_BINARY --port $SERVER_PORT --debug > server.log 2>&1 &
    SERVER_PID=$!
    
    # 서버가 시작될 때까지 대기
    print_info "서버 시작 대기 중..."
    sleep 3
    
    # 서버가 실행 중인지 확인
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        print_error "서버 시작 실패"
        cat server.log
        exit 1
    fi
    
    print_success "서버가 시작되었습니다 (PID: $SERVER_PID, 포트: $SERVER_PORT)"
}

stop_server() {
    if [ $SERVER_PID -ne 0 ]; then
        print_info "서버 종료 중..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=0
        print_success "서버가 종료되었습니다"
    fi
}

run_test() {
    local test_name="$1"
    local test_args="$2"
    
    print_info "=== $test_name 테스트 시작 ==="
    
    if [ ! -f "$CLIENT_BINARY" ]; then
        print_error "테스트 클라이언트 실행 파일을 찾을 수 없습니다: $CLIENT_BINARY"
        return 1
    fi
    
    local verbose_flag=""
    if [ "$VERBOSE" = true ]; then
        verbose_flag="--verbose"
    fi
    
    if $CLIENT_BINARY --port $SERVER_PORT $verbose_flag $test_args; then
        print_success "$test_name 테스트 성공"
        return 0
    else
        print_error "$test_name 테스트 실패"
        return 1
    fi
}

run_all_tests() {
    print_info "=== 전체 테스트 시작 ==="
    
    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    
    # 1. 연결 테스트
    total_tests=$((total_tests + 1))
    if run_test "연결" "--test connection"; then
        passed_tests=$((passed_tests + 1))
    else
        failed_tests=$((failed_tests + 1))
    fi
    
    # 2. 로그인 테스트
    total_tests=$((total_tests + 1))
    if run_test "로그인" "--test login"; then
        passed_tests=$((passed_tests + 1))
    else
        failed_tests=$((failed_tests + 1))
    fi
    
    # 3. 플레이어 이동 테스트
    total_tests=$((total_tests + 1))
    if run_test "플레이어 이동" "--test move"; then
        passed_tests=$((passed_tests + 1))
    else
        failed_tests=$((failed_tests + 1))
    fi
    
    # 4. 채팅 테스트
    total_tests=$((total_tests + 1))
    if run_test "채팅" "--test chat"; then
        passed_tests=$((passed_tests + 1))
    else
        failed_tests=$((failed_tests + 1))
    fi
    
    # 5. 연결 해제 테스트
    total_tests=$((total_tests + 1))
    if run_test "연결 해제" "--test disconnect"; then
        passed_tests=$((passed_tests + 1))
    else
        failed_tests=$((failed_tests + 1))
    fi
    
    # 결과 출력
    print_info "=== 테스트 결과 ==="
    print_info "총 테스트: $total_tests개"
    print_success "성공: $passed_tests개"
    if [ $failed_tests -gt 0 ]; then
        print_error "실패: $failed_tests개"
    else
        print_info "실패: $failed_tests개"
    fi
    
    if [ $failed_tests -eq 0 ]; then
        print_success "모든 테스트가 성공했습니다!"
        return 0
    else
        print_error "일부 테스트가 실패했습니다"
        return 1
    fi
}

run_stress_test() {
    print_info "=== 스트레스 테스트 시작 ==="
    print_info "연결 수: $STRESS_CONNECTIONS, 지속 시간: ${STRESS_DURATION}초"
    
    if run_test "스트레스" "--test stress --stress-connections $STRESS_CONNECTIONS --stress-duration $STRESS_DURATION"; then
        print_success "스트레스 테스트 성공"
        return 0
    else
        print_error "스트레스 테스트 실패"
        return 1
    fi
}

# 명령행 인수 파싱
while [[ $# -gt 0 ]]; do
    case $1 in
        --port)
            SERVER_PORT="$2"
            shift 2
            ;;
        --test)
            TEST_TYPE="$2"
            shift 2
            ;;
        --stress-connections)
            STRESS_CONNECTIONS="$2"
            shift 2
            ;;
        --stress-duration)
            STRESS_DURATION="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "BT MMORPG 서버 테스트 스크립트"
            echo ""
            echo "사용법: $0 [옵션]"
            echo ""
            echo "옵션:"
            echo "  --port <포트>              서버 포트 (기본값: 8080)"
            echo "  --test <테스트명>          실행할 테스트"
            echo "    all                     전체 테스트 (기본값)"
            echo "    connection              연결 테스트"
            echo "    login                   로그인 테스트"
            echo "    move                    플레이어 이동 테스트"
            echo "    chat                    채팅 테스트"
            echo "    disconnect              연결 해제 테스트"
            echo "    stress                  스트레스 테스트"
            echo "  --stress-connections <수>  스트레스 테스트 연결 수 (기본값: 10)"
            echo "  --stress-duration <초>     스트레스 테스트 지속 시간 (기본값: 30)"
            echo "  --verbose                 상세 로그 출력"
            echo "  --help                    이 도움말 표시"
            echo ""
            echo "예시:"
            echo "  $0                        # 전체 테스트 실행"
            echo "  $0 --test stress          # 스트레스 테스트 실행"
            echo "  $0 --test connection      # 연결 테스트만 실행"
            echo "  $0 --verbose              # 상세 로그와 함께 테스트"
            exit 0
            ;;
        *)
            print_error "알 수 없는 옵션: $1"
            exit 1
            ;;
    esac
done

# 메인 실행
print_info "BT MMORPG 서버 테스트 시작"
print_info "테스트 타입: $TEST_TYPE"
print_info "서버 포트: $SERVER_PORT"

# 서버 시작
start_server

# 테스트 실행
case $TEST_TYPE in
    "all")
        run_all_tests
        ;;
    "connection")
        run_test "연결" "--test connection"
        ;;
    "login")
        run_test "로그인" "--test login"
        ;;
    "move")
        run_test "플레이어 이동" "--test move"
        ;;
    "chat")
        run_test "채팅" "--test chat"
        ;;
    "disconnect")
        run_test "연결 해제" "--test disconnect"
        ;;
    "stress")
        run_stress_test
        ;;
    *)
        print_error "알 수 없는 테스트 타입: $TEST_TYPE"
        exit 1
        ;;
esac

test_result=$?

# 서버 종료
stop_server

# 최종 결과
if [ $test_result -eq 0 ]; then
    print_success "테스트 완료: 성공"
    exit 0
else
    print_error "테스트 완료: 실패"
    exit 1
fi
