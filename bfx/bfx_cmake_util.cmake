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

## @name bfx_add_section_management
    ## @brief add section management to cmake-target (generates linker script and copy sections code)
    ## @param TARGET_NAME cmake-target
    ## @param PROJECT_ROOT project root directory
    ## @param LD_TEMPLATE base linker script template path
    ## @param OUTPUT_DIR output directory for generated files
    ## @param OUTPUT_LD output path for generated linker script
    ## @param COPY_SECTIONS_FILE output path for copy sections JSON file
    ## @param OUTPUT_C_FILE output path for copy sections C file
##
function(bfx_add_section_management)
    # 解析函数参数
    set(oneValueArgs TARGET PROJECT_ROOT LD_TEMPLATE OUTPUT_DIR OUTPUT_LD COPY_SECTIONS_FILE OUTPUT_C_FILE)
    cmake_parse_arguments(BFX_SECTION "" "${oneValueArgs}" "" ${ARGN})
    
    # 验证必需参数
    if(NOT BFX_SECTION_TARGET)
        message(FATAL_ERROR "bfx_add_section_management: TARGET parameter is required")
    endif()
    if(NOT BFX_SECTION_PROJECT_ROOT)
        message(FATAL_ERROR "bfx_add_section_management: PROJECT_ROOT parameter is required")
    endif()
    if(NOT BFX_SECTION_LD_TEMPLATE)
        message(FATAL_ERROR "bfx_add_section_management: LD_TEMPLATE parameter is required")
    endif()
    if(NOT BFX_SECTION_OUTPUT_DIR)
        message(FATAL_ERROR "bfx_add_section_management: OUTPUT_DIR parameter is required")
    endif()
    if(NOT BFX_SECTION_OUTPUT_LD)
        message(FATAL_ERROR "bfx_add_section_management: OUTPUT_LD parameter is required")
    endif()
    if(NOT BFX_SECTION_COPY_SECTIONS_FILE)
        message(FATAL_ERROR "bfx_add_section_management: COPY_SECTIONS_FILE parameter is required")
    endif()
    if(NOT BFX_SECTION_OUTPUT_C_FILE)
        message(FATAL_ERROR "bfx_add_section_management: OUTPUT_C_FILE parameter is required")
    endif()
    
    # 验证目标是否存在
    if(NOT TARGET ${BFX_SECTION_TARGET})
        message(FATAL_ERROR "Target '${BFX_SECTION_TARGET}' does not exist")
    endif()
    
    # 验证Python脚本路径
    set(BFX_SECTION_SCRIPT ${BFX_CMAKE_ROOT_DIR}/section/gcc/bfx_section_main.py)
    if(NOT EXISTS ${BFX_SECTION_SCRIPT})
        message(FATAL_ERROR "BufferFlowX section management script not found: ${BFX_SECTION_SCRIPT}")
    endif()
    
    # 检查Python解释器
    find_package(Python3 REQUIRED)
    
    # 创建输出目录
    file(MAKE_DIRECTORY ${BFX_SECTION_OUTPUT_DIR})
    
    # 获取项目根目录的绝对路径
    get_filename_component(PROJECT_ROOT_ABS ${BFX_SECTION_PROJECT_ROOT} ABSOLUTE)
    get_filename_component(LD_TEMPLATE_ABS ${BFX_SECTION_LD_TEMPLATE} ABSOLUTE)
    
    # 创建自定义命令来生成链接脚本和C代码
    add_custom_command(
        OUTPUT ${BFX_SECTION_OUTPUT_LD} ${BFX_SECTION_OUTPUT_C_FILE}
        COMMAND ${Python3_EXECUTABLE}
                ${BFX_SECTION_SCRIPT}
                --project-root ${PROJECT_ROOT_ABS}
                --ld-template ${LD_TEMPLATE_ABS}
                --output-dir ${BFX_SECTION_OUTPUT_DIR}
                --output-ld ${BFX_SECTION_OUTPUT_LD}
                --copy-sections-file ${BFX_SECTION_COPY_SECTIONS_FILE}
                --output-c-file ${BFX_SECTION_OUTPUT_C_FILE}
        DEPENDS # 添加bfx.yml配置文件作为依赖，这些通常位于项目根目录下
        COMMENT "Generating linker script and copy sections code for ${BFX_SECTION_TARGET}"
        WORKING_DIRECTORY ${PROJECT_ROOT_ABS}
        VERBATIM
    )
    
    # 将生成的C文件添加到目标
    target_sources(${BFX_SECTION_TARGET} PRIVATE
        ${BFX_SECTION_OUTPUT_C_FILE}
    )
    
    # 设置链接器脚本
    set_target_properties(${BFX_SECTION_TARGET} PROPERTIES
        LINK_FLAGS "-T${BFX_SECTION_OUTPUT_LD}"
    )
    
    # 添加依赖，确保在构建目标前先生成链接脚本和C文件
    add_dependencies(${BFX_SECTION_TARGET} ${BFX_SECTION_TARGET}_section_generated)
    
    # 创建自定义目标来追踪生成的文件
    add_custom_target(${BFX_SECTION_TARGET}_section_generated
        DEPENDS ${BFX_SECTION_OUTPUT_LD} ${BFX_SECTION_OUTPUT_C_FILE}
        COMMENT "Section management generation for ${BFX_SECTION_TARGET}"
    )
    
    # 设置生成文件的属性
    set_source_files_properties(${BFX_SECTION_OUTPUT_C_FILE}
        PROPERTIES
            GENERATED TRUE
            SKIP_AUTOMOC TRUE  # 如果是 Qt 项目
    )
    
    # 清理生成的文件
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES
        ${BFX_SECTION_OUTPUT_LD}
        ${BFX_SECTION_OUTPUT_C_FILE}
        ${BFX_SECTION_COPY_SECTIONS_FILE}
        ${BFX_SECTION_OUTPUT_DIR}/*.ld.bfx  # 临时模板文件
    )
    
    # 打印成功信息
    message(STATUS "Added section management for target '${BFX_SECTION_TARGET}'")
    message(STATUS "  Project Root: ${PROJECT_ROOT_ABS}")
    message(STATUS "  LD Template: ${LD_TEMPLATE_ABS}")
    message(STATUS "  Output LD: ${BFX_SECTION_OUTPUT_LD}")
    message(STATUS "  Output C: ${BFX_SECTION_OUTPUT_C_FILE}")
endfunction()