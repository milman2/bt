# 클라이언트 소스 파일들
file(GLOB_RECURSE CLIENT_SOURCES "*.cpp")

# 헤더 파일 디렉토리 설정
include_directories(${CMAKE_SOURCE_DIR}/shared)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 클라이언트 실행 파일 생성
add_executable(AsioTestClient ${CLIENT_SOURCES})

# 라이브러리 링크
target_link_libraries(AsioTestClient 
    SharedProtocol
    Threads::Threads 
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
)

# 플랫폼 독립적 네트워킹 라이브러리
if(WIN32)
    target_link_libraries(AsioTestClient ws2_32 wsock32)
elseif(UNIX)
    target_link_libraries(AsioTestClient pthread)
endif()

# 컴파일 정의
target_compile_definitions(AsioTestClient PRIVATE 
    BT_CLIENT_BUILD
    HAVE_BOOST
    BOOST_BIND_GLOBAL_PLACEHOLDERS
)

# 소스 그룹 설정 (IDE에서 보기 좋게)
source_group("Source Files" FILES ${CLIENT_SOURCES})

# 클라이언트 전용 컴파일러 플래그
target_compile_options(AsioTestClient PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# 클라이언트 전용 링커 플래그
target_link_options(AsioTestClient PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-s>
)
