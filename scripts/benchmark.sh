#!/bin/bash

# BT MMORPG 서버 벤치마크 스크립트

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
BENCHMARK_RESULTS="benchmark_results.txt"

# 벤치마크 설정
CONNECTION_COUNTS=(10 50 100 200 500)
DURATION_SECONDS=30
MESSAGE_INTERVAL=1

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

run_connection_benchmark() {
    local connections=$1
    local duration=$2
    
    print_info "연결 수: $connections, 지속 시간: ${duration}초"
    
    # 벤치마크 시작 시간 기록
    local start_time=$(date +%s)
    
    # 테스트 클라이언트로 스트레스 테스트 실행
    if $CLIENT_BINARY --port $SERVER_PORT --test stress \
        --stress-connections $connections --stress-duration $duration > /dev/null 2>&1; then
        local end_time=$(date +%s)
        local actual_duration=$((end_time - start_time))
        
        print_success "벤치마크 완료: $connections개 연결, ${actual_duration}초 소요"
        
        # 결과를 파일에 기록
        echo "$connections,$actual_duration,SUCCESS" >> "$BENCHMARK_RESULTS"
        
        return 0
    else
        print_error "벤치마크 실패: $connections개 연결"
        echo "$connections,0,FAILED" >> "$BENCHMARK_RESULTS"
        return 1
    fi
}

run_throughput_benchmark() {
    local connections=$1
    local duration=$2
    
    print_info "처리량 테스트: $connections개 연결, ${duration}초"
    
    # 여러 클라이언트를 동시에 실행하여 메시지 전송
    local pids=()
    local messages_sent=0
    
    for ((i=1; i<=connections; i++)); do
        (
            local client_messages=0
            local start_time=$(date +%s)
            
            while [ $(($(date +%s) - start_time)) -lt $duration ]; do
                if $CLIENT_BINARY --port $SERVER_PORT --test chat > /dev/null 2>&1; then
                    client_messages=$((client_messages + 1))
                fi
                sleep $MESSAGE_INTERVAL
            done
            
            echo $client_messages > "/tmp/client_${i}_messages"
        ) &
        pids+=($!)
    done
    
    # 모든 클라이언트 완료 대기
    for pid in "${pids[@]}"; do
        wait $pid
    done
    
    # 총 메시지 수 계산
    for ((i=1; i<=connections; i++)); do
        if [ -f "/tmp/client_${i}_messages" ]; then
            local client_messages=$(cat "/tmp/client_${i}_messages")
            messages_sent=$((messages_sent + client_messages))
            rm -f "/tmp/client_${i}_messages"
        fi
    done
    
    local messages_per_second=$((messages_sent / duration))
    print_success "처리량 테스트 완료: ${messages_sent}개 메시지, ${messages_per_second} msg/sec"
    
    # 결과를 파일에 기록
    echo "THROUGHPUT,$connections,$messages_sent,$messages_per_second" >> "$BENCHMARK_RESULTS"
    
    return 0
}

run_latency_benchmark() {
    local connections=$1
    
    print_info "지연시간 테스트: $connections개 연결"
    
    local total_latency=0
    local successful_tests=0
    
    for ((i=1; i<=connections; i++)); do
        local start_time=$(date +%s%3N)  # 밀리초 단위
        
        if $CLIENT_BINARY --port $SERVER_PORT --test connection > /dev/null 2>&1; then
            local end_time=$(date +%s%3N)
            local latency=$((end_time - start_time))
            total_latency=$((total_latency + latency))
            successful_tests=$((successful_tests + 1))
        fi
    done
    
    if [ $successful_tests -gt 0 ]; then
        local avg_latency=$((total_latency / successful_tests))
        print_success "지연시간 테스트 완료: 평균 ${avg_latency}ms"
        
        # 결과를 파일에 기록
        echo "LATENCY,$connections,$avg_latency,$successful_tests" >> "$BENCHMARK_RESULTS"
        return 0
    else
        print_error "지연시간 테스트 실패"
        echo "LATENCY,$connections,0,0" >> "$BENCHMARK_RESULTS"
        return 1
    fi
}

generate_report() {
    print_info "벤치마크 보고서 생성 중..."
    
    local report_file="benchmark_report_$(date +%Y%m%d_%H%M%S).txt"
    
    {
        echo "BT MMORPG 서버 벤치마크 보고서"
        echo "생성 시간: $(date)"
        echo "=========================================="
        echo ""
        
        echo "연결 테스트 결과:"
        echo "연결수,소요시간(초),상태"
        grep "^[0-9]" "$BENCHMARK_RESULTS" | grep "SUCCESS" || echo "테스트 결과 없음"
        echo ""
        
        echo "처리량 테스트 결과:"
        echo "연결수,총메시지수,초당메시지수"
        grep "^THROUGHPUT" "$BENCHMARK_RESULTS" || echo "테스트 결과 없음"
        echo ""
        
        echo "지연시간 테스트 결과:"
        echo "연결수,평균지연시간(ms),성공한테스트수"
        grep "^LATENCY" "$BENCHMARK_RESULTS" || echo "테스트 결과 없음"
        echo ""
        
        echo "시스템 정보:"
        echo "OS: $(uname -a)"
        echo "CPU: $(lscpu | grep "Model name" | cut -d: -f2 | xargs)"
        echo "메모리: $(free -h | grep "Mem:" | awk '{print $2}')"
        echo "서버 포트: $SERVER_PORT"
        
    } > "$report_file"
    
    print_success "벤치마크 보고서 생성 완료: $report_file"
    cat "$report_file"
}

# 메인 실행
print_info "BT MMORPG 서버 벤치마크 시작"

# 결과 파일 초기화
> "$BENCHMARK_RESULTS"

# 서버 시작
start_server

# 벤치마크 실행
print_info "=== 연결 테스트 벤치마크 ==="
for connections in "${CONNECTION_COUNTS[@]}"; do
    run_connection_benchmark $connections $DURATION_SECONDS
    sleep 2  # 서버 안정화를 위한 대기
done

print_info "=== 처리량 테스트 벤치마크 ==="
for connections in "${CONNECTION_COUNTS[@]}"; do
    run_throughput_benchmark $connections $DURATION_SECONDS
    sleep 2
done

print_info "=== 지연시간 테스트 벤치마크 ==="
for connections in "${CONNECTION_COUNTS[@]}"; do
    run_latency_benchmark $connections
    sleep 1
done

# 서버 종료
stop_server

# 보고서 생성
generate_report

print_success "벤치마크 완료!"
