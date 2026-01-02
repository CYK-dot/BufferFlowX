"""
模板处理器模块 - 处理Jinja2模板和文件生成
"""
import os
from jinja2 import Environment, FileSystemLoader, Template, select_autoescape


class TemplateHandler:
    """处理链接器脚本和头文件模板"""
    
    def __init__(self, template_dir=None):
        current_dir = os.path.dirname(os.path.abspath(__file__))
        if template_dir is None:
            # 尝试向上一级找到template目录
            template_dir = os.path.join(os.path.dirname(current_dir), "template")
        
        # 确保template目录存在
        if not os.path.exists(template_dir):
            raise Exception(f"模板目录不存在: {template_dir}")
        
        self.template_env = Environment(
            loader=FileSystemLoader(template_dir),
            autoescape=select_autoescape(['html', 'xml'])
        )
        
        # 加载模板
        self.templates = {}
        self.header_template = None
        self._load_templates()
    
    def _load_templates(self):
        """加载所有必需的模板"""
        try:
            gcc_template = self._load_template("bfx_ld_template_gcc.j2")
            keil_template = self._load_template("bfx_ld_template_armlink.j2")
            header_template = self._load_template("bfx_header_template.h.j2")
            
            if not gcc_template or not keil_template:
                missing_templates = []
                if not gcc_template:
                    missing_templates.append("bfx_ld_template_gcc.j2")
                if not keil_template:
                    missing_templates.append("bfx_ld_template_armlink.j2")
                
                raise Exception(f"无法加载必需的模板文件: {', '.join(missing_templates)}")
            
            self.templates = {
                "gcc": gcc_template,
                "keil": keil_template
            }
            
            if header_template:
                self.header_template = header_template
            
        except Exception as e:
            print(f"错误: 无法加载模板: {e}")
            raise
    
    def _load_template(self, template_name):
        """加载单个模板"""
        try:
            template = self.template_env.get_template(template_name)
            return template
        except Exception as e:
            print(f"错误: 无法加载模板 {template_name}: {e}")
            return None
    
    def generate_linker_script(self, custom_sections, linker_type="gcc"):
        """生成链接器脚本"""
        try:
            template = self.templates.get(linker_type)
            if not template:
                error_msg = f"错误: 不支持的链接器类型 '{linker_type}'\n\n"
                error_msg += "支持的类型: gcc, keil\n"
                error_msg += "请在UI中选择正确的链接器类型。"
                raise Exception(error_msg)
            
            # 准备模板上下文
            context = {
                "custom_sections": custom_sections,
                "timestamp": self._get_timestamp()
            }
            
            # 渲染模板
            script = template.render(**context)
            return script
            
        except Exception as e:
            error_msg = f"生成链接器脚本时出错: {str(e)}\n\n"
            error_msg += "可能的原因:\n"
            error_msg += "  - 模板文件损坏\n"
            error_msg += "  - 模板变量格式错误\n"
            error_msg += "  - 缺少必需的段信息\n\n"
            error_msg += f"详细错误: {str(e)}"
            raise Exception(error_msg)
    
    def generate_header_file(self, custom_sections, component_name=None):
        """生成头文件"""
        if not self.header_template:
            error_msg = "错误: 无法加载头文件模板 'bfx_header_template.h.j2'\n\n"
            error_msg += "请确保以下文件存在:\n"
            error_msg += "  - bfx_header_template.h.j2\n\n"
            error_msg += "模板文件应位于template/目录中。"
            raise Exception(error_msg)
        
        try:
            # 准备模板上下文
            context = {
                "custom_sections": custom_sections,
                "component_name": component_name or "DEFAULT",
                "timestamp": self._get_timestamp()
            }
            
            # 渲染模板
            header_content = self.header_template.render(**context)
            return header_content
            
        except Exception as e:
            error_msg = f"生成头文件时出错: {str(e)}\n\n"
            error_msg += "可能的原因:\n"
            error_msg += "  - 模板文件损坏\n"
            error_msg += "  - 模板变量格式错误\n"
            error_msg += "  - 缺少必需的段信息\n\n"
            error_msg += f"详细错误: {str(e)}"
            raise Exception(error_msg)
    
    def _get_timestamp(self):
        """获取当前时间戳"""
        from datetime import datetime
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def save_linker_script(self, content, filepath):
        """保存链接器脚本到文件"""
        try:
            # 确保目录存在
            directory = os.path.dirname(filepath)
            if directory and not os.path.exists(directory):
                os.makedirs(directory)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            
            return True, f"链接器脚本已保存到: {filepath}"
        except Exception as e:
            return False, f"保存链接器脚本失败: {str(e)}"
    
    def save_header_file(self, content, filepath):
        """保存头文件到文件"""
        try:
            # 确保目录存在
            directory = os.path.dirname(filepath)
            if directory and not os.path.exists(directory):
                os.makedirs(directory)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            
            return True, f"头文件已保存到: {filepath}"
        except Exception as e:
            return False, f"保存头文件失败: {str(e)}"