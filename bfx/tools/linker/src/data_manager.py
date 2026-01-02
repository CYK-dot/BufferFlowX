"""
数据管理模块 - 处理JSON数据结构和验证
"""
import json
import os
from datetime import datetime


class DataManager:
    """管理链接器段定义的数据"""
    
    def __init__(self, initial_data=None):
        """初始化数据管理器"""
        if initial_data is None:
            # 默认数据结构
            self.data = {
                "custom_sections": [],
                "component": "",  # 新增组件字段
                "metadata": {
                    "version": "1.0",
                    "created_at": datetime.now().isoformat(),
                    "description": "BufferFlowX Linker Script Configuration"
                },
                "output_paths": {
                    "linker_script": "",
                    "header_file": ""
                }
            }
        else:
            self.data = initial_data
        
        # 初始化项目路径
        self.current_project_path = None
    
    def validate_size_value(self, size_value):
        """验证大小值格式"""
        # 检查是否为有效的十六进制或十进制数字
        size_str = str(size_value).strip()
        
        # 检查十六进制格式
        if size_str.startswith("0x") or size_str.startswith("0X"):
            try:
                int(size_str, 16)
                return True
            except ValueError:
                return False, "无效的十六进制大小值"
        else:
            # 检查十进制格式
            try:
                int(size_str)
                return True, ""
            except ValueError:
                return False, "大小值应为有效的数字（十进制或十六进制）"
    
    def validate_address_value(self, address_value):
        """验证地址值格式"""
        # 地址可以是十六进制或十进制，也可以是表达式
        addr_str = str(address_value).strip()
        
        # 允许表达式，如 "0x20000000 + 0x1000"
        # 简单检查：允许字母、数字、空格、+、-、*、/、() 等
        valid_chars = set("0123456789abcdefABCDEFxX +-*/()")
        for char in addr_str:
            if char not in valid_chars:
                return False, f"地址值包含无效字符: {char}"
        
        return True, ""
    
    def add_section(self, section_data):
        """添加新段"""
        # 检查段名是否已存在
        for i, existing_section in enumerate(self.data["custom_sections"]):
            if existing_section["name"] == section_data["name"]:
                return False, f"段名 '{section_data['name']}' 已存在"
        
        # 添加新段
        self.data["custom_sections"].append(section_data)
        return True, f"已添加新段: {section_data['name']}"
    
    def update_section(self, index, section_data):
        """更新现有段"""
        if 0 <= index < len(self.data["custom_sections"]):
            self.data["custom_sections"][index] = section_data
            return True, f"已更新段: {section_data['name']}"
        return False, "无效的段索引"
    
    def delete_section(self, index):
        """删除指定索引的段"""
        if 0 <= index < len(self.data["custom_sections"]):
            section_name = self.data["custom_sections"][index]["name"]
            del self.data["custom_sections"][index]
            return True, f"已删除段: {section_name}"
        return False, "无效的段索引"
    
    def move_section_up(self, index):
        """将段上移"""
        if 0 < index < len(self.data["custom_sections"]):
            self.data["custom_sections"][index], self.data["custom_sections"][index-1] = \
                self.data["custom_sections"][index-1], self.data["custom_sections"][index]
            return True, f"已上移段: {self.data['custom_sections'][index]['name']}"
        return False, "无法移动段"
    
    def move_section_down(self, index):
        """将段下移"""
        if 0 <= index < len(self.data["custom_sections"]) - 1:
            self.data["custom_sections"][index], self.data["custom_sections"][index+1] = \
                self.data["custom_sections"][index+1], self.data["custom_sections"][index]
            return True, f"已下移段: {self.data['custom_sections'][index+1]['name']}"
        return False, "无法移动段"
    
    def load_from_json(self, filepath):
        """从JSON文件加载数据"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                loaded_data = json.load(f)
            
            # 确保数据结构完整
            if "custom_sections" not in loaded_data:
                loaded_data["custom_sections"] = []
            if "metadata" not in loaded_data:
                loaded_data["metadata"] = {}
            if "output_paths" not in loaded_data:
                loaded_data["output_paths"] = {
                    "linker_script": "",
                    "header_file": ""
                }
            
            self.data = loaded_data
            self.current_project_path = filepath
            return True, f"项目已加载: {os.path.basename(filepath)}"
        except Exception as e:
            return False, f"加载项目失败: {str(e)}"
    
    def save_to_json(self, filepath):
        """保存数据到JSON文件"""
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(self.data, f, indent=2, ensure_ascii=False)
            
            self.current_project_path = filepath
            return True, f"项目已保存: {os.path.basename(filepath)}"
        except Exception as e:
            return False, f"保存项目失败: {str(e)}"
    
    def get_sections_list(self):
        """获取段列表"""
        return [section["name"] for section in self.data["custom_sections"]]
    
    def get_section_by_index(self, index):
        """根据索引获取段数据"""
        if 0 <= index < len(self.data["custom_sections"]):
            return self.data["custom_sections"][index].copy()
        return None

    def reset_data(self):
        """重置数据为默认值"""
        self.data = {
            "custom_sections": [],
            "component": "",  # 重置组件字段
            "metadata": {
                "version": "1.0",
                "created_at": datetime.now().isoformat(),
                "description": "BufferFlowX Linker Script Configuration"
            },
            "output_paths": {
                "linker_script": "",
                "header_file": ""
            }
        }
        self.current_project_path = None