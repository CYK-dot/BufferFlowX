#!/usr/bin/env python3
import os
import re
import json
import argparse
import shutil
import sys

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
        print(f"[BFX_SECTION_TOOL][ERROR] MEMORY region not found in {input_file_path}")
        sys.exit(1)
    memory_content = memory_match.group(1)
    region_pattern = r'(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,\s]+)\s*,\s*LENGTH\s*=\s*([^,\s]+)'
    regions = re.findall(region_pattern, memory_content, re.IGNORECASE)
    if not regions:
        print(f"[BFX_SECTION_TOOL][ERROR] No memory regions found in {input_file_path}")
        sys.exit(1)
    result = {}
    for name, origin, length in regions:
        result[name.strip()] = {
            'origin': convert_size_to_hex(origin.strip()),
            'length': convert_size_to_hex(length.strip())
        }
    return result

def clean_output(temp_dir, output_file):
    """
    清除生成的结果
    """
    try:
        if os.path.exists(temp_dir):
            shutil.rmtree(temp_dir)
        if os.path.exists(output_file):
            os.remove(output_file)    
        print("[BFX_SECTION] [OK] cleanup completed")
    except Exception as e:
        print(f"[BFX_SECTION] [ERROR] cleanup error: {str(e)}")

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
        print(f"[BFX_SECTION] [ERROR] input file {args.file} does not exist")
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
    print(f"[BFX_SECTION] [OK] parsed {len(memory_regions)} regions from {args.file}")

if __name__ == "__main__":
    main()