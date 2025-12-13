# 项目配置 ==============================================================

cmake_minimum_required(VERSION 3.10)
project(easy_test
    VERSION 1.0.0
    DESCRIPTION "Unit tests for angle loop"
    LANGUAGES C CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# gtest目录配置 ========================================================

set(GTEST_ROOT ${CMAKE_SOURCE_DIR}/lib)
set(GMOCK_ROOT ${CMAKE_SOURCE_DIR}/lib)
set(GTEST_HEADER ${CMAKE_SOURCE_DIR}/lib/headers)

# 代码配置 ==============================================================

# cmake function headers
include(../bfx/bfx_cmake_util.cmake)
include(./lib/gtest_cov.cmake)

# global defines
set(GLOBAL_DEFINES

)

# global includes
set(GLOBAL_INCLUDES
)
bfx_get_include_dirs(GLOBAL_INCLUDES)
message("inc_dir=${GLOBAL_INCLUDES}")

# 代码源文件 ============================================================

# code to test
set(CODE_SOURCES
)
bfx_get_all_srcs(CODE_SOURCES)

# code of testcases
set(TEST_SOURCES
    "testcase/dfifo.cpp"
    "testcase/siso_fifo_nosplit.cpp"
    "testcase/siso_fifo_split.cpp"
    "testcase/cli_raw.cpp"
    "testcase/fsm.cpp"
)

# 可执行文件 ============================================================
enable_code_coverage()

add_executable(${PROJECT_NAME} 
    ${CODE_SOURCES}
    ${TEST_SOURCES}
    "./main.cpp"
)
add_compile_definitions(
    ${GLOBAL_DEFINES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLOBAL_INCLUDES}
    ${GTEST_HEADER}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${GTEST_ROOT}/libgtest.a"
    "${GMOCK_ROOT}/libgmock.a"
    "${GMOCK_ROOT}/libgtest_main.a"
    pthread
)
bfx_add_puml_fsm(${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/testcase/FsmTest.puml"
    "${CMAKE_CURRENT_SOURCE_DIR}/testcase/generated/"
)
add_coverage_target(${PROJECT_NAME})