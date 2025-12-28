#!/usr/bin/env python3
import os
import re
import json
import argparse
import shutil
import sys
from log_utils import log_info, log_error, log_success, exit_with_error

def remove_comments(content):
    """
    去除 LD 脚本中的注释
    支持 C 风格的 /* */ 和 // 注释
    """
    content = re.sub(r'/\*.*?\*/', ' ', content, flags=re.DOTALL)
    content = re.sub(r'//.*', ' ', content)
    return content

def convert_size_to_hex(size_str):
    """
    将 LD 脚本中的大小表示转换为十六进制
    支持 K, M 等单位
    """
    size_str = size_str.strip().upper()
    if size_str.startswith('0X'):
        return '0x' + size_str[2:].lower()
    match = re.match(r'^(\d+)([KMG]?)$', size_str)
    if not match:
        try:
            num = int(size_str, 0)
            return f"0x{num:x}"
        except ValueError:
            return size_str
    number, unit = match.groups()
    number = int(number)
    if unit == 'K':
        number *= 1024
    elif unit == 'M':
        number *= 1024 * 1024
    elif unit == 'G':
        number *= 1024 * 1024 * 1024
    
    return f"0x{number:x}"


def parse_memory_regions(content, input_file_path):
    """
    解析 LD 脚本中的 MEMORY 区域
    """
    memory_pattern = r'MEMORY\s*{([^}]*(?:}[^}])*)}'
    memory_match = re.search(memory_pattern, content, re.IGNORECASE | re.DOTALL)
    if not memory_match:
        exit_with_error(f"MEMORY region not found in {input_file_path}")
    memory_content = memory_match.group(1)
    region_pattern = r'(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,\s]+)\s*,\s*LENGTH\s*=\s*([^,\s]+)'
    regions = re.findall(region_pattern, memory_content, re.IGNORECASE)
    if not regions:
        exit_with_error(f"No memory regions found in {input_file_path}")
    result = {}
    for name, origin, length in regions:
        result[name.strip()] = {
            'origin': convert_size_to_hex(origin.strip()),
            'length': convert_size_to_hex(length.strip())
        }
    return result

def clean_output(temp_dir, output_file):
    """Clean the output by removing the temporary directory, .ld.bfx files and output file."""
    success = True
    
    # Remove .ld.bfx files in the temp directory
    if os.path.exists(temp_dir):
        for file_name in os.listdir(temp_dir):
            if file_name.endswith('.ld.bfx'):
                bfx_file_path = os.path.join(temp_dir, file_name)
                try:
                    os.remove(bfx_file_path)
                    log_success(f"Removed .ld.bfx file: {bfx_file_path}")
                except Exception as e:
                    log_error(f"Failed to remove .ld.bfx file {bfx_file_path}: {str(e)}")
                    success = False
    
    # Remove the entire temp directory
    if os.path.exists(temp_dir):
        try:
            shutil.rmtree(temp_dir)
            log_success(f"Removed temporary directory: {temp_dir}")
        except Exception as e:
            log_error(f"Failed to remove temporary directory {temp_dir}: {str(e)}")
            success = False
    else:
        # 临时目录不存在时，不报错
        log_info(f"Temporary directory does not exist, skipping: {temp_dir}")
    
    if os.path.exists(output_file):
        try:
            os.remove(output_file)
            log_success(f"Removed output file: {output_file}")
        except Exception as e:
            log_error(f"Failed to remove output file {output_file}: {str(e)}")
            success = False
    else:
        # 输出文件不存在时，不报错
        log_info(f"Output file does not exist, skipping: {output_file}")
    
    return success

def main():
    parser = argparse.ArgumentParser(description='解析 LD 脚本中的 MEMORY 区域')
    parser.add_argument('-f', '--file', help='待解析的ld脚本路径及名称')
    parser.add_argument('-t', '--temp', help='中间文件存放路径')
    parser.add_argument('-o', '--output', help='解析出的json路径及名称')
    parser.add_argument('-c', '--clean', nargs=2, metavar=('TEMP_DIR', 'OUTPUT_FILE'), 
                        help='清除生成结果，参数为临时目录和输出文件')
    args = parser.parse_args()
    if args.clean:
        temp_dir, output_file = args.clean
        clean_output(temp_dir, output_file)
        return
    if not args.file or not args.temp or not args.output:
        parser.print_help()
        sys.exit(1)
    if not os.path.exists(args.file):
        exit_with_error(f"input file {args.file} does not exist")
        sys.exit(1)
    os.makedirs(args.temp, exist_ok=True)
    input_filename = os.path.basename(args.file)
    with open(args.file, 'r', encoding='utf-8') as f:
        content = f.read()
    content_no_comments = remove_comments(content)
    bfx_file_path = os.path.join(args.temp, f"{os.path.splitext(input_filename)[0]}.ld.bfx")
    with open(bfx_file_path, 'w', encoding='utf-8') as f:
        f.write(content_no_comments)
    memory_regions = parse_memory_regions(content_no_comments, args.file)
    output_dir = os.path.dirname(args.output)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    with open(args.output, 'w', encoding='utf-8') as f:
        json.dump(memory_regions, f, indent=2, ensure_ascii=False)
    log_success(f"parsed {len(memory_regions)} regions from {args.file}")

if __name__ == "__main__":
    main()