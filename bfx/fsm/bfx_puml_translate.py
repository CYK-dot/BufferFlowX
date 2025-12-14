#!/usr/bin/env python3
"""
PlantUML状态图转C代码生成器
支持简单状态图，支持嵌套状态（单层嵌套）
"""

import re
import sys
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple
from jinja2 import Template

@dataclass
class Event:
    """事件类"""
    name: str
    comment: str = ""
    id: int = 0

@dataclass
class State:
    """状态类"""
    name: str
    comment: str = ""
    id: int = 0
    parent: Optional['State'] = None
    initial_substate: Optional[str] = None  # 初始子状态名
    is_composite: bool = False
    children: Dict[str, 'State'] = field(default_factory=dict)
    
    def get_full_name(self) -> str:
        """获取完整状态名（包含父状态前缀）"""
        if self.parent:
            return f"{self.parent.get_full_name()}_{self.name}"
        return self.name
    
    def get_macro_name(self, project_name: str) -> str:
        """获取宏定义名"""
        full_name = self.get_full_name()
        return f"{project_name.upper()}_{full_name.upper()}"
    
    def get_callback_name(self, project_name: str) -> str:
        """获取回调函数名"""
        # 状态名转换为首字母大写，其余小写
        def capitalize_name(name: str) -> str:
            parts = name.split('_')
            return ''.join(part.capitalize() for part in parts)
        
        if self.parent:
            # 嵌套状态：BFX_project_Parent_Child_ActionCb
            parent_cap = capitalize_name(self.parent.name)
            child_cap = capitalize_name(self.name)
            return f"BFX_{project_name}_{parent_cap}_{child_cap}_ActionCb"
        else:
            # 顶层状态：BFX_project_State_ActionCb
            state_cap = capitalize_name(self.name)
            return f"BFX_{project_name}_{state_cap}_ActionCb"

@dataclass
class Transition:
    """状态转移类"""
    from_state: str
    to_state: str
    event: str
    event_comment: str = ""

