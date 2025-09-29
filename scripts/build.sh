#!/bin/bash

# BT MMORPG 서버 빌드 스크립트

set -e  # 오류 발생 시 스크립트 종료

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# 빌드 타입 설정 (기본값: Release)
BUILD_TYPE=${1:-Release}

# 빌드 디렉토리
BUILD_DIR="build"

print_info "BT MMORPG 서버 빌드 시작..."
print_info "빌드 타입: $BUILD_TYPE"

# 이전 빌드 디렉토리 정리
if [ -d "$BUILD_DIR" ]; then
    print_info "이전 빌드 디렉토리 정리 중..."
    rm -rf "$BUILD_DIR"
fi

# 빌드 디렉토리 생성
print_info "빌드 디렉토리 생성 중..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake 설정
print_info "CMake 설정 중..."

# 패키지 매니저 설정
PACKAGE_MANAGER="auto"
if [ -n "$1" ] && [ "$1" = "--conan" ]; then
    PACKAGE_MANAGER="conan"
    shift
elif [ -n "$1" ] && [ "$1" = "--vcpkg" ]; then
    PACKAGE_MANAGER="vcpkg"
    shift
elif [ -n "$1" ] && [ "$1" = "--system" ]; then
    PACKAGE_MANAGER="system"
    shift
fi

# Conan 설정
if [ "$PACKAGE_MANAGER" = "conan" ] || [ "$PACKAGE_MANAGER" = "auto" ]; then
    if [ -f "../conanfile.txt" ]; then
        print_info "Conan 의존성 설치 중..."
        conan install .. --build=missing --output-folder=.
        if [ $? -eq 0 ]; then
            print_info "Conan 의존성 설치 완료"
            PACKAGE_MANAGER="conan"
        else
            print_warning "Conan 설치 실패, 다른 방법 시도"
            PACKAGE_MANAGER="auto"
        fi
    fi
fi

# vcpkg 툴체인 설정
VCPKG_TOOLCHAIN=""
if [ "$PACKAGE_MANAGER" = "vcpkg" ] || ([ "$PACKAGE_MANAGER" = "auto" ] && [ -n "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]); then
    VCPKG_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    print_info "vcpkg 툴체인 사용: $VCPKG_TOOLCHAIN"
    PACKAGE_MANAGER="vcpkg"
fi

# CMake 설정
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=g++"
CMAKE_ARGS="$CMAKE_ARGS -DPACKAGE_MANAGER=$PACKAGE_MANAGER"

if [ -n "$VCPKG_TOOLCHAIN" ]; then
    CMAKE_ARGS="$CMAKE_ARGS $VCPKG_TOOLCHAIN"
fi

print_info "CMake 설정 중... (패키지 매니저: $PACKAGE_MANAGER)"
cmake $CMAKE_ARGS ..

if [ $? -ne 0 ]; then
    print_error "CMake 설정 실패!"
    exit 1
fi

# 컴파일
print_info "컴파일 중..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    print_error "컴파일 실패!"
    exit 1
fi

print_success "빌드 완료!"
print_info "실행 파일 위치: $BUILD_DIR/bin/BT_MMORPG_Server"

# 실행 파일 권한 설정
chmod +x bin/BT_MMORPG_Server

print_info "빌드된 파일들:"
ls -la bin/

cd ..
print_success "빌드 스크립트 완료!"
