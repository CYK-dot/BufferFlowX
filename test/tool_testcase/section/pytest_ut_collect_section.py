import pytest
import os
import json
import shutil
import sys
import argparse
import tempfile

sys.path.append(os.path.join(os.path.dirname(__file__), '../../../bfx/section/adapt/gcc'))
import bfx_collect_section


def test_collect_section_should_find_all_yml_files():
    # 测试收集所有 bfx.yml 文件
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 使用当前目录作为项目根目录
    project_root = os.path.dirname(__file__)
    output_json_path = os.path.join(output_dir, 'collected_sections.json')
    
    # 准备命令行参数
    run_args = ['-p', project_root, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行收集函数
    exit_code = bfx_collect_section.main()
    
    # 验证退出码
    assert exit_code == 0, "Collection should succeed"
    
    # 验证输出文件存在
    assert os.path.exists(output_json_path), "JSON output file should be created"
    
    # 读取并验证内容
    with open(output_json_path, 'r', encoding='utf-8') as f:
        collected_data = json.load(f)
    
    # 验证收集的数据结构
    assert 'project_root' in collected_data
    assert 'bfx_files' in collected_data
    assert 'sections' in collected_data
    
    # 验证项目根目录
    assert collected_data['project_root'] == project_root
    
    # 验证找到的 bfx.yml 文件数量（至少3个：top_level_bfx.yml, subdir/bfx.yml, subdir2/bfx.yml）
    assert len(collected_data['bfx_files']) >= 3, f"Should find at least 3 bfx.yml files, found {len(collected_data['bfx_files'])}"
    
    # 验证 sections 现在是字典，键是组件名
    assert isinstance(collected_data['sections'], dict), "Sections should be a dictionary keyed by component name"
    
    # 计算总 sections 数量（所有组件的 sections 总和）
    total_sections = sum(len(sections_list) for sections_list in collected_data['sections'].values())
    assert total_sections >= 4, f"Should find at least 4 sections in total, found {total_sections}"
    
    # 验证每个组件的 sections 列表
    for component_name, sections_list in collected_data['sections'].items():
        assert isinstance(sections_list, list), f"Sections for component {component_name} should be a list"
        for section in sections_list:
            assert 'source_file' in section
            assert 'component_name' in section
            assert 'section_name' in section
            assert 'region' in section
            assert 'align' in section
            assert 'address' in section
            assert 'size' in section
            # 验证组件名与字典键一致
            assert section['component_name'] == component_name
    
    # 清理输出文件
    if os.path.exists(output_json_path):
        os.remove(output_json_path)


def test_collect_section_clean_function():
    # 测试清理功能
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建临时输出文件
    output_json_path = os.path.join(output_dir, 'test_collect_clean.json')
    with open(output_json_path, 'w') as f:
        json.dump({"test": "data"}, f)
    
    # 验证文件存在
    assert os.path.exists(output_json_path)
    
    # 准备清理参数
    run_args = ['-c', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行清理函数
    exit_code = bfx_collect_section.main()
    
    # 验证退出码（清理成功时返回0）
    assert exit_code == 0, "Clean should succeed"
    
    # 验证文件已被删除
    assert not os.path.exists(output_json_path), "Output file should be removed"


def test_collect_section_missing_required_args():
    # 测试缺少必需参数的情况
    run_args = ['-p', 'dummy_dir']  # 缺少 -t 参数
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # main函数应该返回1（错误码）
    exit_code = bfx_collect_section.main()
    assert exit_code == 1


def test_collect_section_project_dir_not_exists():
    # 测试项目目录不存在的情况
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    project_dir = os.path.join(os.path.dirname(__file__), 'nonexistent_dir')
    output_json_path = os.path.join(output_dir, 'nonexistent.json')
    run_args = ['-p', project_dir, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # main函数应该返回1（错误码）
    exit_code = bfx_collect_section.main()
    assert exit_code == 1
    
    # 清理output目录
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)


def test_collect_section_with_invalid_yaml():
    # 测试包含无效 YAML 的情况
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 使用包含无效 YAML 的目录作为项目根目录
    project_root = os.path.dirname(__file__)
    output_json_path = os.path.join(output_dir, 'collected_with_invalid.json')
    
    # 准备命令行参数
    run_args = ['-p', project_root, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行收集函数
    # 注意：即使有无效的 YAML 文件，程序也应该成功完成（错误处理）
    exit_code = bfx_collect_section.main()
    
    # 验证退出码（即使有错误文件，只要能处理错误就返回0）
    assert exit_code == 0, "Collection should handle invalid YAML gracefully"
    
    # 验证输出文件存在
    assert os.path.exists(output_json_path), "JSON output file should still be created"
    
    # 清理输出文件
    if os.path.exists(output_json_path):
        os.remove(output_json_path)


def test_collect_section_validation_errors():
    # 测试验证错误处理
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建一个包含无效字段的临时 bfx.yml 文件
    temp_dir = os.path.join(output_dir, 'temp_invalid_test')
    os.makedirs(temp_dir, exist_ok=True)
    
    invalid_yml_content = """
name: test_component
section:
  - name: .test_section
    region: RAM
    invalid_field: 4
"""
    
    invalid_yml_path = os.path.join(temp_dir, 'bfx.yml')
    with open(invalid_yml_path, 'w', encoding='utf-8') as f:
        f.write(invalid_yml_content)
    
    output_json_path = os.path.join(output_dir, 'collected_with_validation_errors.json')
    
    # 准备命令行参数
    run_args = ['-p', temp_dir, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行收集函数
    exit_code = bfx_collect_section.main()
    
    # 验证退出码
    assert exit_code == 0, "Collection should handle validation errors gracefully"
    
    # 验证输出文件存在（即使有验证错误的文件，其他有效文件仍应被处理）
    assert os.path.exists(output_json_path), "JSON output file should still be created"
    
    # 读取并验证内容
    with open(output_json_path, 'r', encoding='utf-8') as f:
        collected_data = json.load(f)
    
    # 验证有效文件被处理
    assert 'sections' in collected_data
    assert isinstance(collected_data['sections'], dict)
    
    # 清理
    if os.path.exists(output_json_path):
        os.remove(output_json_path)
    shutil.rmtree(temp_dir)


def test_collect_section_duplicate_sections():
    # 测试重复 section 名称检测 - 现在应该返回失败
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建一个包含重复 section 名称的临时 bfx.yml 文件
    temp_dir = os.path.join(output_dir, 'temp_duplicate_test')
    os.makedirs(temp_dir, exist_ok=True)
    
    duplicate_yml_content = """
name: test_component
section:
  - name: .duplicate_section
    region: RAM
  - name: .duplicate_section  # 重复名称
    region: FLASH
"""
    
    duplicate_yml_path = os.path.join(temp_dir, 'bfx.yml')
    with open(duplicate_yml_path, 'w', encoding='utf-8') as f:
        f.write(duplicate_yml_content)
    
    output_json_path = os.path.join(output_dir, 'collected_with_duplicates.json')
    
    # 准备命令行参数
    run_args = ['-p', temp_dir, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行收集函数
    exit_code = bfx_collect_section.main()
    
    # 验证退出码 - 现在遇到重复段应该返回失败（退出码为1）
    assert exit_code == 1, "Collection should fail when duplicate sections are found"
    
    # 验证输出文件不存在，因为收集过程失败
    assert not os.path.exists(output_json_path), "JSON output file should not be created when duplicate sections are found"
    
    # 清理
    if os.path.exists(output_json_path):
        os.remove(output_json_path)
    shutil.rmtree(temp_dir)


def test_collect_section_aggregation():
    # 测试组件名聚合功能
    output_dir = os.path.join(os.path.dirname(__file__), 'output')
    os.makedirs(output_dir, exist_ok=True)
    
    # 创建多个目录，每个目录有相同组件名但不同 sections 的 bfx.yml 文件
    temp_dir = os.path.join(output_dir, 'temp_aggregation_test')
    os.makedirs(temp_dir, exist_ok=True)
    
    # 创建子目录
    subdir1 = os.path.join(temp_dir, 'subdir1')
    subdir2 = os.path.join(temp_dir, 'subdir2')
    os.makedirs(subdir1, exist_ok=True)
    os.makedirs(subdir2, exist_ok=True)
    
    # 创建第一个子目录的 bfx.yml
    yml_content1 = """
name: shared_component
section:
  - name: .section1
    region: RAM
    address: 0x20000000
    size: 0x100
"""
    
    yml_content2 = """
name: shared_component
section:
  - name: .section2
    region: FLASH
    address: 0x8000000
    size: 0x200
"""
    
    with open(os.path.join(subdir1, 'bfx.yml'), 'w', encoding='utf-8') as f:
        f.write(yml_content1)
    
    with open(os.path.join(subdir2, 'bfx.yml'), 'w', encoding='utf-8') as f:
        f.write(yml_content2)
    
    output_json_path = os.path.join(output_dir, 'collected_with_aggregation.json')
    
    # 准备命令行参数
    run_args = ['-p', temp_dir, '-t', output_json_path]
    sys.argv = ['bfx_collect_section.py'] + run_args
    
    # 运行收集函数
    exit_code = bfx_collect_section.main()
    
    # 验证退出码
    assert exit_code == 0, "Collection should handle aggregation successfully"
    
    # 验证输出文件存在
    assert os.path.exists(output_json_path), "JSON output file should be created"
    
    # 读取并验证内容
    with open(output_json_path, 'r', encoding='utf-8') as f:
        collected_data = json.load(f)
    
    # 验证 sections 现在是字典
    assert isinstance(collected_data['sections'], dict)
    
    # 验证 'shared_component' 只出现一次（聚合）
    assert 'shared_component' in collected_data['sections']
    assert len(collected_data['sections']) == 1, f"Should have only 1 component after aggregation, found {len(collected_data['sections'])}"
    
    # 验证该组件有2个 sections（从两个不同文件聚合而来）
    shared_sections = collected_data['sections']['shared_component']
    assert len(shared_sections) == 2, f"Should have 2 sections in shared_component after aggregation, found {len(shared_sections)}"
    
    section_names = [s['section_name'] for s in shared_sections]
    assert '.section1' in section_names
    assert '.section2' in section_names
    
    # 清理
    if os.path.exists(output_json_path):
        os.remove(output_json_path)
    shutil.rmtree(temp_dir)