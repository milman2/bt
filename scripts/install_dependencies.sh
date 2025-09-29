#!/bin/bash

# BT MMORPG 서버 의존성 설치 스크립트

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

print_info "BT MMORPG 서버 의존성 설치 시작..."

# 시스템 업데이트
print_info "시스템 패키지 업데이트 중..."
sudo apt-get update

# 필수 빌드 도구 설치
print_info "빌드 도구 설치 중..."
sudo apt-get install -y \
    build-essential \
    cmake \
    make \
    git \
    pkg-config

# C++ 개발 도구 설치
print_info "C++ 개발 도구 설치 중..."
sudo apt-get install -y \
    g++ \
    gdb \
    valgrind \
    cppcheck \
    clang-format

# Boost 라이브러리 설치
print_info "Boost 라이브러리 설치 중..."
sudo apt-get install -y \
    libboost-all-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-chrono-dev

# 네트워킹 관련 라이브러리
print_info "네트워킹 라이브러리 설치 중..."
sudo apt-get install -y \
    libssl-dev \
    libcrypto++-dev

# 데이터베이스 라이브러리 (선택사항)
print_info "데이터베이스 라이브러리 설치 중..."
sudo apt-get install -y \
    libmysqlclient-dev \
    libsqlite3-dev

# 로깅 라이브러리
print_info "로깅 라이브러리 설치 중..."
sudo apt-get install -y \
    libspdlog-dev

# JSON 처리 라이브러리
print_info "JSON 처리 라이브러리 설치 중..."
sudo apt-get install -y \
    nlohmann-json3-dev

# 테스트 프레임워크
print_info "테스트 프레임워크 설치 중..."
sudo apt-get install -y \
    libgtest-dev \
    libgmock-dev

# 성능 분석 도구
print_info "성능 분석 도구 설치 중..."
sudo apt-get install -y \
    perf-tools-unstable \
    linux-tools-generic

# 개발 도구
print_info "개발 도구 설치 중..."
sudo apt-get install -y \
    vim \
    nano \
    curl \
    wget \
    htop \
    netstat-nat

print_success "의존성 설치 완료!"

# 설치된 도구 버전 확인
print_info "설치된 도구 버전 확인:"
echo "GCC: $(gcc --version | head -n1)"
echo "G++: $(g++ --version | head -n1)"
echo "CMake: $(cmake --version | head -n1)"
echo "Make: $(make --version | head -n1)"

print_info "개발 환경 설정 완료!"
print_info "이제 'make build' 명령으로 프로젝트를 빌드할 수 있습니다."