class PlantUMLParser:
    """PlantUML解析器"""
    
    def __init__(self):
        self.project_name = ""
        self.states: Dict[str, State] = {}  # 状态名 -> State对象
        self.events: Dict[str, Event] = {}  # 事件名 -> Event对象
        self.transitions: List[Transition] = []
        self.current_parent: Optional[State] = None
        self.top_level_initial: Optional[str] = None
        
    def parse(self, uml_text: str):
        """解析PlantUML文本"""
        lines = uml_text.strip().split('\n')
        
        # 提取项目名
        for line in lines:
            line = line.strip()
            if line.startswith('@startuml'):
                parts = line.split()
                if len(parts) > 1:
                    self.project_name = parts[1]
                break
        
        # 状态栈，用于处理嵌套状态
        state_stack: List[State] = []
        
        for line in lines:
            line = line.strip()
            
            # 跳过空行和注释
            if not line or line.startswith("'") or line.startswith('@'):
                continue
            
            # 处理状态声明: "statename: comment"
            if ':' in line and not line.startswith('state ') and not '-->' in line:
                parts = line.split(':', 1)
                state_name = parts[0].strip()
                comment = parts[1].strip() if len(parts) > 1 else ""
                
                # 创建状态对象
                state = State(
                    name=state_name,
                    comment=comment,
                    parent=state_stack[-1] if state_stack else None
                )
                
                # 添加到状态字典
                full_name = state.get_full_name()
                self.states[full_name] = state
                
                # 如果是嵌套状态，添加到父状态的children中
                if state.parent:
                    state.parent.children[state_name] = state
                    state.parent.is_composite = True
            
            # 处理复合状态开始: "state statename {"
            elif line.startswith('state ') and line.endswith('{'):
                state_name = line[6:-1].strip()
                
                # 查找或创建复合状态
                if state_stack:
                    # 嵌套状态，需要完整名
                    parent_full = state_stack[-1].get_full_name()
                    full_name = f"{parent_full}_{state_name}" if parent_full else state_name
                else:
                    full_name = state_name
                
                if full_name not in self.states:
                    state = State(
                        name=state_name,
                        parent=state_stack[-1] if state_stack else None
                    )
                    self.states[full_name] = state
                
                # 压栈
                state_stack.append(self.states[full_name])
            
            # 处理复合状态结束: "}"
            elif line == '}':
                if state_stack:
                    state_stack.pop()
            
            # 处理状态转移: "state1 --> state2 : event/comment"
            elif '-->' in line and ':' in line:
                self._parse_transition(line, state_stack)
            
            # 处理初始状态: "[*] --> state"
            elif '[*] -->' in line:
                self._parse_initial_state(line, state_stack)
    
    def _parse_transition(self, line: str, state_stack: List[State]):
        """解析状态转移"""
        # 提取源状态和目标状态
        parts = line.split('-->')
        from_state = parts[0].strip()
        
        # 提取事件部分
        event_part = parts[1].split(':', 1)
        to_state = event_part[0].strip()
        
        if len(event_part) > 1:
            event_info = event_part[1].strip()
            # 事件可能带有注释: "event/comment"
            if '/' in event_info:
                event_name, event_comment = event_info.split('/', 1)
                event_name = event_name.strip()
                event_comment = event_comment.strip()
            else:
                event_name = event_info
                event_comment = ""
        else:
            event_name = ""
            event_comment = ""
        
        # 构建完整状态名（考虑当前上下文）
        from_full = self._resolve_state_name(from_state, state_stack)
        to_full = self._resolve_state_name(to_state, state_stack)
        
        # 记录事件
        if event_name and event_name not in self.events:
            self.events[event_name] = Event(name=event_name)
        
        # 记录转移
        transition = Transition(
            from_state=from_full,
            to_state=to_full,
            event=event_name,
            event_comment=event_comment
        )
        self.transitions.append(transition)
    
    def _parse_initial_state(self, line: str, state_stack: List[State]):
        """解析初始状态"""
        # 提取目标状态
        target_state = line.split('-->')[1].strip()
        
        if state_stack:
            # 嵌套初始状态
            parent_state = state_stack[-1]
            target_full = self._resolve_state_name(target_state, state_stack)
            
            # 设置父状态的初始子状态
            for state in self.states.values():
                if state.get_full_name() == target_full and state.parent == parent_state:
                    parent_state.initial_substate = state.name
                    break
        else:
            # 顶层初始状态
            target_full = self._resolve_state_name(target_state, state_stack)
            self.top_level_initial = target_full
    
    def _resolve_state_name(self, state_name: str, state_stack: List[State]) -> str:
        """解析状态名为完整名称"""
        if state_name == '[*]':
            return '[*]'
        
        # 如果状态名已经包含父状态（通过点号），直接转换
        if '.' in state_name:
            return state_name.replace('.', '_')
        
        # 在当前上下文中查找状态
        if state_stack:
            # 首先在父状态的子状态中查找
            parent = state_stack[-1]
            for child in parent.children.values():
                if child.name == state_name:
                    return child.get_full_name()
        
        # 在整个状态图中查找
        for state in self.states.values():
            if state.name == state_name:
                return state.get_full_name()
        
        # 如果没有找到，返回原始名称（可能是顶层状态）
        return state_name
    
    def assign_ids(self):
        """为状态和事件分配ID"""
        # 分配状态ID（从1开始）
        state_id = 1
        for state in self.states.values():
            state.id = state_id
            state_id += 1
        
        # 分配事件ID（从1开始）
        event_id = 1
        for event in self.events.values():
            event.id = event_id
            event_id += 1
    
    def get_state_transitions(self) -> Dict[str, List[Tuple[str, str, str]]]:
        """获取每个状态的转移表"""
        state_transitions = {}
        
        for transition in self.transitions:
            if transition.from_state == '[*]':
                continue  # 忽略初始状态转移
                
            if transition.from_state not in state_transitions:
                state_transitions[transition.from_state] = []
            
            # 查找目标状态ID
            to_state_id = 0
            for state in self.states.values():
                if state.get_full_name() == transition.to_state:
                    to_state_id = state.get_macro_name(self.project_name)
                    break
            
            # 查找事件ID
            event_id = 0
            if transition.event in self.events:
                event_id = f"{self.project_name.upper()}_{transition.event.upper()}"
            
            state_transitions[transition.from_state].append(
                (event_id, to_state_id, transition.event_comment)
            )
        
        return state_transitions
    
    def get_state_info(self) -> List[Dict]:
        """获取所有状态的信息"""
        state_info = []
        
        # 获取状态转移信息
        state_transitions = self.get_state_transitions()
        
        for state in self.states.values():
            # 确定默认状态ID
            if state.initial_substate:
                # 查找初始子状态
                for child in state.children.values():
                    if child.name == state.initial_substate:
                        default_id = child.get_macro_name(self.project_name)
                        break
                else:
                    default_id = state.get_macro_name(self.project_name)
            else:
                default_id = state.get_macro_name(self.project_name)
            
            # 确定父状态ID
            if state.parent:
                father_id = state.parent.get_macro_name(self.project_name)
            else:
                father_id = "BFX_STATUS_FATHER_NONE"
            
            # 获取转移表名
            trans_tbl_name = f"g_{self.project_name}_{state.get_full_name().lower()}_TransTbl"
            
            # 检查该状态是否有转移表
            state_full_name = state.get_full_name()
            empty_trans_tbl = state_full_name not in state_transitions or len(state_transitions[state_full_name]) == 0
            
            state_info.append({
                'state_id': state.get_macro_name(self.project_name),
                'default_id': default_id,
                'father_id': father_id,
                'trans_tbl_name': trans_tbl_name,
                'callback_name': state.get_callback_name(self.project_name),
                'empty_trans_tbl': empty_trans_tbl
            })
        
        return state_info

