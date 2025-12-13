#!/usr/bin/env python3
"""
PlantUML状态图ID分配器
功能：读取*_build.puml文件，为状态和事件分配唯一ID，支持增量编译
修正版本：修复状态定义和转换解析问题
"""

import os
import re
import sys
import argparse
from typing import Dict, List, Tuple, Set, Optional, Any
from dataclasses import dataclass, field
from enum import Enum


class DiagramType(Enum):
    """图类型枚举"""
    UNKNOWN = 0
    STATE_DIAGRAM = 1
    STATE_ID_DIAGRAM = 2
    EVENT_ID_DIAGRAM = 3


@dataclass
class State:
    """状态定义"""
    name: str
    description: str = ""
    parent: Optional[str] = None
    is_initial: bool = False
    id: Optional[int] = None
    full_name: str = ""


@dataclass
class Transition:
    """状态转换定义"""
    source: str
    target: str
    event: str
    id: Optional[int] = None


@dataclass
class ParsedDiagram:
    """已解析的图表"""
    type: DiagramType
    title: str
    content: str
    states: Dict[str, State] = field(default_factory=dict)
    transitions: List[Transition] = field(default_factory=list)
    state_ids: Dict[str, int] = field(default_factory=dict)
    event_ids: Dict[str, int] = field(default_factory=dict)


