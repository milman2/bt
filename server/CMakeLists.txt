# 서버 소스 파일들
file(GLOB_RECURSE SERVER_SOURCES "*.cpp")

# 헤더 파일 디렉토리 설정
include_directories(${CMAKE_SOURCE_DIR}/shared)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/BT)

# 서버 실행 파일 생성
add_executable(BT_MMORPG_Server ${SERVER_SOURCES})

# 라이브러리 링크
target_link_libraries(BT_MMORPG_Server 
    SharedProtocol
    BT_Library
    Threads::Threads
    Boost::system
    Boost::thread
    Boost::chrono
    Boost::date_time
    Boost::atomic
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 컴파일 정의
target_compile_definitions(BT_MMORPG_Server PRIVATE 
    BT_SERVER_BUILD
    HAVE_BOOST
    BOOST_BIND_GLOBAL_PLACEHOLDERS
)

# 소스 그룹 설정 (IDE에서 보기 좋게)
source_group("Source Files" FILES ${SERVER_SOURCES})

# 서버 전용 컴파일러 플래그
target_compile_options(BT_MMORPG_Server PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# 서버 전용 링커 플래그
target_link_options(BT_MMORPG_Server PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-s>
)

# 설정 파일을 빌드 디렉토리로 복사
file(COPY ${CMAKE_SOURCE_DIR}/config/ DESTINATION ${CMAKE_BINARY_DIR}/config/)
