import pytest
import os
import json
import shutil
import sys
import argparse
import tempfile

sys.path.append(os.path.join(os.path.dirname(__file__), '../../../bfx/section/adapt/gcc'))
import bfx_get_region


def test_get_region_should_create_and_delete_json_file():
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 解析ld脚本
    input_ld_path = os.path.join(os.path.dirname(__file__), 'basic_parse.ld')
    output_json_path = os.path.join(output_dir, 'basic_parse.json')
    run_args = ['-f', input_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    bfx_get_region.main()
    with open(output_json_path, 'r', encoding='utf-8') as f:
        memory_regions = json.load(f)
    # 判断文件是否存在且合法
    assert os.path.exists(output_json_path), "JSON file should be created"
    # 清理中间文件
    clean_args = ['-c', os.path.join(os.path.dirname(__file__), 'output'),
            output_json_path]
    sys.argv = ['bfx_get_region.py'] + clean_args
    bfx_get_region.main()
    # 验证中间文件是否被删除
    assert not os.path.exists(output_json_path), "JSON file should be removed"


def test_get_region_should_ignore_comments():
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 解析ld脚本
    input_ld_path = os.path.join(os.path.dirname(__file__), 'linker_with_comments.ld')
    output_json_path = os.path.join(output_dir, 'linker_with_comments.json')
    run_args = ['-f', input_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    bfx_get_region.main()
    with open(output_json_path, 'r', encoding='utf-8') as f:
        memory_regions = json.load(f)
    # 判断文件存在且合法
    assert os.path.exists(output_json_path), "JSON file should be created"
    # 判断解析结果是否正确
    expected_memory_regions = {
        "RAM": {
            "origin": "0x20000000",
            "length": "0x20000"
        },
        "CCMRAM": {
            "origin": "0x10000000",
            "length": "0x10000"
        },
        "FLASH": {
            "origin": "0x8000000",
            "length": "0x80000"
        }
    }
    assert json.dumps(memory_regions, sort_keys=True) == json.dumps(expected_memory_regions, sort_keys=True), f"Memory regions mismatch:\nActual: {json.dumps(memory_regions, indent=2)}\nExpected: {json.dumps(expected_memory_regions, indent=2)}"
    # 清理中间文件
    clean_args = ['-c', os.path.join(os.path.dirname(__file__), 'output'),
            output_json_path]
    sys.argv = ['bfx_get_region.py'] + clean_args
    bfx_get_region.main()


def test_get_region_should_parse_stm32cubemx_ld():
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 解析ld脚本
    input_ld_path = os.path.join(os.path.dirname(__file__), 'stm32.ld')
    output_json_path = os.path.join(output_dir, 'stm32cubemx.json')
    run_args = ['-f', input_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    bfx_get_region.main()
    with open(output_json_path, 'r', encoding='utf-8') as f:
        memory_regions = json.load(f)
    # 判断文件存在且合法
    assert os.path.exists(output_json_path), "JSON file should be created"
    # 判断解析结果是否正确
    expected_memory_regions = {
        "RAM": {
            "origin": "0x20000000",
            "length": "0x20000"
        },
        "CCMRAM": {
            "origin": "0x10000000",
            "length": "0x10000"
        },
        "FLASH": {
            "origin": "0x8000000",
            "length": "0x80000"
        }
    }
    assert json.dumps(memory_regions, sort_keys=True) == json.dumps(expected_memory_regions, sort_keys=True), f"Memory regions mismatch:\nActual: {json.dumps(memory_regions, indent=2)}\nExpected: {json.dumps(expected_memory_regions, indent=2)}"
    # 清理中间文件
    clean_args = ['-c', os.path.join(os.path.dirname(__file__), 'output'),
            output_json_path]
    sys.argv = ['bfx_get_region.py'] + clean_args
    bfx_get_region.main()


def test_get_region_missing_required_args():
    # 测试缺少必需参数的情况
    run_args = ['-f', 'dummy.ld']  # 缺少 -t 和 -o 参数
    sys.argv = ['bfx_get_region.py'] + run_args
    
    # 捕获系统退出异常
    with pytest.raises(SystemExit) as exc_info:
        bfx_get_region.main()
    assert exc_info.value.code == 1


def test_get_region_input_file_not_exists():
    # 测试输入文件不存在的情况
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    input_ld_path = os.path.join(os.path.dirname(__file__), 'nonexistent.ld')
    output_json_path = os.path.join(output_dir, 'nonexistent.json')
    run_args = ['-f', input_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    
    # 捕获系统退出异常
    with pytest.raises(SystemExit) as exc_info:
        bfx_get_region.main()
    assert exc_info.value.code == 1
    
    # 清理output目录
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)


def test_get_region_with_different_size_formats():
    # 测试不同大小格式的解析（十进制、十六进制、带单位）
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建临时LD文件，包含不同格式的大小
    temp_ld_path = os.path.join(os.path.dirname(__file__), 'size_formats.ld')
    with open(temp_ld_path, 'w', encoding='utf-8') as f:
        f.write("""
MEMORY
{
RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
CCMRAM (xrw)   : ORIGIN = 0x10000000, LENGTH = 0x10000
FLASH (rx)     : ORIGIN = 0x8000000, LENGTH = 512
EEPROM (rw)    : ORIGIN = 0x9000000, LENGTH = 4K
}
""")

    output_json_path = os.path.join(output_dir, 'size_formats.json')
    run_args = ['-f', temp_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    
    bfx_get_region.main()
    
    with open(output_json_path, 'r', encoding='utf-8') as f:
        memory_regions = json.load(f)
    
    # 验证解析结果
    expected_memory_regions = {
        "RAM": {
            "origin": "0x20000000",
            "length": "0x20000"  # 128K = 128*1024 = 0x20000
        },
        "CCMRAM": {
            "origin": "0x10000000",
            "length": "0x10000"  # 0x10000
        },
        "FLASH": {
            "origin": "0x8000000",
            "length": "0x200"    # 512 = 0x200
        },
        "EEPROM": {
            "origin": "0x9000000",
            "length": "0x1000"   # 4K = 4*1024 = 0x1000
        }
    }
    assert json.dumps(memory_regions, sort_keys=True) == json.dumps(expected_memory_regions, sort_keys=True), f"Memory regions mismatch:\nActual: {json.dumps(memory_regions, indent=2)}\nExpected: {json.dumps(expected_memory_regions, indent=2)}"
    
    # 清理临时文件
    if os.path.exists(temp_ld_path):
        os.remove(temp_ld_path)
    
    # 清理output目录
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)


def test_get_region_clean_function():
    # 测试清理功能
    output_dir = os.path.join(os.path.dirname(__file__), 'output_to_clean')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建临时文件
    test_file_path = os.path.join(output_dir, 'test_file.txt')
    with open(test_file_path, 'w') as f:
        f.write('test content')
    
    output_json_path = os.path.join(os.path.dirname(__file__), 'test_output.json')
    with open(output_json_path, 'w') as f:
        f.write('{}')
    
    # 验证文件存在
    assert os.path.exists(test_file_path)
    assert os.path.exists(output_json_path)
    
    # 使用清理参数
    clean_args = ['-c', output_dir, output_json_path]
    sys.argv = ['bfx_get_region.py'] + clean_args
    bfx_get_region.main()
    
    # 验证文件已被删除
    assert not os.path.exists(test_file_path), "Temporary directory should be removed"
    assert not os.path.exists(output_json_path), "Output file should be removed"
    
    # 清理output目录
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)


def test_get_region_ld_without_regions():
    # 创建一个MEMORY区域但没有实际内存区域定义的LD脚本
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建临时LD文件，有MEMORY区域但没有区域定义
    temp_ld_path = os.path.join(os.path.dirname(__file__), 'no_regions.ld')
    with open(temp_ld_path, 'w', encoding='utf-8') as f:
        f.write("""
/* This is a test LD script with MEMORY section but no regions */
ENTRY(Reset_Handler)

MEMORY
{
    /* Empty MEMORY section */
}

SECTIONS
{
    .text : {
        *(.text*)
    }
}
""")

    output_json_path = os.path.join(output_dir, 'no_regions.json')
    run_args = ['-f', temp_ld_path,
            '-t', os.path.join(os.path.dirname(__file__), 'output'),
            '-o', output_json_path]
    sys.argv = ['bfx_get_region.py'] + run_args
    
    # 捕获系统退出异常
    with pytest.raises(SystemExit) as exc_info:
        bfx_get_region.main()
    assert exc_info.value.code == 1
    
    # 清理临时文件
    if os.path.exists(temp_ld_path):
        os.remove(temp_ld_path)
    
    # 清理output目录
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)