# BufferFlowX 段管理工具使用指南

## 简介

BufferFlowX 段管理工具是一套用于自动化管理嵌入式项目中内存段的工具链。它通过YAML配置文件定义各个组件的内存段需求，并自动生成链接脚本和必要的C代码，以实现对RAM/FLASH等内存区域的精确控制。

### 主要功能

- **自动段分配**：根据YAML配置自动分配内存段到指定的内存区域
- **内存区域映射**：支持将段运行时放在一个区域，存储在另一个区域（如RAM AT FLASH、RAM AT EEPROM等）
- **符号生成**：自动生成段的起始、结束符号
- **数据拷贝**：自动生成构造函数，在main()函数之前将数据从存储区域拷贝到运行区域

## 核心概念

### 术语解释

- **段（Section）**：程序中的特定数据或代码区域（如.text、.data、.bss等）
- **区域（Region）**：物理内存区域（如RAM、FLASH、EEPROM等）
- **AT机制**：允许段在运行时位于一个区域，但存储在另一个区域（例如：代码在FLASH中存储，在RAM中运行）
- **组件（Component）**：包含bfx.yml配置文件的项目模块

### 目录结构

```
bfx/section/
├── gcc/                    # GCC工具链相关脚本
│   ├── bfx_section_main.py    # 主入口脚本
│   ├── bfx_get_region.py      # 区域信息获取脚本
│   ├── bfx_get_section.py     # 段信息获取脚本
│   ├── bfx_generate_ld.py     # 链接脚本生成脚本
│   ├── sections_template.j2   # 链接脚本模板
│   ├── copy_sections_template.j2 # 拷贝段C代码模板
│   └── log_utils.py           # 日志工具
└── README_zhCN.md          # 本文档
```

## 快速开始

### 1. 创建YAML配置文件

在项目组件目录下创建`bfx.yml`文件：

```yaml
name: "my_component"  # 组件名称
section:              # 段定义列表
  - name: ".text"
    region: "FLASH"
    align: 4
    size: 2048
  - name: ".data"
    region: "RAM"
    load_region: "FLASH"  # 存储在FLASH，运行时在RAM
    align: 4
    size: 512
  - name: ".bss"
    region: "RAM"
    align: 8
    size: 1024
```

### 2. 准备链接脚本模板

准备基础链接脚本模板（如`template.ld.bfx`）：

```ld
MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH (rx) : ORIGIN = 0x8000000, LENGTH = 1M
}

SECTIONS
{
  /* 工具将在此处自动插入段定义 */
}
```

### 3. 运行工具

```bash
python bfx/section/gcc/bfx_section_main.py \
  --project-root /path/to/project \
  --ld-template /path/to/template.ld.bfx \
  --output-dir /path/to/output \
  --output-ld /path/to/output/generated.ld \
  --copy-sections-file /path/to/output/copy_sections.json \
  --output-c-file /path/to/output/copy_sections.c
```

### 4. 在代码中使用自定义段

```c
// 将函数放到自定义段中
__attribute__((section(".my_code"))) 
void my_custom_function(void) {
    // 函数实现
}
```

## 详细使用指南

### YAML配置详解

#### 配置参数说明

- `name`：组件名称（必选）
- `section`：段定义列表（必选）
  - `name`：段名称（必选）
  - `region`：运行时内存区域（必选）
  - `load_region`：存储时内存区域（可选，用于AT机制）
  - `align`：内存对齐（可选，默认为0）
  - `size`：段的最大长度（可选，默认为0）
  - `address`：段起始地址（可选，默认为0）

#### 配置示例

```yaml
name: "component1"
section:
  - name: ".text"
    region: "FLASH"
    align: 4
    size: 2048
  - name: ".data"
    region: "RAM"
    load_region: "FLASH"  # 运行时在RAM，存储在FLASH
    align: 4
    size: 512
  - name: ".bss"
    region: "RAM"
    align: 8
    size: 1024
```

### 命令行工具使用

#### 参数说明

- `--project-root`：项目根目录路径
- `--ld-template`：基础链接脚本模板路径
- `--output-dir`：输出目录路径
- `--output-ld`：生成的链接脚本输出路径
- `--copy-sections-file`：生成的拷贝段信息JSON文件路径
- `--output-c-file`：生成的C代码文件路径
- `--clean`：清理生成的文件

### 集成到项目

#### CMake集成（推荐）

```cmake
# 包含BufferFlowX的CMake工具
include(${CMAKE_SOURCE_DIR}/bfx/bfx_cmake_util.cmake)

# 使用BufferFlowX的段管理功能
bfx_add_section_management(
    TARGET your_main_target
    PROJECT_ROOT ${CMAKE_SOURCE_DIR}
    LD_TEMPLATE ${CMAKE_SOURCE_DIR}/template.ld.bfx
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
    OUTPUT_LD ${CMAKE_BINARY_DIR}/generated/generated.ld
    COPY_SECTIONS_FILE ${CMAKE_BINARY_DIR}/generated/copy_sections.json
    OUTPUT_C_FILE ${CMAKE_BINARY_DIR}/generated/copy_sections.c
)
```

