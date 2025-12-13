# coverage.cmake
# 启用覆盖率编译选项
function(enable_code_coverage)
    # 检查编译器
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling code coverage for ${CMAKE_CXX_COMPILER_ID}")
        
        # 为所有目标添加覆盖率选项
        add_compile_options(
            -g
            -O0
            -fprofile-arcs
            -ftest-coverage
        )
        
        # 注意：add_link_options 需要 CMake 3.13+
        add_link_options(
            -fprofile-arcs
            -ftest-coverage
        )
        
        # 设置环境变量，确保生成覆盖率数据
        set(ENV{CFLAGS} "-fprofile-arcs -ftest-coverage")
        set(ENV{CXXFLAGS} "-fprofile-arcs -ftest-coverage")
        set(ENV{LDFLAGS} "-fprofile-arcs -ftest-coverage")
        
        # 确保覆盖率数据文件能被找到
        if(WIN32)
            set(ENV{GCOV_PREFIX} ${CMAKE_BINARY_DIR})
            set(ENV{GCOV_PREFIX_STRIP} 0)
        endif()
        
    else()
        message(WARNING "Code coverage not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# 为指定目标添加覆盖率报告
function(add_coverage_target TARGET_NAME)
    # 确保目标存在
    if(NOT TARGET ${TARGET_NAME})
        message(WARNING "Target ${TARGET_NAME} does not exist")
        return()
    endif()
    
    # 查找 gcovr
    find_program(GCOVR gcovr)
    if(NOT GCOVR)
        message(WARNING "gcovr not found. Install with: pip install gcovr")
        return()
    endif()
    
    # 设置覆盖率目录
    set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
    
    # 创建覆盖率目标名称（避免与现有目标冲突）
    set(COVERAGE_TARGET_NAME "${TARGET_NAME}_coverage_report")
    
    # 如果目标已存在，跳过
    if(TARGET ${COVERAGE_TARGET_NAME})
        return()
    endif()
    
    # 创建覆盖率报告目标
    add_custom_target(${COVERAGE_TARGET_NAME}
        # 创建输出目录
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
        
        # 清理旧的覆盖率数据（可选）
        COMMAND ${CMAKE_COMMAND} -E rm -f ${COVERAGE_DIR}/*.html ${COVERAGE_DIR}/*.xml
        
        # 运行测试（如果有的话）
        # 注意：这里假设你会手动运行测试，或者通过其他方式运行
        
        # 生成覆盖率报告
        COMMAND ${GCOVR}
            --root ${CMAKE_SOURCE_DIR}/../
            --exclude "${CMAKE_SOURCE_DIR}/.*"
            --html-details ${COVERAGE_DIR}/coverage.html
            --xml ${COVERAGE_DIR}/coverage.xml
            --print-summary
            --sonarqube ${COVERAGE_DIR}/sonarqube.xml
        
        # 复制 .gcno 文件到构建目录（如果需要）
        COMMAND ${CMAKE_COMMAND} -E echo "Copying coverage data files..."
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report for ${TARGET_NAME}"
    )
    
    # 添加清理目标
    add_custom_target("${TARGET_NAME}_coverage_clean"
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_DIR}
        COMMAND find . -name "*.gcda" -type f -delete
        COMMAND find . -name "*.gcno" -type f -delete
        COMMENT "Cleaning coverage data for ${TARGET_NAME}"
    )
    
    message(STATUS "Added coverage target: ${COVERAGE_TARGET_NAME}")
    message(STATUS "Run tests, then: cmake --build . --target ${COVERAGE_TARGET_NAME}")
endfunction()