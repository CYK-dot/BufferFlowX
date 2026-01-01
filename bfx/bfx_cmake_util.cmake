set(BFX_CMAKE_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

## @name bfx_get_include_dirs
    ## @brief append include directories to OUT_VAR
    ## @param OUT_VAR list of includes
##
macro(bfx_get_include_dirs OUT_VAR)
    list(APPEND ${OUT_VAR}
        ${BFX_CMAKE_ROOT_DIR}/common
        ${BFX_CMAKE_ROOT_DIR}/double_fifo
        ${BFX_CMAKE_ROOT_DIR}/siso_fifo
        ${BFX_CMAKE_ROOT_DIR}/cli
        ${BFX_CMAKE_ROOT_DIR}/fsm
        ${BFX_CMAKE_ROOT_DIR}/l2proto
    )
endmacro()

## @name bfx_get_fsm_srcs
    ## @brief append source files of BFX_FSM to OUT_VAR
    ## @param OUT_VAR list of sources
##
macro(bfx_get_fsm_srcs OUT_VAR)
    list(APPEND ${OUT_VAR}
        ${BFX_CMAKE_ROOT_DIR}/fsm/bfx_fsm.c
    )
endmacro()

## @name bfx_get_cli_srcs
    ## @brief append source files of BFX_CLI to OUT_VAR
    ## @param OUT_VAR list of sources
##
macro(bfx_get_cli_srcs OUT_VAR)
    list(APPEND ${OUT_VAR}
        ${BFX_CMAKE_ROOT_DIR}/cli/bfx_cli_core.c
        ${BFX_CMAKE_ROOT_DIR}/cli/bfx_cli_at.c
        ${BFX_CMAKE_ROOT_DIR}/cli/bfx_cli_unix.c
        ${BFX_CMAKE_ROOT_DIR}/cli/bfx_cli_view.c
    )
endmacro()

## @name bfx_get_l2proto_srcs
    ## @brief append source files of BFX_PROTOL2 to OUT_VAR
    ## @param OUT_VAR list of sources
##
macro(bfx_get_l2proto_srcs OUT_VAR)
    list(APPEND ${OUT_VAR}
        ${BFX_CMAKE_ROOT_DIR}/l2proto/bfx_l2proto.c
    )
endmacro()

## @name bfx_get_all_srcs
    ## @brief append all source files of BFX to OUT_VAR
    ## @param OUT_VAR list of sources
##
macro(bfx_get_all_srcs OUT_VAR)
    bfx_get_fsm_srcs(${OUT_VAR})
    bfx_get_cli_srcs(${OUT_VAR})
    bfx_get_l2proto_srcs(${OUT_VAR})
endmacro()

## @name bfx_add_puml_fsm
    ## @brief add script pre-compiler to cmake-target
    ## @param TARGET_NAME cmake-target
    ## @param PUML_FILE FSM description file(.puml) path
    ## @param OUTPUT_DIR generate .c/.h to which path
##
function(bfx_add_puml_fsm TARGET_NAME PUML_FILE OUTPUT_DIR)
    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "Target '${TARGET_NAME}' does not exist")
    endif()
    if(NOT EXISTS ${PUML_FILE})
        message(FATAL_ERROR "PlantUML file '${PUML_FILE}' does not exist")
    endif()
    
    # 获取绝对路径
    get_filename_component(PUML_ABS_PATH ${PUML_FILE} ABSOLUTE)
    get_filename_component(PUML_NAME ${PUML_FILE} NAME_WE)  # 不带扩展名的文件名
    
    # 生成的 C/H 文件名
    set(GENERATED_C_FILE ${OUTPUT_DIR}/${PUML_NAME}.c)
    set(GENERATED_H_FILE ${OUTPUT_DIR}/${PUML_NAME}.h)
    
    # 查找 Python 脚本路径
    if(EXISTS ${BFX_CMAKE_ROOT_DIR}/fsm/bfx_puml_translate.py)
        set(PYTHON_SCRIPT ${BFX_CMAKE_ROOT_DIR}/fsm/bfx_puml_translate.py)
    endif()
    
    # 检查 Python 解释器
    find_package(Python3 REQUIRED)
    
    # 检查是否能导入 Jinja2 模块
    if(Python3_Interpreter_FOUND)
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
            RESULT_VARIABLE JINJA2_IMPORT_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT JINJA2_IMPORT_RESULT EQUAL 0)
            message(FATAL_ERROR "Python module 'jinja2' not found. Please install it with: pip install jinja2")
        endif()
    endif()

    # 创建输出目录
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    
    # 创建自定义命令来生成代码
    add_custom_command(
        OUTPUT ${GENERATED_C_FILE} ${GENERATED_H_FILE}
        COMMAND ${Python3_EXECUTABLE}
                ${PYTHON_SCRIPT}
                ${PUML_ABS_PATH}
                ${OUTPUT_DIR}
        DEPENDS ${PUML_ABS_PATH} ${PYTHON_SCRIPT}
        COMMENT "Generating C code from PlantUML: ${PUML_NAME}.puml"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    
    # 将生成的文件添加到目标
    target_sources(${TARGET_NAME} PRIVATE
        ${GENERATED_C_FILE}
    )
    
    # 添加包含目录
    target_include_directories(${TARGET_NAME} PRIVATE
        ${OUTPUT_DIR}
    )
    
    # 添加依赖，确保在构建目标前先生成文件
    add_dependencies(${TARGET_NAME}
        ${PUML_NAME}_fsm_generated
    )
    
    # 创建自定义目标来追踪生成的文件
    add_custom_target(${PUML_NAME}_fsm_generated
        DEPENDS ${GENERATED_C_FILE} ${GENERATED_H_FILE}
    )
    
    # 设置生成文件的属性
    set_source_files_properties(${GENERATED_C_FILE} ${GENERATED_H_FILE}
        PROPERTIES
            GENERATED TRUE
            SKIP_AUTOMOC TRUE  # 如果是 Qt 项目
    )
    
    # 清理生成的文件
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES
        ${OUTPUT_DIR}/${PUML_NAME}.c
        ${OUTPUT_DIR}/${PUML_NAME}.h
    )
    
    # 打印成功信息
    message(STATUS "Added PlantUML FSM generation for target '${TARGET_NAME}'")
    message(STATUS "  Input: ${PUML_ABS_PATH}")
    message(STATUS "  Output: ${OUTPUT_DIR}")
endfunction()
