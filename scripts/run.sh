#!/bin/bash

# BT MMORPG 서버 실행 스크립트

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

# 서버 실행 파일 경로
SERVER_BINARY="build/bin/BT_MMORPG_Server"

# 실행 파일 존재 확인
if [ ! -f "$SERVER_BINARY" ]; then
    print_error "서버 실행 파일을 찾을 수 없습니다: $SERVER_BINARY"
    print_info "먼저 빌드를 실행하세요: ./scripts/build.sh"
    exit 1
fi

# 실행 파일 권한 확인
if [ ! -x "$SERVER_BINARY" ]; then
    print_warning "실행 파일에 실행 권한이 없습니다. 권한을 설정합니다..."
    chmod +x "$SERVER_BINARY"
fi

print_info "BT MMORPG 서버 시작 중..."

# 서버 실행
exec "$SERVER_BINARY" "$@"