def generate_header_file(parser: PlantUMLParser, template_str: str) -> str:
    """生成.h文件"""
    template = Template(template_str)
    
    # 准备状态和事件宏定义
    state_macros = []
    for state in parser.states.values():
        state_macros.append({
            'name': state.get_macro_name(parser.project_name),
            'id': state.id,
            'comment': state.comment
        })
    
    event_macros = []
    for event in parser.events.values():
        event_macros.append({
            'name': f"{parser.project_name.upper()}_{event.name.upper()}",
            'id': event.id
        })
    state_info = parser.get_state_info()

    return template.render(
        project_name=parser.project_name,
        state_macros=state_macros,
        state_info=state_info,
        event_macros=event_macros,
        initial_state_macro=f"{parser.project_name.upper()}_INITIAL_STATE" if parser.top_level_initial else None,
        initial_state_id=next((s.id for s in parser.states.values() 
                             if s.get_full_name() == parser.top_level_initial), 0)
    )

def generate_source_file(parser: PlantUMLParser, template_str: str) -> str:
    """生成.c文件"""
    template = Template(template_str)
    
    # 获取状态转移表
    state_transitions = parser.get_state_transitions()
    
    # 准备转移表数据
    trans_tables = []
    for state_full_name, transitions in state_transitions.items():
        # 查找状态对象
        state_obj = None
        for state in parser.states.values():
            if state.get_full_name() == state_full_name:
                state_obj = state
                break
        
        if state_obj:
            trans_tables.append({
                'state_name': state_obj.get_full_name(),
                'macro_name': state_obj.get_macro_name(parser.project_name),
                'transitions': transitions,
                'trans_tbl_name': f"g_{parser.project_name}_{state_full_name.lower()}_TransTbl"
            })
    
    # 获取状态信息
    state_info = parser.get_state_info()
    
    return template.render(
        project_name=parser.project_name,
        trans_tables=trans_tables,
        initial_state_macro=f"{parser.project_name.upper()}_INITIAL_STATE" if parser.top_level_initial else None,
        state_info=state_info
    )

