"""
UI组件模块 - 包含GUI界面组件
"""
import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import re
import os


class SectionEditorFrame(ttk.Frame):
    """段编辑器组件"""
    
    def __init__(self, parent, data_manager, template_handler, on_update_callback=None):
        super().__init__(parent)
        self.data_manager = data_manager
        self.template_handler = template_handler
        self.on_update_callback = on_update_callback
        
        # 当前编辑的段索引
        self.current_index = None
        
        self.setup_ui()
    
    def setup_ui(self):
        """设置UI界面"""
        # 主布局 - 使用PanedWindow分割区域
        main_pane = tk.PanedWindow(self, orient=tk.VERTICAL, sashrelief=tk.RAISED)
        main_pane.pack(fill=tk.BOTH, expand=True)
        
        # 上部：段列表和编辑器
        editor_area = tk.Frame(main_pane)
        main_pane.add(editor_area, height=370)
        
        # 下部：生成功能区域
        generation_area = tk.Frame(main_pane)
        main_pane.add(generation_area, height=380)
        
        # 设置编辑器区域
        self.setup_editor_area(editor_area)
        
        # 设置生成功能区域
        self.setup_generation_area(generation_area)
    
    def setup_editor_area(self, parent):
        """设置编辑器区域"""
        # 左侧：段列表
        list_frame = tk.LabelFrame(parent, text="段列表", padx=10, pady=10)
        list_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.section_listbox = tk.Listbox(list_frame, height=15)
        self.section_listbox.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.section_listbox.bind('<<ListboxSelect>>', self.on_section_select)
        
        # 段操作按钮
        btn_frame = tk.Frame(list_frame)
        btn_frame.pack(fill=tk.X, padx=5, pady=(5, 0))
        
        tk.Button(btn_frame, text="添加段", command=self.add_section, width=10).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_frame, text="删除段", command=self.delete_section, width=10).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_frame, text="上移", command=self.move_section_up, width=8).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_frame, text="下移", command=self.move_section_down, width=8).pack(side=tk.LEFT, padx=2)
        
        # 右侧：编辑表单
        form_frame = tk.LabelFrame(parent, text="段属性编辑", padx=10, pady=10)
        form_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
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
        
        # 状态标签
        row += 1
        self.status_label = tk.Label(form_frame, text="准备就绪", fg="gray")
        self.status_label.grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=5)
    
    def setup_generation_area(self, parent):
        """设置生成功能区域"""
        # 生成控制区域
        control_frame = tk.LabelFrame(parent, text="生成控制", padx=10, pady=10)
        control_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 组件名称输入
        component_frame = tk.Frame(control_frame)
        component_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Label(component_frame, text="组件名称:").pack(side=tk.LEFT, padx=(0, 10))
        self.component_entry = tk.Entry(component_frame, width=30)
        self.component_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        # 绑定失去焦点事件，自动保存组件名称
        self.component_entry.bind('<FocusOut>', self._on_component_change)
        
        # 链接器类型选择
        type_frame = tk.Frame(control_frame)
        type_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Label(type_frame, text="链接器类型:").pack(side=tk.LEFT, padx=(0, 10))
        
        # 单选按钮选择链接器类型
        self.linker_type = tk.StringVar(value="gcc")
        tk.Radiobutton(type_frame, text="GCC/Clang (ld)", 
                      variable=self.linker_type, value="gcc").pack(side=tk.LEFT, padx=5)
        
        tk.Radiobutton(type_frame, text="Keil (armlink)", 
                      variable=self.linker_type, value="keil").pack(side=tk.LEFT, padx=5)
        
        # 使用PanedWindow创建左右两栏布局
        paned_window = tk.PanedWindow(control_frame, orient=tk.HORIZONTAL, sashrelief=tk.RAISED)
        paned_window.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 左栏：路径输入区域
        left_frame = tk.LabelFrame(paned_window, text="路径设置", padx=10, pady=5)
        paned_window.add(left_frame, stretch="always")
        
        # 链接器脚本路径输入
        linker_path_input_frame = tk.Frame(left_frame)
        linker_path_input_frame.pack(fill=tk.X, pady=2)
        
        tk.Label(linker_path_input_frame, text="链接器脚本路径:", width=15, anchor=tk.W).pack(side=tk.LEFT)
        self.linker_path_entry = tk.Entry(linker_path_input_frame, width=40)
        self.linker_path_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        tk.Button(linker_path_input_frame, text="浏览", command=self.browse_linker_path,
                 width=8).pack(side=tk.RIGHT, padx=2)
        
        # 头文件路径输入
        header_path_input_frame = tk.Frame(left_frame)
        header_path_input_frame.pack(fill=tk.X, pady=2)
        
        tk.Label(header_path_input_frame, text="头文件路径:", width=15, anchor=tk.W).pack(side=tk.LEFT)
        self.header_path_entry = tk.Entry(header_path_input_frame, width=40)
        self.header_path_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        tk.Button(header_path_input_frame, text="浏览", command=self.browse_header_path,
                 width=8).pack(side=tk.RIGHT, padx=2)
        
        # 右栏：当前文件状态显示
        right_frame = tk.LabelFrame(paned_window, text="当前文件状态", padx=10, pady=5)
        paned_window.add(right_frame, stretch="always")
        
        # 当前链接器脚本路径标签
        current_linker_frame = tk.Frame(right_frame)
        current_linker_frame.pack(fill=tk.X, pady=2)
        tk.Label(current_linker_frame, text="当前脚本:", width=15, anchor=tk.W).pack(side=tk.LEFT)
        self.current_linker_label = tk.Label(current_linker_frame, text="未保存", fg="gray")
        self.current_linker_label.pack(side=tk.LEFT)
        
        # 当前头文件路径标签
        current_header_frame = tk.Frame(right_frame)
        current_header_frame.pack(fill=tk.X, pady=2)
        tk.Label(current_header_frame, text="当前头文件:", width=15, anchor=tk.W).pack(side=tk.LEFT)
        self.current_header_label = tk.Label(current_header_frame, text="未保存", fg="gray")
        self.current_header_label.pack(side=tk.LEFT)
        
        # 生成按钮
        btn_frame = tk.Frame(control_frame)
        btn_frame.pack(fill=tk.X, padx=5, pady=10)
        
        tk.Button(btn_frame, text="更新/生成 文件", command=self.update_generate_files,
                 width=25, bg="#4CAF50", fg="white").pack(side=tk.LEFT, padx=5)
    
    def move_section_up(self):
        """将选中的段上移"""
        selection = self.section_listbox.curselection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个段")
            return
        
        index = selection[0]
        success, msg = self.data_manager.move_section_up(index)
        if success:
            self.update_list()
            # 重新选中移动后的项
            self.section_listbox.selection_set(index-1)
            self.current_index = index-1
            self.status_label.config(text=msg, fg="green")
            # 如果有回调函数，调用它来重新生成内容
            if self.on_update_callback:
                self.on_update_callback()
        else:
            messagebox.showerror("错误", msg)
    
    def move_section_down(self):
        """将选中的段下移"""
        selection = self.section_listbox.curselection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个段")
            return
        
        index = selection[0]
        success, msg = self.data_manager.move_section_down(index)
        if success:
            self.update_list()
            # 重新选中移动后的项
            self.section_listbox.selection_set(index+1)
            self.current_index = index+1
            self.status_label.config(text=msg, fg="green")
            # 如果有回调函数，调用它来重新生成内容
            if self.on_update_callback:
                self.on_update_callback()
        else:
            messagebox.showerror("错误", msg)
    
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
        self.status_label.config(text="正在添加新段", fg="blue")
        self.name_entry.focus()
    
    def delete_section(self):
        """删除选中的段"""
        selection = self.section_listbox.curselection()
        if not selection:
            messagebox.showwarning("警告", "请先选择一个段")
            return
        
        index = selection[0]
        section_name = self.data_manager.data["custom_sections"][index]["name"]
        
        if messagebox.askyesno("确认", f"确定要删除段 '{section_name}' 吗？"):
            success, msg = self.data_manager.delete_section(index)
            if success:
                self.update_list()
                self.clear_form()
                self.status_label.config(text=msg, fg="red")
                # 如果有回调函数，调用它来重新生成内容
                if self.on_update_callback:
                    self.on_update_callback()
            else:
                messagebox.showerror("错误", msg)
    
    def on_section_select(self, event):
        """当选择段列表中的项时"""
        selection = self.section_listbox.curselection()
        if not selection:
            return
        
        self.current_index = selection[0]
        section = self.data_manager.get_section_by_index(self.current_index)
        if section:
            self.load_section_to_form(section)
            self.status_label.config(text=f"正在编辑段: {section['name']}", fg="blue")
    
    def load_section_to_form(self, section):
        """加载段数据到表单"""
        self.clear_form()
        
        # 更新组件名称显示（全局组件名称）
        self._update_component_display()
        
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
            is_valid, error_msg = self.data_manager.validate_size_value(size_value)
            if not is_valid:
                messagebox.showerror("错误", error_msg)
                return
            section["max_size"] = size_value
        elif size_type == "fixed" and size_value:
            # 验证大小格式
            is_valid, error_msg = self.data_manager.validate_size_value(size_value)
            if not is_valid:
                messagebox.showerror("错误", error_msg)
                return
            section["fixed_size"] = size_value
        
        # 添加起始地址
        start_address = self.start_address_entry.get().strip()
        if start_address:
            # 验证地址格式
            is_valid, error_msg = self.data_manager.validate_address_value(start_address)
            if not is_valid:
                messagebox.showerror("错误", error_msg)
                return
            section["start_address"] = start_address
        
        # 保存段
        if self.current_index is None:
            # 添加新段
            success, msg = self.data_manager.add_section(section)
            if success:
                self.current_index = len(self.data_manager.data["custom_sections"]) - 1
                action_text = "已添加新段"
            else:
                # 检查是否要替换重复的段
                if "已存在" in msg:
                    existing_idx = -1
                    for i, existing_section in enumerate(self.data_manager.data["custom_sections"]):
                        if existing_section["name"] == name:
                            existing_idx = i
                            break
                    if existing_idx >= 0 and messagebox.askyesno("警告", f"段名 '{name}' 已存在。是否要替换它？"):
                        # 替换现有段
                        success, msg = self.data_manager.update_section(existing_idx, section)
                        if success:
                            self.current_index = existing_idx
                            self.update_list()
                            # 重新选中该段
                            self.section_listbox.selection_clear(0, tk.END)
                            self.section_listbox.selection_set(existing_idx)
                            self.status_label.config(text=f"已替换段: {name}", fg="green")
                            # 如果有回调函数，调用它来重新生成内容
                            if self.on_update_callback:
                                self.on_update_callback()
                            return
                        else:
                            messagebox.showerror("错误", msg)
                            return
                else:
                    messagebox.showerror("错误", msg)
                    return
        else:
            # 更新现有段
            success, msg = self.data_manager.update_section(self.current_index, section)
            if success:
                action_text = "已更新段"
            else:
                messagebox.showerror("错误", msg)
                return
        
        # 更新UI
        self.update_list()
        # 重新选中当前段
        self.section_listbox.selection_clear(0, tk.END)
        self.section_listbox.selection_set(self.current_index)
        
        self.status_label.config(text=f"{action_text}: {name}", fg="green")
        # 如果有回调函数，调用它来重新生成内容
        if self.on_update_callback:
            self.on_update_callback()
    
    def clear_form(self):
        """清空表单"""
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
        self.current_index = None
        self.status_label.config(text="编辑已取消", fg="gray")
    
    def update_list(self):
        """更新段列表"""
        self.section_listbox.delete(0, tk.END)
        for section_name in self.data_manager.get_sections_list():
            self.section_listbox.insert(tk.END, section_name)
        
        # 更新组件字段显示（全局组件名称）
        self._update_component_display()
        
        # 更新路径配置到UI
        self.update_paths_from_data_manager()

    def update_paths_from_data_manager(self):
        """从data_manager更新路径配置到UI"""
        output_paths = self.data_manager.data.get("output_paths", {})
        linker_script_path = output_paths.get("linker_script", "")
        header_file_path = output_paths.get("header_file", "")
        
        # 如果路径是相对路径且存在项目路径，则转换为绝对路径用于显示
        display_linker_path = linker_script_path
        display_header_path = header_file_path
        
        if self.data_manager.current_project_path:
            project_dir = os.path.dirname(self.data_manager.current_project_path)
            if linker_script_path and not os.path.isabs(linker_script_path):
                try:
                    display_linker_path = os.path.join(project_dir, linker_script_path)
                except:
                    display_linker_path = linker_script_path  # 如果转换失败，使用原始路径
            if header_file_path and not os.path.isabs(header_file_path):
                try:
                    display_header_path = os.path.join(project_dir, header_file_path)
                except:
                    display_header_path = header_file_path  # 如果转换失败，使用原始路径
        
        # 更新输入框
        self.linker_path_entry.delete(0, tk.END)
        self.linker_path_entry.insert(0, linker_script_path)  # 仍显示相对路径在输入框中
        self.header_path_entry.delete(0, tk.END)
        self.header_path_entry.insert(0, header_file_path)  # 仍显示相对路径在输入框中
        
        # 更新标签显示（显示绝对路径的文件名）
        if display_linker_path:
            self.current_linker_label.config(text=os.path.basename(display_linker_path), fg="blue")
        else:
            self.current_linker_label.config(text="未保存", fg="gray")
            
        if display_header_path:
            self.current_header_label.config(text=os.path.basename(display_header_path), fg="blue")
        else:
            self.current_header_label.config(text="未保存", fg="gray")
    
    def browse_linker_path(self):
        """浏览并设置链接器脚本路径"""
        # 根据链接器类型确定默认文件扩展名
        linker_type = self.linker_type.get()
        default_ext = ".ld" if linker_type == "gcc" else ".sct"
        
        if linker_type == "gcc":
            filetypes = [("GCC Linker Script", "*.ld"), ("All files", "*.*")]
        else:
            filetypes = [("Keil Scatter File", "*.sct"), ("All files", "*.*")]
        
        filepath = filedialog.asksaveasfilename(
            title="选择链接器脚本保存路径",
            defaultextension=default_ext,
            filetypes=filetypes
        )
        
        if filepath:
            # 将路径设置到输入框中（相对于项目文件的相对路径）
            project_dir = os.path.dirname(self.data_manager.current_project_path) if self.data_manager.current_project_path else os.getcwd()
            try:
                rel_path = os.path.relpath(filepath, project_dir)
                self.linker_path_entry.delete(0, tk.END)
                self.linker_path_entry.insert(0, rel_path)
            except ValueError:
                # 如果无法生成相对路径，则使用绝对路径
                self.linker_path_entry.delete(0, tk.END)
                self.linker_path_entry.insert(0, filepath)
    
    def browse_header_path(self):
        """浏览并设置头文件路径"""
        filepath = filedialog.asksaveasfilename(
            title="选择头文件保存路径",
            defaultextension=".h",
            filetypes=[("Header files", "*.h"), ("All files", "*.*")]
        )
        
        if filepath:
            # 将路径设置到输入框中（相对于项目文件的相对路径）
            project_dir = os.path.dirname(self.data_manager.current_project_path) if self.data_manager.current_project_path else os.getcwd()
            try:
                rel_path = os.path.relpath(filepath, project_dir)
                self.header_path_entry.delete(0, tk.END)
                self.header_path_entry.insert(0, rel_path)
            except ValueError:
                # 如果无法生成相对路径，则使用绝对路径
                self.header_path_entry.delete(0, tk.END)
                self.header_path_entry.insert(0, filepath)
    
    def _on_component_change(self, event=None):
        """当组件名称输入框失去焦点时，更新数据管理器中的组件名称"""
        component_name = self.component_entry.get().strip()
        self.data_manager.data["component"] = component_name
        # 如果有回调函数，调用它来更新预览
        if self.on_update_callback:
            self.on_update_callback()
    
    def _update_component_display(self):
        """更新组件名称显示"""
        component_name = self.data_manager.data.get("component", "")
        if hasattr(self, 'component_entry'):
            self.component_entry.delete(0, tk.END)
            self.component_entry.insert(0, component_name)
    
    def update_generate_files(self):
        """更新/生成文件 - 根据用户选择的路径生成链接器脚本和/或头文件"""
        linker_path = self.linker_path_entry.get().strip()
        header_path = self.header_path_entry.get().strip()
        
        # 至少需要选择一个路径
        if not linker_path and not header_path:
            messagebox.showerror("错误", "请至少选择链接器脚本或头文件中的一个路径")
            return
        
        # 如果有项目路径，则基于项目路径构建实际路径
        project_dir = os.path.dirname(self.data_manager.current_project_path) if self.data_manager.current_project_path else os.getcwd()
        
        actual_linker_path = None
        actual_header_path = None
        
        if linker_path:
            if os.path.isabs(linker_path):
                actual_linker_path = linker_path
            else:
                actual_linker_path = os.path.join(project_dir, linker_path)
        
        if header_path:
            if os.path.isabs(header_path):
                actual_header_path = header_path
            else:
                actual_header_path = os.path.join(project_dir, header_path)
        
        # 生成内容
        try:
            linker_type = self.linker_type.get()
            
            if linker_path:
                # 生成链接器脚本
                linker_script_content = self.template_handler.generate_linker_script(
                    self.data_manager.data["custom_sections"], linker_type
                )
                
                success, msg = self.template_handler.save_linker_script(linker_script_content, actual_linker_path)
                if success:
                    # 更新保存路径（存储相对路径）
                    if self.data_manager.current_project_path:
                        project_dir = os.path.dirname(self.data_manager.current_project_path)
                        try:
                            rel_path = os.path.relpath(actual_linker_path, project_dir)
                            self.data_manager.data["output_paths"]["linker_script"] = rel_path
                        except ValueError:
                            # 如果无法生成相对路径，则使用原始路径
                            self.data_manager.data["output_paths"]["linker_script"] = actual_linker_path
                    else:
                        self.data_manager.data["output_paths"]["linker_script"] = actual_linker_path
                    self.current_linker_label.config(text=os.path.basename(actual_linker_path), fg="blue")
                    messagebox.showinfo("成功", msg)
                else:
                    messagebox.showerror("错误", msg)
            
            if header_path:
                # 生成头文件
                component_name = self.data_manager.data.get("component", "DEFAULT")
                header_content = self.template_handler.generate_header_file(
                    self.data_manager.data["custom_sections"],
                    component_name
                )
                
                success, msg = self.template_handler.save_header_file(header_content, actual_header_path)
                if success:
                    # 更新保存路径（存储相对路径）
                    if self.data_manager.current_project_path:
                        project_dir = os.path.dirname(self.data_manager.current_project_path)
                        try:
                            rel_path = os.path.relpath(actual_header_path, project_dir)
                            self.data_manager.data["output_paths"]["header_file"] = rel_path
                        except ValueError:
                            # 如果无法生成相对路径，则使用原始路径
                            self.data_manager.data["output_paths"]["header_file"] = actual_header_path
                    else:
                        self.data_manager.data["output_paths"]["header_file"] = actual_header_path
                    self.current_header_label.config(text=os.path.basename(actual_header_path), fg="blue")
                    messagebox.showinfo("成功", msg)
                else:
                    messagebox.showerror("错误", msg)
                    
        except Exception as e:
            messagebox.showerror("错误", f"生成文件失败: {str(e)}")


