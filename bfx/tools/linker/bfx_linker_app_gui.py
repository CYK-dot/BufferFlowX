"""
BufferFlowX 链接器脚本生成器 - GUI应用入口
重构版本，将原单一文件拆分为模块化结构
"""
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json
import os
import re
import sys
from datetime import datetime

# 导入自定义模块
from src.data_manager import DataManager
from src.template_handler import TemplateHandler
from src.ui_components import SectionEditorFrame, PreviewFrame


class JSONSectionEditor:
    """JSON段编辑器主类 - GUI应用入口"""
    
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("BufferFlowX 链接器脚本生成器")
        self.root.geometry("750x700")  # 减小默认窗口大小
        
        # 初始化数据管理器和模板处理器
        self.data_manager = DataManager()
        self.template_handler = TemplateHandler()
        
        # 创建UI组件
        self.setup_menu()
        self.setup_ui()
        
        # 更新列表
        self.editor_frame.update_list()
    
    def setup_menu(self):
        """设置菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="新建项目", command=self.new_project, accelerator="Ctrl+N")
        file_menu.add_command(label="打开项目", command=self.open_project, accelerator="Ctrl+O")
        file_menu.add_command(label="保存项目", command=self.save_project, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="导出链接器脚本", command=self.save_linker_script)
        file_menu.add_command(label="导出头文件", command=self.save_header_file)
        file_menu.add_separator()
        file_menu.add_command(label="退出", command=self.exit_app)
        
        # 绑定快捷键
        self.root.bind('<Control-n>', lambda e: self.new_project())
        self.root.bind('<Control-o>', lambda e: self.open_project())
        self.root.bind('<Control-s>', lambda e: self.save_project())
        
        # 帮助菜单
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(label="关于", command=self.show_about)
    
    def setup_ui(self):
        """设置主UI界面"""
        # 使用Notebook创建标签页
        notebook = ttk.Notebook(self.root)
        notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 创建编辑器框架
        self.editor_frame = SectionEditorFrame(
            notebook, 
            self.data_manager, 
            self.template_handler,
            on_update_callback=self.update_preview_content
        )
        notebook.add(self.editor_frame, text="段编辑")
        
        # 创建预览框架
        self.preview_frame = PreviewFrame(notebook, self.template_handler)
        notebook.add(self.preview_frame, text="预览")
    
    def update_preview_content(self):
        """更新预览内容"""
        try:
            linker_type = self.editor_frame.linker_type.get()
            component_name = self.data_manager.data.get("component", "DEFAULT")
            self.preview_frame.update_preview(self.data_manager.data["custom_sections"], linker_type, component_name)
        except Exception as e:
            messagebox.showerror("错误", f"更新预览失败: {str(e)}")
    
    def new_project(self):
        """新建项目"""
        if messagebox.askyesno("确认", "新建项目将清空当前内容，是否继续？"):
            self.data_manager.reset_data()
            self.editor_frame.update_list()
            self.editor_frame.clear_form()
            
            # 更新组件名称显示（使用默认值）
            self.editor_frame._update_component_display()
            
            self.preview_frame.update_preview([], "gcc", "DEFAULT")
            self.editor_frame.status_label.config(text="已创建新项目", fg="green")
    
    def open_project(self):
        """打开项目"""
        filepath = filedialog.askopenfilename(
            title="打开JSON项目文件",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        
        if filepath:
            try:
                success, msg = self.data_manager.load_from_json(filepath)
                if success:
                    # 更新UI
                    self.editor_frame.update_list()
                    self.editor_frame.clear_form()
                    
                    # 更新组件名称显示
                    self.editor_frame._update_component_display()
                    
                    # 更新路径显示（处理相对路径）
                    self.editor_frame.update_paths_from_data_manager()
                    
                    self.update_preview_content()
                    self.editor_frame.status_label.config(text=f"已加载项目: {os.path.basename(filepath)}", fg="green")
                else:
                    messagebox.showerror("错误", msg)
            except Exception as e:
                messagebox.showerror("错误", f"打开项目失败: {str(e)}")
    
    def save_project(self):
        """保存项目"""
        if self.data_manager.current_project_path:
            # 组件名称已经在_data_manager中，无需额外处理
            
            # 直接保存到当前项目路径
            success, message = self.data_manager.save_to_json(self.data_manager.current_project_path)
            if success:
                self.editor_frame.status_label.config(text=message, fg="green")
            else:
                messagebox.showerror("错误", message)
        else:
            # 如果没有当前项目路径，执行另存为操作
            self.save_project_as()

    def save_project_as(self):
        """另存为项目"""
        # 组件名称已经在_data_manager中，无需额外处理
        
        filepath = filedialog.asksaveasfilename(
            title="保存JSON项目文件",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        
        if not filepath:
            return
        
        success, message = self.data_manager.save_to_json(filepath)
        if success:
            self.editor_frame.status_label.config(text=message, fg="green")
        else:
            messagebox.showerror("错误", message)
    
    def save_linker_script(self):
        """保存链接器脚本"""
        linker_type = self.editor_frame.linker_type.get()
        
        # 生成链接器脚本
        try:
            linker_script = self.template_handler.generate_linker_script(
                self.data_manager.data["custom_sections"], linker_type
            )
        except Exception as e:
            messagebox.showerror("错误", f"生成链接器脚本失败: {str(e)}")
            return
        
        # 选择保存路径
        if linker_type == "gcc":
            default_ext = ".ld"
            filetypes = [("GCC Linker Script", "*.ld"), ("All files", "*.*")]
        else:
            default_ext = ".sct"
            filetypes = [("Keil Scatter File", "*.sct"), ("All files", "*.*")]
        
        filepath = filedialog.asksaveasfilename(
            title="保存链接器脚本",
            defaultextension=default_ext,
            filetypes=filetypes
        )
        
        if filepath:
            try:
                success, msg = self.template_handler.save_linker_script(linker_script, filepath)
                if success:
                    messagebox.showinfo("成功", msg)
                else:
                    messagebox.showerror("错误", msg)
            except Exception as e:
                messagebox.showerror("错误", f"保存链接器脚本失败: {str(e)}")
    
    def save_header_file(self):
        """保存头文件"""
        # 选择保存路径
        filepath = filedialog.asksaveasfilename(
            title="保存头文件",
            defaultextension=".h",
            filetypes=[("Header files", "*.h"), ("All files", "*.*")]
        )
        
        if not filepath:
            return  # 用户取消了操作
        
        # 生成头文件
        try:
            # 使用全局组件名称，如果没有则使用默认值
            component_name = self.data_manager.data.get("component", "DEFAULT")
            header_content = self.template_handler.generate_header_file(
                self.data_manager.data["custom_sections"],
                component_name
            )
        except Exception as e:
            messagebox.showerror("错误", f"生成头文件失败: {str(e)}")
            return
        
        try:
            success, msg = self.template_handler.save_header_file(header_content, filepath)
            if success:
                messagebox.showinfo("成功", msg)
            else:
                messagebox.showerror("错误", msg)
        except Exception as e:
            messagebox.showerror("错误", f"保存头文件失败: {str(e)}")
    
    def exit_app(self):
        """退出应用"""
        if messagebox.askokcancel("退出", "确定要退出吗？"):
            self.root.destroy()
    
    def show_about(self):
        """显示关于对话框"""
        about_text = """
BufferFlowX 链接器脚本生成器 v1.0

一个用于生成自定义链接器脚本和头文件的GUI工具。

功能特性：
- 可视化编辑内存段配置
- 支持GCC和Keil链接器脚本生成
- 自动生成配套头文件
- JSON格式项目文件管理
- 语法高亮预览

作者: BufferFlowX团队
        """
        messagebox.showinfo("关于", about_text)
    
    def run(self):
        """运行应用"""
        self.root.mainloop()


def main():
    """主函数"""
    try:
        app = JSONSectionEditor()
        app.run()
    except Exception as e:
        print(f"应用启动失败: {e}")
        messagebox.showerror("错误", f"应用启动失败:\n{e}")
        sys.exit(1)


if __name__ == "__main__":
    main()