class PlantUMLIDDispatcher:
    """PlantUML ID分配器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.diagrams: List[ParsedDiagram] = []
        self.stats = {
            'total_states': 0,
            'total_events': 0,
            'new_state_ids': 0,
            'new_event_ids': 0,
            'existing_state_ids': 0,
            'existing_event_ids': 0
        }
    
    def parse_file(self, file_path: str) -> None:
        """解析PlantUML文件"""
        if not os.path.exists(file_path):
            print(f"错误: 文件不存在 - {file_path}")
            sys.exit(1)
        
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        if self.verbose:
            print(f"读取文件: {file_path} ({len(content)} 字符)")
        
        # 分割图表（每个@startuml...@enduml块）
        diagram_blocks = self._split_diagrams(content)
        
        for i, block in enumerate(diagram_blocks):
            diagram = self._parse_diagram_block(block, i)
            if diagram:
                self.diagrams.append(diagram)
    
    def _split_diagrams(self, content: str) -> List[str]:
        """分割多个图表块"""
        pattern = r'@startuml\s*(.*?)\s*@enduml'
        matches = list(re.finditer(pattern, content, re.DOTALL))
        
        if not matches:
            print("警告: 未找到有效的@startuml...@enduml块")
            return []
        
        diagrams = []
        for match in matches:
            # 提取整个图表块（包含@startuml和@enduml）
            start = match.start()
            end = match.end()
            diagrams.append(content[start:end])
        
        return diagrams
    
    def _parse_diagram_block(self, block: str, index: int) -> Optional[ParsedDiagram]:
        """解析单个图表块"""
        lines = block.strip().split('\n')
        
        if len(lines) < 2:
            return None
        
        # 提取标题（第一行@startuml后面的内容）
        title_match = re.match(r'@startuml\s+(.+)', lines[0].strip())
        if title_match:
            title = title_match.group(1).strip()
        else:
            title = f"diagram_{index}"
        
        # 确定图表类型
        diagram_type = self._detect_diagram_type(title, lines)
        
        # 创建图表对象
        diagram = ParsedDiagram(
            type=diagram_type,
            title=title,
            content=block
        )
        
        # 根据类型解析
        if diagram_type == DiagramType.STATE_DIAGRAM:
            self._parse_state_diagram(diagram, lines)
        elif diagram_type == DiagramType.STATE_ID_DIAGRAM:
            self._parse_id_diagram(diagram, lines, is_state=True)
        elif diagram_type == DiagramType.EVENT_ID_DIAGRAM:
            self._parse_id_diagram(diagram, lines, is_state=False)
        
        return diagram
    
    def _detect_diagram_type(self, title: str, lines: List[str]) -> DiagramType:
        """检测图表类型"""
        # 检查标题
        if title.startswith('__STATEID_GENERATED__'):
            return DiagramType.STATE_ID_DIAGRAM
        elif title.startswith('__EVENTID_GENERATED__'):
            return DiagramType.EVENT_ID_DIAGRAM
        
        # 检查内容 - 简单检查是否包含状态图元素
        content = '\n'.join(lines).lower()
        if 'state ' in content or '-->' in content:
            return DiagramType.STATE_DIAGRAM
        
        return DiagramType.UNKNOWN
    
    def _parse_state_diagram(self, diagram: ParsedDiagram, lines: List[str]) -> None:
        """解析状态图 - 修正版本"""
        state_stack = []  # 用于跟踪嵌套状态
        processed_states = set()  # 已处理的状态名
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            # 跳过开始和结束标记
            if line.startswith('@startuml') or line.startswith('@enduml'):
                i += 1
                continue
            
            # 跳过空行
            if not line:
                i += 1
                continue
            
            # 处理状态块开始
            state_start_match = re.match(r'^state\s+(\w+)\s*\{', line)
            if state_start_match:
                state_name = state_start_match.group(1)
                if self.verbose:
                    print(f"    开始状态块: {state_name}")
                
                # 检查这个状态是否已定义
                if not state_stack:
                    # 顶层状态
                    full_name = state_name
                    if full_name not in diagram.states:
                        diagram.states[full_name] = State(
                            name=state_name,
                            full_name=full_name,
                            parent=None
                        )
                else:
                    # 嵌套状态
                    full_name = f"{state_stack[-1]}_{state_name}"
                    if full_name not in diagram.states:
                        diagram.states[full_name] = State(
                            name=state_name,
                            full_name=full_name,
                            parent=state_stack[-1]
                        )
                
                state_stack.append(full_name)
                i += 1
                continue
            
            # 处理状态块结束
            if line == '}':
                if state_stack:
                    if self.verbose:
                        print(f"    结束状态块: {state_stack[-1]}")
                    state_stack.pop()
                i += 1
                continue
            
            # 处理状态定义行 (格式: StateName: Description)
            # 注意：一行中可能有多个状态定义，但PlantUML通常是每行一个
            if ':' in line and '-->' not in line:
                # 提取状态定义部分 (可能包含多个状态定义)
                parts = [p.strip() for p in line.split(':')]
                if len(parts) >= 2:
                    state_name = parts[0].strip()
                    description = ':'.join(parts[1:]).strip()  # 重新组合描述部分
                    
                    # 处理可能的多个状态名 (如 "Status1, Status2: 描述")
                    if ',' in state_name:
                        state_names = [s.strip() for s in state_name.split(',')]
                        # 只取第一个状态名作为主要状态，其他作为别名
                        state_name = state_names[0]
                    
                    # 构建完整状态名
                    if state_stack:
                        full_name = f"{state_stack[-1]}_{state_name}"
                        parent = state_stack[-1]
                    else:
                        full_name = state_name
                        parent = None
                    
                    # 检查是否为初始状态
                    is_initial = '初始状态' in description
                    
                    # 创建或更新状态
                    if full_name in diagram.states:
                        # 更新现有状态
                        diagram.states[full_name].description = description
                        diagram.states[full_name].is_initial = is_initial
                    else:
                        # 创建新状态
                        diagram.states[full_name] = State(
                            name=state_name,
                            description=description,
                            parent=parent,
                            is_initial=is_initial,
                            full_name=full_name
                        )
                    
                    if self.verbose:
                        print(f"    解析状态: {full_name} -> '{description}' (初始: {is_initial})")
                    
                    processed_states.add(full_name)
            
            # 处理状态转换 (格式: Source --> Target : Event)
            elif '-->' in line and ':' in line:
                # 提取转换部分
                match = re.match(r'^(\w+)\s*-->\s*(\w+)\s*:\s*(\w+)', line)
                if match:
                    source_name = match.group(1)
                    target_name = match.group(2)
                    event_name = match.group(3)
                    
                    # 构建完整状态名
                    if state_stack:
                        # 检查源状态是否已在处理的状态中
                        source_full = f"{state_stack[-1]}_{source_name}" if f"{state_stack[-1]}_{source_name}" in diagram.states else source_name
                        target_full = f"{state_stack[-1]}_{target_name}" if f"{state_stack[-1]}_{target_name}" in diagram.states else target_name
                    else:
                        source_full = source_name
                        target_full = target_name
                    
                    # 确保源状态存在
                    if source_full not in diagram.states:
                        diagram.states[source_full] = State(
                            name=source_name,
                            full_name=source_full,
                            parent=state_stack[-1] if state_stack else None
                        )
                    
                    # 确保目标状态存在
                    if target_full not in diagram.states:
                        diagram.states[target_full] = State(
                            name=target_name,
                            full_name=target_full,
                            parent=state_stack[-1] if state_stack else None
                        )
                    
                    # 添加到转换列表
                    diagram.transitions.append(Transition(
                        source=source_full,
                        target=target_full,
                        event=event_name
                    ))
                    
                    # 添加到事件列表（全局唯一）
                    if event_name not in diagram.event_ids:
                        diagram.event_ids[event_name] = None
                    
                    if self.verbose:
                        print(f"    解析转换: {source_full} -> {target_full} : {event_name}")
            
            i += 1
        
        if self.verbose:
            print(f"  解析完成: {len(diagram.states)} 个状态, {len(diagram.transitions)} 个转换")
    
    def _parse_id_diagram(self, diagram: ParsedDiagram, lines: List[str], is_state: bool = True) -> None:
        """解析ID图表"""
        for line in lines:
            line = line.strip()
            
            # 跳过开始和结束标记
            if line.startswith('@startuml') or line.startswith('@enduml'):
                continue
            
            # 跳过title行
            if line.startswith('title'):
                continue
            
            # 解析ID映射
            id_match = re.match(r'^(\w+)\s*:\s*(\d+)', line)
            if id_match:
                name = id_match.group(1)
                id_value = int(id_match.group(2))
                
                if is_state:
                    diagram.state_ids[name] = id_value
                    if self.verbose:
                        print(f"    读取状态ID: {name} = {id_value}")
                else:
                    diagram.event_ids[name] = id_value
                    if self.verbose:
                        print(f"    读取事件ID: {name} = {id_value}")
    
    def dispatch_ids(self) -> None:
        """分配ID"""
        # 查找状态图和ID图
        state_diagram = None
        state_id_diagram = None
        event_id_diagram = None
        
        for diagram in self.diagrams:
            if diagram.type == DiagramType.STATE_DIAGRAM:
                state_diagram = diagram
            elif diagram.type == DiagramType.STATE_ID_DIAGRAM:
                state_id_diagram = diagram
            elif diagram.type == DiagramType.EVENT_ID_DIAGRAM:
                event_id_diagram = diagram
        
        if not state_diagram:
            print("错误: 未找到状态图")
            sys.exit(1)
        
        # 收集所有状态和事件
        all_states = list(state_diagram.states.keys())
        all_events = set()
        
        for transition in state_diagram.transitions:
            all_events.add(transition.event)
        
        if self.verbose:
            print(f"收集到状态: {sorted(all_states)}")
            print(f"收集到事件: {sorted(all_events)}")
        
        self.stats['total_states'] = len(all_states)
        self.stats['total_events'] = len(all_events)
        
        # 获取现有的ID映射
        existing_state_ids = {}
        existing_event_ids = {}
        
        if state_id_diagram:
            existing_state_ids = state_id_diagram.state_ids
            self.stats['existing_state_ids'] = len(existing_state_ids)
            if self.verbose:
                print(f"已有状态ID: {existing_state_ids}")
        
        if event_id_diagram:
            existing_event_ids = event_id_diagram.event_ids
            self.stats['existing_event_ids'] = len(existing_event_ids)
            if self.verbose:
                print(f"已有事件ID: {existing_event_ids}")
        
        # 分配状态ID
        next_state_id = max(existing_state_ids.values(), default=-1) + 1
        state_id_map = existing_state_ids.copy()
        
        for state in sorted(all_states):
            if state not in state_id_map:
                state_id_map[state] = next_state_id
                next_state_id += 1
                self.stats['new_state_ids'] += 1
                if self.verbose:
                    print(f"分配新状态ID: {state} = {state_id_map[state]}")
        
        # 分配事件ID
        next_event_id = max(existing_event_ids.values(), default=-1) + 1
        event_id_map = existing_event_ids.copy()
        
        for event in sorted(all_events):
            if event not in event_id_map:
                event_id_map[event] = next_event_id
                next_event_id += 1
                self.stats['new_event_ids'] += 1
                if self.verbose:
                    print(f"分配新事件ID: {event} = {event_id_map[event]}")
        
        # 更新状态图中的ID引用
        for state_name, state_id in state_id_map.items():
            if state_name in state_diagram.states:
                state_diagram.states[state_name].id = state_id
        
        for transition in state_diagram.transitions:
            if transition.event in event_id_map:
                transition.id = event_id_map[transition.event]
        
        # 创建新的ID图表
        state_id_title = f"__STATEID_GENERATED__{state_diagram.title}"
        event_id_title = f"__EVENTID_GENERATED__{state_diagram.title}"
        
        # 生成状态ID图表内容
        state_id_content = self._generate_id_diagram_content(
            state_id_title, state_id_map, "状态ID映射"
        )
        
        # 生成事件ID图表内容
        event_id_content = self._generate_id_diagram_content(
            event_id_title, event_id_map, "事件ID映射"
        )
        
        # 创建新的图表对象
        new_state_id_diagram = ParsedDiagram(
            type=DiagramType.STATE_ID_DIAGRAM,
            title=state_id_title,
            content=state_id_content,
            state_ids=state_id_map
        )
        
        new_event_id_diagram = ParsedDiagram(
            type=DiagramType.EVENT_ID_DIAGRAM,
            title=event_id_title,
            content=event_id_content,
            event_ids=event_id_map
        )
        
        # 更新图表列表
        if state_id_diagram:
            self.diagrams.remove(state_id_diagram)
        if event_id_diagram:
            self.diagrams.remove(event_id_diagram)
        
        self.diagrams.append(new_state_id_diagram)
        self.diagrams.append(new_event_id_diagram)
    
    def _generate_id_diagram_content(self, title: str, id_map: Dict[str, int], description: str = "") -> str:
        """生成ID图表内容"""
        lines = [f"@startuml {title}"]
        
        if description:
            lines.append(f"title {description}")
        
        # 按ID排序输出
        sorted_items = sorted(id_map.items(), key=lambda x: x[1])
        for name, id_value in sorted_items:
            lines.append(f"    {name}: {id_value}")
        
        lines.append("@enduml")
        return '\n'.join(lines)
    
    def generate_output(self) -> str:
        """生成输出内容"""
        output_lines = []
        
        # 确保状态图在前，ID图在后
        state_diagram = None
        id_diagrams = []
        
        for diagram in self.diagrams:
            if diagram.type == DiagramType.STATE_DIAGRAM:
                state_diagram = diagram
            elif diagram.type in [DiagramType.STATE_ID_DIAGRAM, DiagramType.EVENT_ID_DIAGRAM]:
                id_diagrams.append(diagram)
        
        # 添加状态图
        if state_diagram:
            output_lines.append(state_diagram.content)
        
        # 添加ID图
        for diagram in id_diagrams:
            if not output_lines:  # 如果输出还是空的
                output_lines.append(diagram.content)
            else:
                output_lines.append('')  # 添加空行分隔
                output_lines.append(diagram.content)
        
        return '\n'.join(output_lines)
    
    def save_output(self, file_path: str) -> None:
        """保存输出到文件"""
        output_content = self.generate_output()
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(output_content)
        
        if self.verbose:
            print(f"已保存到: {file_path} ({len(output_content)} 字符)")
    
    def print_statistics(self) -> None:
        """打印统计信息"""
        print("\n" + "="*60)
        print("ID分配统计:")
        print(f"  总状态数: {self.stats['total_states']}")
        print(f"  总事件数: {self.stats['total_events']}")
        print(f"  已有状态ID: {self.stats['existing_state_ids']}")
        print(f"  已有事件ID: {self.stats['existing_event_ids']}")
        print(f"  新分配状态ID: {self.stats['new_state_ids']}")
        print(f"  新分配事件ID: {self.stats['new_event_ids']}")
        print("="*60)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='PlantUML状态图ID分配器 - 为状态和事件分配唯一ID',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s diagram_build.puml              # 处理文件
  %(prog)s diagram_build.puml -o output.puml  # 指定输出文件
  %(prog)s diagram_build.puml -v           # 详细模式
  %(prog)s -d diagrams/                    # 批量处理目录
  
支持增量编译:
  1. 如果文件中已有__STATEID_GENERATED__图表，会读取已有ID映射
  2. 只会为新状态/事件分配ID，保持已有ID不变
  3. 嵌套状态使用下划线分隔层次 (如: Status1_Sub1)
  4. 事件全局唯一，不添加层次前缀
        """
    )
    
    parser.add_argument('input', nargs='?', 
                       help='输入文件路径 (*_build.puml)')
    parser.add_argument('-o', '--output', 
                       help='输出文件路径 (默认: 覆盖输入文件)')
    parser.add_argument('-d', '--directory', 
                       help='处理目录中的所有*_build.puml文件')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='显示详细处理信息')
    parser.add_argument('--backup', action='store_true',
                       help='处理前备份原文件')
    parser.add_argument('--version', action='version', 
                       version='PlantUML ID分配器 v1.1 (修正版)')
    
    args = parser.parse_args()
    
    # 创建分配器实例
    dispatcher = PlantUMLIDDispatcher(verbose=args.verbose)
    
    def process_single_file(input_file: str, output_file: str = None) -> bool:
        """处理单个文件"""
        if args.backup:
            import shutil
            backup_file = f"{input_file}.backup"
            shutil.copy2(input_file, backup_file)
            if args.verbose:
                print(f"已创建备份: {backup_file}")
        
        print(f"解析文件: {input_file}")
        dispatcher.parse_file(input_file)
        
        if args.verbose:
            print(f"找到 {len(dispatcher.diagrams)} 个图表")
            for i, diagram in enumerate(dispatcher.diagrams):
                print(f"  图表 {i}: {diagram.title} ({diagram.type.name})")
        
        print("分配ID...")
        dispatcher.dispatch_ids()
        
        # 确定输出文件
        if output_file is None:
            output_file = input_file
        
        print(f"生成输出...")
        dispatcher.save_output(output_file)
        dispatcher.print_statistics()
        return True
    
    # 处理输入
    if args.directory:
        # 处理目录
        if not os.path.exists(args.directory):
            print(f"错误: 目录不存在 - {args.directory}")
            sys.exit(1)
        
        success_count = 0
        fail_count = 0
        
        for root, dirs, files in os.walk(args.directory):
            for file in files:
                if file.endswith('_build.puml'):
                    input_path = os.path.join(root, file)
                    
                    print(f"\n处理文件: {input_path}")
                    try:
                        if process_single_file(input_path):
                            success_count += 1
                        else:
                            fail_count += 1
                    except Exception as e:
                        print(f"错误: 处理失败 - {e}")
                        fail_count += 1
        
        print(f"\n完成! 成功处理 {success_count} 个文件，失败 {fail_count} 个文件")
        
    elif args.input:
        # 处理单个文件
        if not os.path.exists(args.input):
            print(f"错误: 文件不存在 - {args.input}")
            sys.exit(1)
        
        if not args.input.endswith('_build.puml') and not args.input.endswith('.puml'):
            print(f"警告: 输入文件不是*_build.puml格式 - {args.input}")
            response = input("是否继续? (y/N): ")
            if response.lower() != 'y':
                sys.exit(0)
        
        print(f"处理文件: {args.input}")
        process_single_file(args.input, args.output)
        
    else:
        # 没有输入参数
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()