# Jinja2模板
HEADER_TEMPLATE = """/**
 * @file {{ project_name }}.h
 * @brief FSM of {{ project_name }}
 * @generator BufferFlowX
**/
#ifndef __BFX_{{ project_name.upper() }}_H__
#define __BFX_{{ project_name.upper() }}_H__

/* headers import ---------------------------------------------------------------------------------------------*/
#include "bfx_fsm.h"

/* state macros -----------------------------------------------------------------------------------------------*/
{% for macro in state_macros %}#define {{ macro.name }} {{ macro.id }}{% if macro.comment %} // {{ macro.comment }}{% endif %}
{% endfor %}
{% if initial_state_macro %}
#define {{ initial_state_macro }} {{ initial_state_id }}
{% endif %}
/* event macros -----------------------------------------------------------------------------------------------*/
{% for macro in event_macros %}#define {{ macro.name }} {{ macro.id }}
{% endfor %}
/* FSM generated ----------------------------------------------------------------------------------------------*/
extern BFX_FSM_HANDLE g_{{ project_name }}_fsmHandle;

/* state callback ---------------------------------------------------------------------------------------------*/
{% for state in state_info %}
__attribute__((weak)) void {{ state.callback_name }}(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize);{% endfor %}
#endif
"""

SOURCE_TEMPLATE = """/**
 * @file {{ project_name }}.c
 * @brief FSM of {{ project_name }}
 * @generator BufferFlowX
**/
#ifdef __cplusplus
extern "C" {
#endif
/* headers import ---------------------------------------------------------------------------------------------*/
#include <stddef.h>
#include "{{ project_name }}.h"

/* state callback ---------------------------------------------------------------------------------------------*/
{% for state in state_info %}
__attribute__((weak)) void {{ state.callback_name }}(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize)
{
}{% endfor %}

/* state table ------------------------------------------------------------------------------------------------*/
{% for table in trans_tables %}const BFX_FSM_TRAN_RECORD {{ table.trans_tbl_name }}[] = {
    {% for trans in table.transitions %}{ {{ trans[0] }}, {{ trans[1] }} },{% if trans[2] %} ///< {{ trans[2] }}{% endif %}
    {% endfor %}
};
{% endfor %}
const BFX_FSM_STATE g_{{ project_name }}_allstatus[] = {
    {% for state in state_info %}{
        {{ state.state_id }}, {{ state.default_id }}, {{ state.father_id }},
        {% if not state.empty_trans_tbl %}sizeof({{ state.trans_tbl_name }}) / sizeof(BFX_FSM_TRAN_RECORD), {{ state.trans_tbl_name }},{% else %}0, (BFX_FSM_TRAN_RECORD const *)NULL,{% endif %}
        {{ state.callback_name }}
    }{% if not loop.last %},{% endif %}
    {% endfor %}
};
/* FSM handle -------------------------------------------------------------------------------------------------*/
BFX_FSM_HANDLE g_{{ project_name }}_fsmHandle = {
    .stateTbl = g_{{ project_name }}_allstatus,
    .stateCnt = sizeof(g_{{ project_name }}_allstatus) / sizeof(BFX_FSM_STATE),
    .currentStateId = {{ initial_state_macro }},
};
#ifdef __cplusplus
}
#endif
"""

def main():
    """主函数"""
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("usage: python plantuml_to_c.py <plantuml_file> [output_dir]")
        sys.exit(1)
    
    # 获取输入文件路径
    plantuml_file = sys.argv[1]
    
    # 获取输出目录，默认为当前目录
    output_dir = sys.argv[2] if len(sys.argv) == 3 else "."
    
    # 确保输出目录存在
    import os
    os.makedirs(output_dir, exist_ok=True)
    
    # 读取PlantUML文件
    with open(plantuml_file, 'r', encoding='utf-8') as f:
        uml_content = f.read()
    
    # 解析PlantUML
    parser = PlantUMLParser()
    parser.parse(uml_content)
    parser.assign_ids()
    
    # 生成.h文件
    header_content = generate_header_file(parser, HEADER_TEMPLATE)
    header_path = os.path.join(output_dir, f"{parser.project_name}.h")
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    # 生成.c文件
    source_content = generate_source_file(parser, SOURCE_TEMPLATE)
    source_path = os.path.join(output_dir, f"{parser.project_name}.c")
    with open(source_path, 'w', encoding='utf-8') as f:
        f.write(source_content)
    
    print(f"-- FSM generated OK: {header_path} and {source_path}")

if __name__ == "__main__":
    main()