cmake_minimum_required(VERSION 3.16)
project(BT_MMORPG VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 플래그 설정
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")

# 디렉토리 구조 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 패키지 매니저 설정 (vcpkg 또는 Conan)
set(PACKAGE_MANAGER "auto" CACHE STRING "패키지 매니저 선택: auto, vcpkg, conan, system")

# vcpkg 경로 설정
if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux")
        message(STATUS "vcpkg 경로 설정: ${CMAKE_PREFIX_PATH}")
        # vcpkg에서 설치된 패키지들을 찾기 위해 경로 추가
        set(CMAKE_MODULE_PATH "${CMAKE_PREFIX_PATH}/share")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" ${CMAKE_PREFIX_PATH})
    endif()
endif()

if(PACKAGE_MANAGER STREQUAL "auto")
    # 자동 감지: vcpkg -> Conan -> system 순서
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(PACKAGE_MANAGER "vcpkg")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
        set(PACKAGE_MANAGER "conan")
    else()
        set(PACKAGE_MANAGER "system")
    endif()
endif()

message(STATUS "선택된 패키지 매니저: ${PACKAGE_MANAGER}")

# vcpkg 설정
if(PACKAGE_MANAGER STREQUAL "vcpkg")
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        message(STATUS "vcpkg 툴체인 설정: ${CMAKE_TOOLCHAIN_FILE}")
        set(VCPKG_TOOLCHAIN TRUE)
        # vcpkg 설치 경로 설정
        set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/x64-linux")
        message(STATUS "vcpkg 설치 경로: ${VCPKG_INSTALLED_DIR}")
        # vcpkg 툴체인이 제대로 작동하도록 설정
        set(CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}")
        set(CMAKE_MODULE_PATH "${VCPKG_INSTALLED_DIR}/share")
    else()
        message(FATAL_ERROR "vcpkg가 선택되었지만 VCPKG_ROOT 환경변수가 설정되지 않았습니다.")
    endif()
endif()

# Conan 설정
if(PACKAGE_MANAGER STREQUAL "conan")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
        message(FATAL_ERROR "Conan이 선택되었지만 conanfile.txt가 없습니다.")
    endif()
    
    # Conan 툴체인 파일이 있는지 확인
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conan_toolchain.cmake")
        include("${CMAKE_CURRENT_BINARY_DIR}/conan_toolchain.cmake")
        message(STATUS "Conan 툴체인 사용")
    else()
        message(WARNING "Conan 툴체인 파일이 없습니다. 'conan install .'을 먼저 실행하세요.")
    endif()
endif()

# Boost 라이브러리 찾기 (필수)
find_package(Boost REQUIRED COMPONENTS system thread)
message(STATUS "Boost found: ${Boost_VERSION}")

# nlohmann/json 라이브러리 찾기
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann/json found: ${nlohmann_json_VERSION}")

# OpenSSL 라이브러리 찾기 (WebSocket용)
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
add_definitions(-DHAVE_BOOST)
add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

# 필요한 라이브러리들
find_package(Threads REQUIRED)

# 공통 라이브러리 설정
set(COMMON_LIBRARIES
    Threads::Threads 
    Boost::system 
    Boost::thread
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 플랫폼 독립적 네트워킹 라이브러리
if(WIN32)
    list(APPEND COMMON_LIBRARIES ws2_32 wsock32)
elseif(UNIX)
    list(APPEND COMMON_LIBRARIES pthread)
endif()

# 서브디렉토리 추가
add_subdirectory(shared)
add_subdirectory(BT)
add_subdirectory(server)
add_subdirectory(client)

# 테스트 설정
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    # BT 라이브러리 테스트는 BT/CMakeLists.txt에서 처리됨
endif()

# 설치 설정
install(TARGETS BT_MMORPG_Server AsioTestClient
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 설정 파일 설치
install(DIRECTORY config/ DESTINATION etc/bt_server)