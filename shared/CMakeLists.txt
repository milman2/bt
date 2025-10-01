# 공통 라이브러리 (헤더 + 소스)
cmake_minimum_required(VERSION 3.16)
project(SharedProtocol)

# Protocol Buffers 파일들
set(PROTO_FILES "Game/packets.proto")
set(PROTO_SRCS)
set(PROTO_HDRS)

# protobuf 컴파일 - generated 폴더에 생성
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)

    set(PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Game/generated/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_SOURCE_DIR}/Game/generated/${PROTO_NAME}.pb.h")

    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND /home/milman2/vcpkg/installed/x64-linux/tools/protobuf/protoc-29.5.0
        ARGS --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/Game/generated
             --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/Game
             ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        COMMENT "Generating protobuf files for ${PROTO_FILE}"
    )
endforeach()

# 공통 소스 파일들
file(GLOB_RECURSE SHARED_SOURCES "*.cpp")
file(GLOB_RECURSE SHARED_HEADERS "*.h")

# 공통 라이브러리 생성 (protobuf 생성 파일 포함)
add_library(${PROJECT_NAME} STATIC ${SHARED_SOURCES} ${SHARED_HEADERS} ${PROTO_SRCS} ${PROTO_HDRS})

# 헤더 파일들을 소스 그룹으로 설정 (IDE에서 보기 좋게)
source_group("Header Files" FILES ${SHARED_HEADERS})
source_group("Source Files" FILES ${SHARED_SOURCES})

# 헤더 파일들이 변경될 때마다 재빌드되도록 설정
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHARED_HEADERS})

# 이 디렉토리를 다른 프로젝트에서 include 할 수 있도록 설정
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# protobuf 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PUBLIC protobuf::libprotobuf)