class PreviewFrame(ttk.Frame):
    """预览组件"""
    
    def __init__(self, parent, template_handler):
        super().__init__(parent)
        self.template_handler = template_handler
        self.setup_ui()
    
    def setup_ui(self):
        """设置预览界面"""
        # 使用Notebook显示链接器脚本和头文件
        preview_notebook = ttk.Notebook(self)
        preview_notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 链接器脚本预览
        linker_frame = ttk.Frame(preview_notebook)
        preview_notebook.add(linker_frame, text="链接器脚本")
        
        # 控制按钮
        linker_control_frame = tk.Frame(linker_frame)
        linker_control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(linker_control_frame, text="复制到剪贴板", command=self.copy_linker_to_clipboard,
                 width=15).pack(side=tk.LEFT, padx=5)
        
        # 脚本预览区域
        self.linker_text = scrolledtext.ScrolledText(linker_frame, wrap=tk.NONE)
        self.linker_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 头文件预览
        header_frame = ttk.Frame(preview_notebook)
        preview_notebook.add(header_frame, text="头文件")
        
        # 控制按钮
        header_control_frame = tk.Frame(header_frame)
        header_control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(header_control_frame, text="复制到剪贴板", command=self.copy_header_to_clipboard,
                 width=15).pack(side=tk.LEFT, padx=5)
        
        # 头文件预览区域
        self.header_text = scrolledtext.ScrolledText(header_frame, wrap=tk.NONE)
        self.header_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 默认显示内容
        self.linker_text.insert(1.0, "请先生成链接器脚本")
        self.header_text.insert(1.0, "请先生成头文件")
    
    def update_preview(self, sections, linker_type="gcc", component_name="PREVIEW"):
        """更新预览内容"""
        try:
            # 生成链接器脚本
            linker_script = self.template_handler.generate_linker_script(sections, linker_type)
            self.linker_text.delete(1.0, tk.END)
            self.linker_text.insert(1.0, linker_script)
            # 应用语法高亮
            self.highlight_linker_script()
        except Exception as e:
            self.linker_text.delete(1.0, tk.END)
            self.linker_text.insert(1.0, f"生成链接器脚本时出错: {str(e)}")
        
        try:
            # 生成头文件 - 使用指定的组件名称
            header_content = self.template_handler.generate_header_file(sections, component_name)
            self.header_text.delete(1.0, tk.END)
            self.header_text.insert(1.0, header_content)
            # 应用语法高亮
            self.highlight_header_file()
        except Exception as e:
            self.header_text.delete(1.0, tk.END)
            self.header_text.insert(1.0, f"生成头文件时出错: {str(e)}")
    
    def copy_linker_to_clipboard(self):
        """复制链接器脚本到剪贴板"""
        script = self.linker_text.get(1.0, tk.END).strip()
        if script:
            self.clipboard_clear()
            self.clipboard_append(script)
            messagebox.showinfo("成功", "链接器脚本已复制到剪贴板")
    
    def copy_header_to_clipboard(self):
        """复制头文件到剪贴板"""
        header = self.header_text.get(1.0, tk.END).strip()
        if header:
            self.clipboard_clear()
            self.clipboard_append(header)
            messagebox.showinfo("成功", "头文件已复制到剪贴板")
    
    def highlight_linker_script(self):
        """为链接器脚本应用语法高亮"""
        # 清除之前的标签
        self.linker_text.tag_remove("comment", "1.0", tk.END)
        self.linker_text.tag_remove("directive", "1.0", tk.END)
        self.linker_text.tag_remove("section", "1.0", tk.END)
        self.linker_text.tag_remove("symbol", "1.0", tk.END)
        self.linker_text.tag_remove("number", "1.0", tk.END)
        
        # GCC链接器脚本语法高亮
        content = self.linker_text.get("1.0", tk.END)
        lines = content.split('\n')
        
        for line_idx, line in enumerate(lines):
            line_start = f"{line_idx + 1}.0"
            line_end = f"{line_idx + 1}.end"
            
            # 注释高亮 (以 /* 或 // 开头的注释)
            comment_start = line.find('/*')
            if comment_start != -1:
                comment_end = line.find('*/', comment_start + 2)
                if comment_end != -1:
                    comment_start_pos = f"{line_idx + 1}.{comment_start}"
                    comment_end_pos = f"{line_idx + 1}.{comment_end + 2}"
                    self.linker_text.tag_add("comment", comment_start_pos, comment_end_pos)
            
            # 另一种注释高亮 (以 // 开头的注释)
            cpp_comment_start = line.find('//')
            if cpp_comment_start != -1:
                cpp_comment_start_pos = f"{line_idx + 1}.{cpp_comment_start}"
                cpp_comment_end_pos = line_end
                self.linker_text.tag_add("comment", cpp_comment_start_pos, cpp_comment_end_pos)
            
            # 指令高亮 (如 SECTIONS, MEMORY 等)
            directives = ['SECTIONS', 'MEMORY', 'PROVIDE', 'INCLUDE', 'ASSERT']
            for directive in directives:
                start_idx = 0
                while True:
                    start_pos = line.find(directive, start_idx)
                    if start_pos == -1:
                        break
                    end_pos = start_pos + len(directive)
                    self.linker_text.tag_add("directive", f"{line_idx + 1}.{start_pos}", f"{line_idx + 1}.{end_pos}")
                    start_idx = end_pos
            
            # 段名高亮 (通常在大括号内)
            # 简单查找可能的段名模式
            import re
            # 匹配段名: 如 .text, .data, 或自定义段名
            section_pattern = r'(\.\w+|\w+(?=\s*:))'
            for match in re.finditer(section_pattern, line):
                start_pos = f"{line_idx + 1}.{match.start()}"
                end_pos = f"{line_idx + 1}.{match.end()}"
                self.linker_text.tag_add("section", start_pos, end_pos)
        
        # 设置高亮颜色
        self.linker_text.tag_config("comment", foreground="green")
        self.linker_text.tag_config("directive", foreground="blue", font=("Consolas", 10, "bold"))
        self.linker_text.tag_config("section", foreground="purple")
        self.linker_text.tag_config("symbol", foreground="orange")
        self.linker_text.tag_config("number", foreground="red")
    
    def highlight_header_file(self):
        """为头文件应用语法高亮"""
        # 清除之前的标签
        self.header_text.tag_remove("comment", "1.0", tk.END)
        self.header_text.tag_remove("directive", "1.0", tk.END)
        self.header_text.tag_remove("keyword", "1.0", tk.END)
        self.header_text.tag_remove("macro", "1.0", tk.END)
        self.header_text.tag_remove("type", "1.0", tk.END)
        
        content = self.header_text.get("1.0", tk.END)
        lines = content.split('\n')
        
        for line_idx, line in enumerate(lines):
            line_start = f"{line_idx + 1}.0"
            line_end = f"{line_idx + 1}.end"
            
            # 预处理指令高亮 (#define, #include, #if 等)
            if line.strip().startswith('#'):
                self.header_text.tag_add("directive", line_start, line_end)
            
            # 注释高亮
            comment_start = line.find('/*')
            if comment_start != -1:
                comment_end = line.find('*/', comment_start + 2)
                if comment_end != -1:
                    comment_start_pos = f"{line_idx + 1}.{comment_start}"
                    comment_end_pos = f"{line_idx + 1}.{comment_end + 2}"
                    self.header_text.tag_add("comment", comment_start_pos, comment_end_pos)
            
            # C++风格注释高亮
            cpp_comment_start = line.find('//')
            if cpp_comment_start != -1:
                cpp_comment_start_pos = f"{line_idx + 1}.{cpp_comment_start}"
                cpp_comment_end_pos = line_end
                self.header_text.tag_add("comment", cpp_comment_start_pos, cpp_comment_end_pos)
            
            # 关键字高亮
            keywords = ['extern', 'uint8_t', 'uint32_t', 'size_t', 'void']
            for keyword in keywords:
                start_idx = 0
                while True:
                    start_pos = line.find(keyword, start_idx)
                    if start_pos == -1:
                        break
                    end_pos = start_pos + len(keyword)
                    # 确保匹配的是完整单词（前后是空格或非字母数字字符）
                    before_char = line[start_pos - 1] if start_pos > 0 else ' '
                    after_char = line[end_pos] if end_pos < len(line) else ' '
                    if not (before_char.isalnum() or before_char == '_') and \
                       not (after_char.isalnum() or after_char == '_'):
                        self.header_text.tag_add("keyword", f"{line_idx + 1}.{start_pos}", f"{line_idx + 1}.{end_pos}")
                    start_idx = end_pos
            
            # 宏定义高亮
            macro_pattern = r'#define\s+(\w+)'
            for match in re.finditer(macro_pattern, line):
                start_pos = f"{line_idx + 1}.{match.start(1)}"
                end_pos = f"{line_idx + 1}.{match.end(1)}"
                self.header_text.tag_add("macro", start_pos, end_pos)
        
        # 设置高亮颜色
        self.header_text.tag_config("comment", foreground="green")
        self.header_text.tag_config("directive", foreground="purple", font=("Consolas", 10, "bold"))
        self.header_text.tag_config("keyword", foreground="blue", font=("Consolas", 10, "bold"))
        self.header_text.tag_config("macro", foreground="orange", font=("Consolas", 10, "bold"))
        self.header_text.tag_config("type", foreground="darkorange", font=("Consolas", 10, "bold"))