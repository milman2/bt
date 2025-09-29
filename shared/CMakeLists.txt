# 공통 라이브러리 (헤더 + 소스)
cmake_minimum_required(VERSION 3.16)
project(SharedProtocol)

# 공통 소스 파일들
file(GLOB_RECURSE SHARED_SOURCES "*.cpp")
file(GLOB_RECURSE SHARED_HEADERS "*.h")

# 공통 라이브러리 생성
add_library(${PROJECT_NAME} STATIC ${SHARED_SOURCES} ${SHARED_HEADERS})

# 헤더 파일들을 소스 그룹으로 설정 (IDE에서 보기 좋게)
source_group("Header Files" FILES ${SHARED_HEADERS})
source_group("Source Files" FILES ${SHARED_SOURCES})

# 헤더 파일들이 변경될 때마다 재빌드되도록 설정
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHARED_HEADERS})

# 이 디렉토리를 다른 프로젝트에서 include 할 수 있도록 설정
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
