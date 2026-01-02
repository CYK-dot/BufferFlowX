# BufferFlowX 链接器脚本生成器 - 打包说明

## 目录结构

```
build_excutable/
├── build_executable.ps1      # PowerShell构建脚本
├── build_excutable_env/      # Python虚拟环境（构建时自动生成）
└── dist/                     # 构建输出目录（构建后生成）
```

## 构建脚本说明

`build_executable.ps1` 是一个PowerShell脚本，用于将Python GUI应用程序打包成独立的可执行文件。

### 脚本参数

脚本支持以下三种参数（必须选择其一）：

- `--build`: 构建可执行文件
- `--clean`: 清理构建产物（保留虚拟环境）
- `--clean_all`: 清理全部（包括虚拟环境）

### 使用方法

1. **构建可执行文件**:
   ```powershell
   .\build_executable.ps1 --build
   ```

2. **清理构建产物**:
   ```powershell
   .\build_executable.ps1 --clean
   ```

3. **完全清理**:
   ```powershell
   .\build_executable.ps1 --clean_all
   ```

## 构建流程

1. 检查系统是否安装了Python
2. 检查`build_excutable_env`虚拟环境是否存在
3. 如果不存在，则创建虚拟环境并安装`requirements.txt`中的依赖
4. 使用PyInstaller将Python脚本打包成exe文件
5. 将所有必需的资源文件（模板文件等）包含在可执行文件中

## 输出

构建成功后，可执行文件将位于`dist/bfx_linker/`目录中，文件名为`bfx_linker.exe`。

## 依赖

- Python 3.x
- PowerShell 5.0+
- 项目依赖（在`requirements.txt`中定义）