cmake_minimum_required(VERSION 3.16)

# BT 라이브러리 프로젝트 설정
project(BT_Library VERSION 1.0.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 플래그 설정
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# 헤더 파일들
set(BT_HEADERS
    BTNode.h
    BTContext.h
    BehaviorTree.h
    BehaviorTreeEngine.h
    IBTExecutor.h
    Action/BTAction.h
    Condition/BTCondition.h
    Control/BTSequence.h
    Control/BTSelector.h
    Control/BTParallel.h
    Control/BTRandom.h
    Decorator/BTDelay.h
    Decorator/BTInvert.h
    Decorator/BTRepeat.h
    Decorator/BTTimeout.h
)

# 소스 파일들 (모든 구현이 헤더 파일에 있음)
set(BT_SOURCES
)

# BT 라이브러리 생성 (헤더 전용)
add_library(BT_Library INTERFACE)

# 헤더 파일들을 include 디렉토리로 설정
target_include_directories(BT_Library INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 컴파일 정의
target_compile_definitions(BT_Library INTERFACE
    BT_LIBRARY_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    BT_LIBRARY_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    BT_LIBRARY_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# 외부 의존성
find_package(nlohmann_json REQUIRED)
target_link_libraries(BT_Library INTERFACE nlohmann_json::nlohmann_json)

# 설치 설정
install(TARGETS BT_Library
    EXPORT BT_LibraryTargets
    INCLUDES DESTINATION include
)

install(FILES ${BT_HEADERS}
    DESTINATION include
)

install(EXPORT BT_LibraryTargets
    FILE BT_LibraryTargets.cmake
    NAMESPACE BT::
    DESTINATION lib/cmake/BT_Library
)

# Config.cmake 파일 생성
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/BT_LibraryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/BT_LibraryConfig.cmake"
    INSTALL_DESTINATION lib/cmake/BT_Library
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/BT_LibraryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/BT_LibraryConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BT_LibraryConfigVersion.cmake"
    DESTINATION lib/cmake/BT_Library
)

# 테스트 설정 (선택사항)
option(BUILD_BT_TESTS "Build BT library tests" OFF)

if(BUILD_BT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
