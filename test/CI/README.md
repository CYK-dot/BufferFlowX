# CI/CD 配置说明

## GitHub Actions

本项目使用 GitHub Actions 进行持续集成。配置文件位于 `.github/workflows/build-and-test.yml`。

### 工作流说明

1. 在每次推送到 `main` 或 `master` 分支时自动触发
2. 在创建 Pull Request 到 `main` 或 `master` 分支时自动触发

### 构建步骤

1. 检出代码
2. 安装依赖项（CMake, build-essential, git, python3, pip3）
3. 安装 Python 包（gcovr, jinja2）
4. 设置测试框架（下载并构建 GoogleTest）
5. 构建测试用例
6. 运行测试
7. 上传覆盖率报告和测试结果作为 artifacts

### 如何使用

1. 将 `test/CI/build-and-test.yml` 文件复制到 `.github/workflows/` 目录下：
   ```bash
   mkdir -p .github/workflows
   cp test/CI/build-and-test.yml .github/workflows/
   ```

2. 推送代码到 GitHub，GitHub Actions 将自动运行流水线

### 查看结果

- 构建和测试结果可以在 GitHub 仓库的 "Actions" 标签页查看
- 代码覆盖率报告和测试结果会作为 artifacts 上传，可以在工作流运行详情中下载