#### 直接使用编译器

1. 将生成的链接脚本添加到编译器链接选项：
   ```bash
   gcc -T /path/to/generated.ld ...
   ```

2. 将生成的C文件添加到编译列表：
   ```bash
   gcc -c /path/to/copy_sections.c ...
   ```

### 在代码中使用自定义段

使用GCC的`__attribute__((section("段名")))`将函数或变量放置到特定段中：

```c
// 将变量放到自定义段中
__attribute__((section(".my_data"))) 
int my_custom_data[100] = {0};

// 将函数放到自定义段中
__attribute__((section(".my_code"))) 
void my_custom_function(void) {
    // 函数实现
}

// 将常量放到特定段中
__attribute__((section(".my_rodata"))) 
const char* my_string = "Hello World";
```

## 高级特性

### AT机制详解

AT机制允许将数据或代码存储在一个内存区域，但在运行时加载到另一个区域。这对于需要快速访问但又需要持久存储的数据特别有用。

#### 工作原理

1. 链接脚本中生成 `> {region} AT > {load_region}` 语法
2. 生成的C文件包含拷贝代码，将数据从load_region拷贝到region
3. 提供LMA（Load Memory Address）符号用于访问原始数据

#### 支持的映射类型

- RAM AT FLASH - 常用于将初始化数据存储在FLASH，运行时在RAM
- RAM AT EEPROM - 适用于需要持久存储但运行在RAM的数据
- BACKUP_RAM AT FLASH - 适用于备份内存区域
- 以及其他任意区域到区域的映射

#### 配置示例

```yaml
# 支持多种存储到运行的映射
section:
  # RAM AT FLASH - 常用于将初始化数据存储在FLASH，运行时在RAM
  - name: ".data"
    region: "RAM"
    load_region: "FLASH"
    
  # RAM AT EEPROM - 适用于需要持久存储但运行在RAM的数据
  - name: ".backup_data"
    region: "RAM"
    load_region: "EEPROM"
    
  # BACKUP_RAM AT FLASH - 适用于备份内存区域
  - name: ".backup_ram"
    region: "BACKUP_RAM"
    load_region: "FLASH"
```

### 符号生成

工具会为每个段生成以下符号：

- `_start{section_name}`：段起始地址
- `_end{section_name}`：段结束地址
- `__load_start{section_name}`：段加载地址（LMA）

## 最佳实践

1. **合理规划内存使用**：确保各组件的内存需求不超过硬件限制
2. **使用AT机制优化性能**：将关键代码从FLASH拷贝到RAM中运行
3. **注意内存对齐**：根据处理器要求设置合适的对齐参数
4. **合理设置段大小**：避免内存溢出
5. **统一命名规范**：使用一致的段命名约定
6. **在代码中使用自定义段**：使用GCC的`__attribute__((section("段名")))`将函数或变量放置到特定段中

## 常见问题解答

### Q: 为什么需要AT机制？
A: 在嵌入式系统中，不同内存类型具有不同特性。例如，RAM通常容量较小且易失，而FLASH容量大但只读。通过AT机制，可以将初始化数据存储在大容量存储器中，运行时在高速访问的RAM中，从而优化内存使用效率。

### Q: 生成的C文件有什么作用？
A: 生成的C文件包含一个构造函数，会在main()函数之前自动运行，将标记为使用AT机制的段从存储区域拷贝到运行区域，确保程序运行时数据在正确的内存位置。

### Q: 如何处理内存冲突？
A: 工具会自动检查内存区域分配，如果检测到冲突会报错。确保各个组件的内存需求不超过硬件限制。

### Q: 如何在C代码中使用自定义段？
A: 使用GCC的`__attribute__((section("段名")))`语法将函数或变量放置到特定段中，例如：
```c
__attribute__((section(".my_data"))) int my_custom_data[100] = {0};
```

## 故障排除

如果遇到问题，请检查：

1. YAML配置文件语法是否正确
2. 内存区域名称是否匹配
3. 项目路径是否正确
4. 生成的文件是否已正确集成到构建系统中
5. 生成的C文件是否已编译到项目中
6. 确保在CMakeLists.txt中正确包含生成的链接脚本

## 注意事项

1. 确保YAML配置文件中的内存区域名称与链接脚本模板中的区域名称一致
2. 合理设置段大小，避免内存溢出
3. 对于需要在运行时内存区域访问但存储在其他区域的数据，必须使用`load_region`参数
4. 生成的C文件必须编译到项目中，否则AT机制功能将无法正常工作
5. 确保在构建系统中正确引用生成的链接脚本
6. 在多组件项目中，确保各组件的段定义不冲突

---
*作者：BufferFlowX团队*  
*版本：0.1*  
*许可证：MIT*