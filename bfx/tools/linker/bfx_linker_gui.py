import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import json
import os
import re
import sys
from datetime import datetime
from jinja2 import Environment, FileSystemLoader, Template, select_autoescape

class JSONSectionEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("JSON段定义编辑器")
        self.root.geometry("1000x700")
        
        # 初始化数据结构
        self.data = {
            "custom_sections": [],
            "metadata": {}
        }
        
        # 当前编辑的段索引
        self.current_index = None
        
        # 链接器脚本类型
        self.linker_type = tk.StringVar(value="gcc")
        
        # 模板环境
        self.template_env = None
        self.templates = {}
        
        # 初始化模板
        self.init_templates()
        
        self.setup_ui()
    
    def init_templates(self):
        """初始化Jinja2模板环境 - 仅使用外置模板"""
        try:
            # 尝试从当前目录加载模板
            current_dir = os.path.dirname(os.path.abspath(__file__))
            template_dir = os.path.join(current_dir, "templates")
            
            # 如果templates目录不存在，尝试在当前目录查找模板文件
            if not os.path.exists(template_dir):
                template_dir = current_dir
            
            # 创建Jinja2环境
            self.template_env = Environment(
                loader=FileSystemLoader(template_dir),
                autoescape=select_autoescape(['j2']),
                trim_blocks=False,
                lstrip_blocks=False
            )
            
            # 加载模板文件 - 如果加载失败，程序将无法正常运行
            gcc_template = self.load_template("bfx_ld_template_gcc.j2")
            keil_template = self.load_template("bfx_ld_template_armlink.j2")
            
            if not gcc_template or not keil_template:
                missing_templates = []
                if not gcc_template:
                    missing_templates.append("bfx_ld_template_gcc.j2")
                if not keil_template:
                    missing_templates.append("bfx_ld_template_armlink.j2")
                
                error_msg = f"无法加载必需的模板文件:\n{', '.join(missing_templates)}\n\n"
                error_msg += f"请确保模板文件位于以下目录之一:\n1. {template_dir}/\n2. {current_dir}/\n\n"
                error_msg += "注意: 内置模板已被移除，必须使用外置模板文件。"
                
                messagebox.showerror("模板加载错误", error_msg)
                sys.exit(1)  # 退出程序
            
            self.templates = {
                "gcc": gcc_template,
                "keil": keil_template
            }
            
        except Exception as e:
            error_msg = f"初始化模板环境失败:\n{str(e)}\n\n"
            error_msg += "请确保已安装Jinja2库并正确配置模板文件。"
            messagebox.showerror("模板错误", error_msg)
            sys.exit(1)
    
    def load_template(self, template_name):
        """从文件加载模板，如果失败则返回None"""
        try:
            template = self.template_env.get_template(template_name)
            return template
        except Exception as e:
            print(f"错误: 无法加载模板 {template_name}: {e}")
            return None
    
    def setup_ui(self):
        # 主布局 - 使用Notebook实现选项卡
        notebook = ttk.Notebook(self.root)
        notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 第一页：段编辑器
        editor_frame = ttk.Frame(notebook)
        notebook.add(editor_frame, text="段编辑器")
        
        # 第二页：链接器脚本生成器
        linker_frame = ttk.Frame(notebook)
        notebook.add(linker_frame, text="链接器脚本")
        
        # 设置段编辑器页面
        self.setup_editor_tab(editor_frame)
        
        # 设置链接器脚本页面
        self.setup_linker_tab(linker_frame)
    
    def setup_editor_tab(self, parent):
        # 段编辑器布局
        main_pane = tk.PanedWindow(parent, orient=tk.HORIZONTAL, sashrelief=tk.RAISED)
        main_pane.pack(fill=tk.BOTH, expand=True)
        
        # 左侧：段列表
        left_frame = tk.Frame(main_pane)
        main_pane.add(left_frame, width=300)
        
        # 段列表控件
        tk.Label(left_frame, text="段列表", font=("Arial", 12, "bold")).pack(pady=(5, 10))
        
        self.section_listbox = tk.Listbox(left_frame, height=20)
        self.section_listbox.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.section_listbox.bind('<<ListboxSelect>>', self.on_section_select)
        
        # 段操作按钮
        btn_frame = tk.Frame(left_frame)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(btn_frame, text="添加段", command=self.add_section, width=10).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_frame, text="删除段", command=self.delete_section, width=10).pack(side=tk.LEFT, padx=2)
        
        # 文件操作按钮
        file_frame = tk.Frame(left_frame)
        file_frame.pack(fill=tk.X, padx=5, pady=(10, 5))
        
        tk.Button(file_frame, text="加载JSON", command=self.load_json, width=12).pack(side=tk.LEFT, padx=2)
        tk.Button(file_frame, text="保存JSON", command=self.save_json, width=12).pack(side=tk.LEFT, padx=2)
        
        # 右侧：段编辑器
        right_frame = tk.Frame(main_pane)
        main_pane.add(right_frame)
        
        # 编辑表单
        form_frame = tk.LabelFrame(right_frame, text="段属性编辑", padx=10, pady=10)
        form_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 使用网格布局创建表单
        row = 0
        
        # 段名称（必需）
        tk.Label(form_frame, text="段名称(必填):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.name_entry = tk.Entry(form_frame, width=30)
        self.name_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 内存区域（必需）
        tk.Label(form_frame, text="内存区域(必填):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.memory_region_entry = tk.Entry(form_frame, width=30)
        self.memory_region_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 加载区域
        tk.Label(form_frame, text="加载区域(选填):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.load_region_entry = tk.Entry(form_frame, width=30)
        self.load_region_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 对齐方式
        tk.Label(form_frame, text="对齐方式(选填):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.alignment_entry = tk.Entry(form_frame, width=30)
        self.alignment_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 最大大小和固定大小（互斥）
        size_frame = tk.Frame(form_frame)
        size_frame.grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        tk.Label(size_frame, text="大小限制(选填):").pack(side=tk.LEFT)
        
        # 使用单选按钮确保互斥
        self.size_var = tk.StringVar(value="none")
        
        tk.Radiobutton(size_frame, text="无", variable=self.size_var, 
                      value="none", command=self.on_size_type_change).pack(side=tk.LEFT, padx=10)
        
        tk.Radiobutton(size_frame, text="最大大小", variable=self.size_var, 
                      value="max", command=self.on_size_type_change).pack(side=tk.LEFT, padx=5)
        
        tk.Radiobutton(size_frame, text="固定大小", variable=self.size_var, 
                      value="fixed", command=self.on_size_type_change).pack(side=tk.LEFT, padx=5)
        
        row += 1
        
        # 大小输入框
        self.size_entry = tk.Entry(form_frame, width=30, state=tk.DISABLED)
        self.size_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 起始地址（字符串存储）
        tk.Label(form_frame, text="起始地址(选填):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.start_address_entry = tk.Entry(form_frame, width=30)
        self.start_address_entry.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # 保存/取消按钮
        button_frame = tk.Frame(form_frame)
        button_frame.grid(row=row, column=0, columnspan=2, pady=20)
        
        tk.Button(button_frame, text="保存段", command=self.save_section, 
                 width=15, bg="#4CAF50", fg="white").pack(side=tk.LEFT, padx=5)
        tk.Button(button_frame, text="取消编辑", command=self.cancel_edit,
                 width=15).pack(side=tk.LEFT, padx=5)
        
        # JSON预览区域
        preview_frame = tk.LabelFrame(right_frame, text="JSON预览", padx=10, pady=10)
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.json_text = scrolledtext.ScrolledText(preview_frame, height=10, wrap=tk.NONE)
        self.json_text.pack(fill=tk.BOTH, expand=True)
        
        # 初始化预览
        self.update_preview()
    
    def setup_linker_tab(self, parent):
        # 链接器脚本生成器布局
        control_frame = tk.Frame(parent)
        control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # 链接器类型选择
        tk.Label(control_frame, text="链接器类型:").pack(side=tk.LEFT, padx=(0, 5))
        
        linker_type_frame = tk.Frame(control_frame)
        linker_type_frame.pack(side=tk.LEFT)
        
        # 单选按钮选择链接器类型
        tk.Radiobutton(linker_type_frame, text="GCC/Clang (ld)", 
                      variable=self.linker_type, value="gcc",
                      command=self.generate_linker_script).pack(side=tk.LEFT, padx=5)
        
        tk.Radiobutton(linker_type_frame, text="Keil (armlink)", 
                      variable=self.linker_type, value="keil",
                      command=self.generate_linker_script).pack(side=tk.LEFT, padx=5)
        
        # 生成和保存按钮
        tk.Button(control_frame, text="重新生成", command=self.generate_linker_script,
                 width=10).pack(side=tk.LEFT, padx=10)
        
        tk.Button(control_frame, text="保存脚本", command=self.save_linker_script,
                 width=10, bg="#2196F3", fg="white").pack(side=tk.LEFT, padx=5)
        
        # 添加一个复制按钮
        tk.Button(control_frame, text="复制到剪贴板", command=self.copy_to_clipboard,
                 width=15).pack(side=tk.LEFT, padx=5)
        
        # 脚本预览区域
        preview_frame = tk.LabelFrame(parent, text="链接器脚本预览", padx=10, pady=10)
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.linker_text = scrolledtext.ScrolledText(preview_frame, height=20, wrap=tk.NONE)
        self.linker_text.pack(fill=tk.BOTH, expand=True)
        
        # 生成默认的链接器脚本
        self.generate_linker_script()
    
    def on_size_type_change(self):
        """处理大小类型变化"""
        size_type = self.size_var.get()
        if size_type == "none":
            self.size_entry.delete(0, tk.END)
            self.size_entry.config(state=tk.DISABLED)
        else:
            self.size_entry.config(state=tk.NORMAL)
    
    def add_section(self):
        """添加新段"""
        self.current_index = None
        self.clear_form()
        self.name_entry.focus()
    
    def delete_section(self):
        """删除选中的段"""
        selection = self.section_listbox.curselection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个段")
            return
        
        if messagebox.askyesno("确认", "确定要删除这个段吗？"):
            index = selection[0]
            del self.data["custom_sections"][index]
            self.update_list()
            self.update_preview()
            self.clear_form()
            # 重新生成链接器脚本
            self.generate_linker_script()
    
    def on_section_select(self, event):
        """当选择段列表中的项时"""
        selection = self.section_listbox.curselection()
        if not selection:
            return
        
        self.current_index = selection[0]
        section = self.data["custom_sections"][self.current_index]
        self.load_section_to_form(section)
    
    def load_section_to_form(self, section):
        """加载段数据到表单"""
        self.clear_form()
        
        self.name_entry.insert(0, section.get("name", ""))
        self.memory_region_entry.insert(0, section.get("memory_region", ""))
        self.load_region_entry.insert(0, section.get("load_region", ""))
        self.alignment_entry.insert(0, section.get("alignment", ""))
        
        # 处理大小字段（互斥）
        if "max_size" in section:
            self.size_var.set("max")
            self.size_entry.insert(0, section["max_size"])
            self.size_entry.config(state=tk.NORMAL)
        elif "fixed_size" in section:
            self.size_var.set("fixed")
            self.size_entry.insert(0, section["fixed_size"])
            self.size_entry.config(state=tk.NORMAL)
        else:
            self.size_var.set("none")
            self.size_entry.config(state=tk.DISABLED)
        
        self.start_address_entry.insert(0, section.get("start_address", ""))
    
    def save_section(self):
        """保存当前编辑的段"""
        # 验证必需字段
        name = self.name_entry.get().strip()
        memory_region = self.memory_region_entry.get().strip()
        
        if not name:
            messagebox.showerror("错误", "段名称是必需字段")
            return
        
        if not memory_region:
            messagebox.showerror("错误", "内存区域是必需字段")
            return
        
        # 构建段数据
        section = {
            "name": name,
            "memory_region": memory_region
        }
        
        # 添加可选字段
        load_region = self.load_region_entry.get().strip()
        if load_region:
            section["load_region"] = load_region
        
        alignment = self.alignment_entry.get().strip()
        if alignment:
            # 尝试转换为整数，但保持字符串格式
            try:
                int(alignment)
                section["alignment"] = alignment
            except ValueError:
                messagebox.showwarning("警告", "对齐方式应为整数")
                return
        
        # 处理大小字段（互斥）
        size_type = self.size_var.get()
        size_value = self.size_entry.get().strip()
        
        if size_type == "max" and size_value:
            # 验证大小格式
            if not self.validate_size_value(size_value):
                return
            section["max_size"] = size_value
        elif size_type == "fixed" and size_value:
            # 验证大小格式
            if not self.validate_size_value(size_value):
                return
            section["fixed_size"] = size_value
        
        # 添加起始地址
        start_address = self.start_address_entry.get().strip()
        if start_address:
            # 验证地址格式
            if not self.validate_address_value(start_address):
                return
            section["start_address"] = start_address
        
        # 更新数据
        if self.current_index is not None:
            # 更新现有段
            self.data["custom_sections"][self.current_index] = section
        else:
            # 添加新段
            self.data["custom_sections"].append(section)
        
        # 更新UI
        self.update_list()
        self.update_preview()
        self.generate_linker_script()  # 重新生成链接器脚本
        # 成功时不显示提示框，仅在失败时提示
    
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
                messagebox.showerror("错误", "无效的十六进制大小值")
                return False
        else:
            # 检查十进制格式
            try:
                int(size_str)
                return True
            except ValueError:
                messagebox.showerror("错误", "大小值应为有效的数字（十进制或十六进制）")
                return False
    
    def validate_address_value(self, address_value):
        """验证地址值格式"""
        # 地址可以是十六进制或十进制，也可以是表达式
        addr_str = str(address_value).strip()
        
        # 允许表达式，如 "0x20000000 + 0x1000"
        # 简单检查：允许字母、数字、空格、+、-、*、/、() 等
        valid_chars = set("0123456789abcdefABCDEFxX +-*/()")
        for char in addr_str:
            if char not in valid_chars:
                messagebox.showerror("错误", f"地址值包含无效字符: {char}")
                return False
        
        return True
    
    def clear_form(self):
        """清空表单"""
        self.current_index = None
        self.name_entry.delete(0, tk.END)
        self.memory_region_entry.delete(0, tk.END)
        self.load_region_entry.delete(0, tk.END)
        self.alignment_entry.delete(0, tk.END)
        self.size_var.set("none")
        self.size_entry.delete(0, tk.END)
        self.size_entry.config(state=tk.DISABLED)
        self.start_address_entry.delete(0, tk.END)
    
    def cancel_edit(self):
        """取消编辑"""
        self.clear_form()
        self.section_listbox.selection_clear(0, tk.END)
    
    def update_list(self):
        """更新段列表"""
        self.section_listbox.delete(0, tk.END)
        for section in self.data["custom_sections"]:
            self.section_listbox.insert(tk.END, section["name"])
    
    def update_preview(self):
        """更新JSON预览"""
        self.json_text.delete(1.0, tk.END)
        
        # 格式化JSON
        formatted_json = json.dumps(self.data, indent=2, ensure_ascii=False)
        self.json_text.insert(1.0, formatted_json)
    
    def generate_linker_script(self):
        """使用Jinja2模板生成链接器脚本"""
        self.linker_text.delete(1.0, tk.END)
        
        linker_type = self.linker_type.get()
        template = self.templates.get(linker_type)
        
        if not template:
            error_msg = f"找不到 {linker_type} 类型的模板\n\n"
            error_msg += f"请确保以下模板文件存在:\n"
            if linker_type == "gcc":
                error_msg += "  - bfx_ld_template_gcc.j2\n"
            else:
                error_msg += "  - bfx_ld_template_armlink.j2\n"
            error_msg += "\n模板文件应位于当前目录或templates/子目录中。"
            
            self.linker_text.insert(1.0, error_msg)
            return
        
        # 准备模板上下文
        context = {
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "custom_sections": self.data["custom_sections"]
        }
        
        try:
            # 使用Jinja2模板渲染
            script = template.render(**context)
            
            self.linker_text.insert(1.0, script)
            
            # 添加语法高亮
            self.highlight_linker_script(linker_type)
        except Exception as e:
            error_msg = f"生成链接器脚本时出错:\n{str(e)}"
            self.linker_text.insert(1.0, error_msg)
            messagebox.showerror("错误", error_msg)
    
    def highlight_linker_script(self, linker_type):
        """高亮链接器脚本语法"""
        # 清除现有标签
        for tag in self.linker_text.tag_names():
            self.linker_text.tag_remove(tag, "1.0", tk.END)
        
        # 定义标签样式
        if linker_type == "gcc":
            self.linker_text.tag_config("comment", foreground="green")
            self.linker_text.tag_config("directive", foreground="blue", font=("Courier", 10, "bold"))
            self.linker_text.tag_config("section", foreground="darkred", font=("Courier", 10, "bold"))
            self.linker_text.tag_config("symbol", foreground="purple")
            self.linker_text.tag_config("number", foreground="darkorange")
        else:  # keil
            self.linker_text.tag_config("comment", foreground="green")
            self.linker_text.tag_config("directive", foreground="blue", font=("Courier", 10, "bold"))
            self.linker_text.tag_config("section", foreground="darkred", font=("Courier", 10, "bold"))
            self.linker_text.tag_config("address", foreground="darkorange")
        
        # 获取文本内容
        content = self.linker_text.get("1.0", tk.END)
        
        # 根据链接器类型应用不同的高亮规则
        if linker_type == "gcc":
            # 高亮GCC注释
            for match in re.finditer(r"/\*.*?\*/", content, re.DOTALL):
                start = f"1.0+{match.start()}c"
                end = f"1.0+{match.end()}c"
                self.linker_text.tag_add("comment", start, end)
            
            # 高亮GCC指令和符号
            gcc_keywords = ["SECTIONS", "ALIGN", "LOADADDR", "SIZEOF", "AT>", "KEEP", "FILL", "ASSERT"]
            for keyword in gcc_keywords:
                for match in re.finditer(r'\b' + re.escape(keyword) + r'\b', content):
                    start = f"1.0+{match.start()}c"
                    end = f"1.0+{match.end()}c"
                    self.linker_text.tag_add("directive", start, end)
            
            # 高亮段名（在冒号前）
            for match in re.finditer(r'^\s*(\.[\w\.]+)\s*:', content, re.MULTILINE):
                start = f"1.0+{match.start(1)}c"
                end = f"1.0+{match.end(1)}c"
                self.linker_text.tag_add("section", start, end)
            
            # 高亮符号定义
            for match in re.finditer(r'(__[\w]+__(?:_start__|_end__|_lma_start__|_lma_end__|_remaining__))', content):
                start = f"1.0+{match.start(1)}c"
                end = f"1.0+{match.end(1)}c"
                self.linker_text.tag_add("symbol", start, end)
            
            # 高亮数字（十六进制和十进制）
            for match in re.finditer(r'\b(0x[0-9A-Fa-f]+|\d+)\b', content):
                start = f"1.0+{match.start()}c"
                end = f"1.0+{match.end()}c"
                self.linker_text.tag_add("number", start, end)
        
        else:  # keil
            # 高亮Keil注释
            for match in re.finditer(r';.*$', content, re.MULTILINE):
                start = f"1.0+{match.start()}c"
                end = f"1.0+{match.end()}c"
                self.linker_text.tag_add("comment", start, end)
            
            # 高亮Keil指令
            keil_keywords = ["LOAD", "MAX_SIZE", "SIZE", "ALIGN"]
            for keyword in keil_keywords:
                for match in re.finditer(r'\b' + re.escape(keyword) + r'\b', content):
                    start = f"1.0+{match.start()}c"
                    end = f"1.0+{match.end()}c"
                    self.linker_text.tag_add("directive", start, end)
            
            # 高亮段名
            for match in re.finditer(r'^\s*(\.[\w\.]+)\s+', content, re.MULTILINE):
                start = f"1.0+{match.start(1)}c"
                end = f"1.0+{match.end(1)}c"
                self.linker_text.tag_add("section", start, end)
            
            # 高亮地址
            for match in re.finditer(r'\b(0x[0-9A-Fa-f]+|\+0|\+\d+)\b', content):
                start = f"1.0+{match.start()}c"
                end = f"1.0+{match.end()}c"
                self.linker_text.tag_add("address", start, end)
    
    def copy_to_clipboard(self):
        """复制链接器脚本到剪贴板"""
        script = self.linker_text.get(1.0, tk.END).strip()
        if script:
            self.root.clipboard_clear()
            self.root.clipboard_append(script)
            # 成功时不显示提示框，仅在失败时提示
    
    def save_linker_script(self):
        """保存链接器脚本到文件"""
        script = self.linker_text.get(1.0, tk.END).strip()
        if not script:
            messagebox.showwarning("警告", "没有可保存的脚本内容")
            return
        
        # 根据链接器类型确定默认文件扩展名
        linker_type = self.linker_type.get()
        default_ext = ".ld" if linker_type == "gcc" else ".sct"
        
        if linker_type == "gcc":
            filetypes = [("GCC Linker Script", "*.ld"), ("All files", "*.*")]
        else:
            filetypes = [("Keil Scatter File", "*.sct"), ("All files", "*.*")]
        
        filepath = filedialog.asksaveasfilename(
            title="保存链接器脚本",
            defaultextension=default_ext,
            filetypes=filetypes
        )
        
        if not filepath:
            return
        
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(script)
            # 成功时不显示提示框，仅在失败时提示
        except Exception as e:
            messagebox.showerror("错误", f"保存文件失败: {str(e)}")
    
    def load_json(self):
        """加载JSON文件"""
        filepath = filedialog.askopenfilename(
            title="选择JSON文件",
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        
        if not filepath:
            return
        
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                self.data = json.load(f)
            
            # 确保数据结构完整
            if "custom_sections" not in self.data:
                self.data["custom_sections"] = []
            if "metadata" not in self.data:
                self.data["metadata"] = {}
            
            self.update_list()
            self.update_preview()
            self.generate_linker_script()  # 重新生成链接器脚本
            # 成功时不显示提示框，仅在失败时提示
        except Exception as e:
            messagebox.showerror("错误", f"加载文件失败: {str(e)}")
    
    def save_json(self):
        """保存JSON文件"""
        filepath = filedialog.asksaveasfilename(
            title="保存JSON文件",
            defaultextension=".json",
            filetypes=[("JSON文件", "*.json"), ("所有文件", "*.*")]
        )
        
        if not filepath:
            return
        
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(self.data, f, indent=2, ensure_ascii=False)
            # 成功时不显示提示框，仅在失败时提示
        except Exception as e:
            messagebox.showerror("错误", f"保存文件失败: {str(e)}")

def main():
    root = tk.Tk()
    app = JSONSectionEditor(root)
    root.mainloop()

if __name__ == "__main__":
    main()