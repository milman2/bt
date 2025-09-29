cmake_minimum_required(VERSION 3.16)
project(BT_MMORPG_Server VERSION 1.0.0 LANGUAGES CXX)

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

# 헤더 파일 디렉토리
include_directories(include)

# 서버 소스 파일들 (테스트 클라이언트 제외)
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_client.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_client_main.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket_server.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/web_server.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/asio_test_client_main.cpp)

file(GLOB_RECURSE HEADERS "include/*.h")
list(REMOVE_ITEM HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/test_client.h)
list(REMOVE_ITEM HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/asio_test_client.h)

# 서버 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 기존 테스트 클라이언트는 제거됨 (AsioTestClient로 대체)

# Boost.Asio 테스트 클라이언트 소스 파일들
file(GLOB_RECURSE ASIO_TEST_CLIENT_SOURCES "src/asio_test_client*.cpp")
file(GLOB_RECURSE ASIO_TEST_CLIENT_HEADERS "include/asio_test_client.h")

# Boost.Asio 테스트 클라이언트 실행 파일 생성
add_executable(AsioTestClient ${ASIO_TEST_CLIENT_SOURCES} ${ASIO_TEST_CLIENT_HEADERS})

# 패키지 매니저 설정 (vcpkg 또는 Conan)
set(PACKAGE_MANAGER "auto" CACHE STRING "패키지 매니저 선택: auto, vcpkg, conan, system")

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
message(STATUS "Boost found: ${Boost_VERSION} (${PACKAGE_MANAGER}에서 설치됨)")
add_definitions(-DHAVE_BOOST)
add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

# nlohmann/json 라이브러리 찾기
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann/json found: ${nlohmann_json_VERSION}")

# OpenSSL 라이브러리 찾기 (WebSocket용)
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")

# 필요한 라이브러리들
find_package(Threads REQUIRED)

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME} 
    Threads::Threads 
    Boost::system 
    Boost::thread
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)
# TestClient는 제거됨
target_link_libraries(AsioTestClient 
    Threads::Threads 
    Boost::system 
    Boost::thread
    nlohmann_json::nlohmann_json
)

# 플랫폼 독립적 네트워킹
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
    # TestClient는 제거됨
    target_link_libraries(AsioTestClient ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
    # TestClient는 제거됨
    target_link_libraries(AsioTestClient pthread)
endif()

# 테스트 설정 (선택사항)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 설치 설정
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 설정 파일 설치
install(DIRECTORY config/ DESTINATION etc/bt_server)
