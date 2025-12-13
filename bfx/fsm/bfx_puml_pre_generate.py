#!/usr/bin/env python3
"""
PlantUML预编译器脚本
功能：读取.puml文件，删除注释，生成*_build.puml文件
"""

import os
import re
import sys
import argparse
from typing import List, Tuple

class PlantUMLPrecompiler:
    def __init__(self, verbose: bool = False):
        """
        初始化预编译器
        
        Args:
            verbose: 是否显示详细处理信息
        """
        self.verbose = verbose
        self.stats = {
            'files_processed': 0,
            'comments_removed': 0,
            'files_skipped': 0
        }
    
    def remove_comments(self, content: str) -> str:
        """
        删除PlantUML文件中的注释
        
        Args:
            content: PlantUML文件内容
            
        Returns:
            删除注释后的内容
        """
        lines = content.split('\n')
        result_lines = []
        in_multiline_comment = False
        
        for line in lines:
            original_line = line
            line_stripped = line.strip()
            
            # 跳过空行
            if not line_stripped:
                result_lines.append(line)
                continue
            
            # 处理多行注释
            if in_multiline_comment:
                if "*/" in line:
                    # 找到多行注释结束位置
                    end_index = line.find("*/") + 2
                    line = line[end_index:]
                    in_multiline_comment = False
                    if self.verbose:
                        print(f"    [多行注释结束] 行内容: {original_line[:50]}...")
                else:
                    # 整行都在多行注释中，跳过该行
                    self.stats['comments_removed'] += 1
                    if self.verbose:
                        print(f"    [跳过整行注释] 行内容: {original_line[:50]}...")
                    continue
            
            # 检查是否有多行注释开始
            if "/*" in line:
                if "*/" in line:
                    # 单行内的多行注释
                    line = re.sub(r'/\*.*?\*/', '', line)
                    self.stats['comments_removed'] += 1
                    if self.verbose:
                        print(f"    [删除单行多行注释] 行内容: {original_line[:50]}...")
                else:
                    # 多行注释开始
                    start_index = line.find("/*")
                    line = line[:start_index]
                    in_multiline_comment = True
                    self.stats['comments_removed'] += 1
                    if self.verbose:
                        print(f"    [多行注释开始] 行内容: {original_line[:50]}...")
            
            # 处理单行注释
            if not in_multiline_comment:
                # 查找单行注释位置
                comment_index = -1
                
                # 先处理以'开头的注释
                if "'" in line:
                    quote_index = line.find("'")
                    # 检查前面是否有转义字符
                    if quote_index > 0 and line[quote_index-1] != '\\':
                        comment_index = quote_index
                
                # 处理以//开头的注释
                if "//" in line:
                    double_slash_index = line.find("//")
                    if comment_index == -1 or double_slash_index < comment_index:
                        comment_index = double_slash_index
                
                # 删除注释部分
                if comment_index != -1:
                    line = line[:comment_index].rstrip()
                    self.stats['comments_removed'] += 1
                    if self.verbose:
                        print(f"    [删除单行注释] 原始行: {original_line[:50]}...")
                        print(f"    [删除单行注释] 处理后: {line[:50]}...")
            
            # 如果行不为空，添加到结果
            if line.strip():
                result_lines.append(line)
            elif original_line.strip():  # 原行非空但处理后为空，保留空行保持格式
                result_lines.append('')
        
        # 检查是否有多行注释未关闭
        if in_multiline_comment and self.verbose:
            print("    [警告] 检测到未关闭的多行注释")
        
        return '\n'.join(result_lines)
    
    def preprocess_file(self, input_file: str, output_file: str = None) -> bool:
        """
        预处理单个.puml文件
        
        Args:
            input_file: 输入文件路径
            output_file: 输出文件路径（可选，默认自动生成）
            
        Returns:
            bool: 是否成功处理
        """
        if not os.path.exists(input_file):
            print(f"错误: 文件不存在 - {input_file}")
            return False
        
        if not input_file.lower().endswith('.puml'):
            print(f"警告: 文件扩展名不是.puml - {input_file}")
        
        # 生成输出文件名
        if output_file is None:
            base_name = os.path.splitext(input_file)[0]
            output_file = f"{base_name}_build.puml"
        
        try:
            # 读取文件内容
            with open(input_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            if self.verbose:
                print(f"处理文件: {input_file}")
                print(f"原始大小: {len(content)} 字符")
            
            # 删除注释
            processed_content = self.remove_comments(content)
            
            if self.verbose:
                print(f"处理后大小: {len(processed_content)} 字符")
                print(f"保存到: {output_file}")
            
            # 写入输出文件
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(processed_content)
            
            self.stats['files_processed'] += 1
            return True
            
        except UnicodeDecodeError:
            print(f"错误: 文件编码问题，尝试使用其他编码 - {input_file}")
            try:
                # 尝试其他常见编码
                with open(input_file, 'r', encoding='gbk') as f:
                    content = f.read()
                
                processed_content = self.remove_comments(content)
                
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write(processed_content)
                
                self.stats['files_processed'] += 1
                return True
                
            except Exception as e:
                print(f"错误: 无法读取文件 - {input_file}: {e}")
                self.stats['files_skipped'] += 1
                return False
                
        except Exception as e:
            print(f"错误: 处理文件失败 - {input_file}: {e}")
            self.stats['files_skipped'] += 1
            return False
    
    def preprocess_directory(self, input_dir: str, output_dir: str = None, 
                           recursive: bool = False) -> Tuple[int, int]:
        """
        预处理目录中的所有.puml文件
        
        Args:
            input_dir: 输入目录
            output_dir: 输出目录（可选，默认在输入目录内创建_build子目录）
            recursive: 是否递归处理子目录
            
        Returns:
            Tuple[int, int]: (成功处理文件数, 失败文件数)
        """
        if not os.path.isdir(input_dir):
            print(f"错误: 目录不存在 - {input_dir}")
            return (0, 1)
        
        # 设置输出目录
        if output_dir is None:
            output_dir = os.path.join(input_dir, "_build")
        
        # 创建输出目录
        os.makedirs(output_dir, exist_ok=True)
        
        success_count = 0
        fail_count = 0
        
        # 获取文件列表
        for root, dirs, files in os.walk(input_dir):
            if not recursive and root != input_dir:
                continue
            
            for file in files:
                if file.lower().endswith('.puml'):
                    input_path = os.path.join(root, file)
                    
                    # 计算相对路径
                    if root == input_dir:
                        rel_path = file
                    else:
                        rel_path = os.path.relpath(input_path, input_dir)
                    
                    # 构建输出路径
                    output_path = os.path.join(output_dir, rel_path)
                    output_dir_path = os.path.dirname(output_path)
                    os.makedirs(output_dir_path, exist_ok=True)
                    
                    # 处理文件
                    print(f"处理: {rel_path}")
                    if self.preprocess_file(input_path, output_path):
                        success_count += 1
                    else:
                        fail_count += 1
        
        return success_count, fail_count
    
    def print_statistics(self):
        """打印处理统计信息"""
        print("\n" + "="*50)
        print("处理统计:")
        print(f"  已处理文件: {self.stats['files_processed']}")
        print(f"  删除注释: {self.stats['comments_removed']}")
        print(f"  跳过文件: {self.stats['files_skipped']}")
        print("="*50)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='PlantUML预编译器 - 删除.puml文件中的注释并生成编译版本',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s diagram.puml              # 处理单个文件
  %(prog)s -v diagram.puml           # 详细模式处理单个文件
  %(prog)s -o custom_output.puml diagram.puml  # 指定输出文件名
  %(prog)s -d diagrams/              # 处理目录中的所有.puml文件
  %(prog)s -d diagrams/ -r           # 递归处理目录
  
支持的注释类型:
  1. 单行注释: // 注释内容
  2. 单引号注释: ' 注释内容
  3. 多行注释: /* 注释内容 */
        """
    )
    
    parser.add_argument('input', nargs='?', 
                       help='输入文件或目录路径')
    parser.add_argument('-o', '--output', 
                       help='输出文件路径（仅处理单个文件时有效）')
    parser.add_argument('-d', '--directory', 
                       help='处理目录中的所有.puml文件')
    parser.add_argument('-r', '--recursive', action='store_true',
                       help='递归处理子目录')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='显示详细处理信息')
    parser.add_argument('--version', action='version', 
                       version='PlantUML预编译器 v1.0')
    
    args = parser.parse_args()
    
    # 创建预编译器实例
    precompiler = PlantUMLPrecompiler(verbose=args.verbose)
    
    # 处理输入
    if args.directory:
        # 处理目录
        if not os.path.exists(args.directory):
            print(f"错误: 目录不存在 - {args.directory}")
            sys.exit(1)
        
        print(f"处理目录: {args.directory}")
        success, fail = precompiler.preprocess_directory(
            args.directory, 
            recursive=args.recursive
        )
        
        print(f"\n完成! 成功处理 {success} 个文件，失败 {fail} 个文件")
        
    elif args.input:
        # 处理单个文件
        if not os.path.exists(args.input):
            print(f"错误: 文件不存在 - {args.input}")
            sys.exit(1)
        
        if os.path.isdir(args.input):
            print(f"注意: {args.input} 是目录，使用 -d 参数处理目录")
            sys.exit(1)
        
        print(f"处理文件: {args.input}")
        if precompiler.preprocess_file(args.input, args.output):
            print(f"完成! 输出文件: {args.output or os.path.splitext(args.input)[0] + '_build.puml'}")
        else:
            print("处理失败!")
            sys.exit(1)
        
    else:
        # 没有输入参数
        parser.print_help()
        sys.exit(1)
    
    # 打印统计信息
    precompiler.print_statistics()


if __name__ == "__main__":
    main()