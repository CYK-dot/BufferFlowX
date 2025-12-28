# BufferFlowX
[![codecov](https://codecov.io/gh/CYK-Dot/BufferFlowX/branch/main/graph/badge.svg)](https://codecov.io/gh/CYK-Dot/BufferFlowX)
项目正在开发中.......

## 简介

BufferFlowX是一个轻量级的嵌入式系统框架，提供了一系列高效的C语言组件，用于解决嵌入式开发中的常见问题。</br>
该项目包含多个可独立使用的模块以及完整的测试工程，可独立演化。

## 核心功能与应用场景

BufferFlowX提供了多个核心组件，每个组件针对特定的嵌入式开发需求：

1. **双FIFO组件 (double_fifo)** - 提供双缓冲区机制，实现无锁的并发数据传输，适用于需要在数据生产和消费之间解耦的场景，如DMA传输、屏幕渲染等。

2. **单入单出FIFO组件 (siso_fifo)** - 实现单生产者单消费者环形缓冲区，支持多种传输模式（split、no-split、vari），适用于串口通信、数据流处理等场景。

3. **多入多出FIFO组件 (mimo_fifo)** - 扩展传统FIFO概念，支持多生产者多消费者无锁同时操作。

4. **状态机组件 (fsm)** - 基于PlantUML状态图生成C代码的工具链，支持嵌套状态和事件处理，简化复杂状态逻辑的实现。

5. **L2协议组件 (l2proto)** - 为SPI/UART等串行通信提供数据帧成帧与同步功能，以及差错控制机制，适用于需要可靠串行通信的应用。

6. **命令行接口组件 (cli)** - 提供命令行解析、命令注册、参数处理等功能，支持通配符匹配和AT命令，便于在嵌入式系统中实现交互式命令行界面。

7. **Bootloader组件** - 提供镜像加载与管理功能，通过依赖注入方式支持从不同介质加载镜像，适用于需要固件更新和多镜像管理的系统。

8. **段管理工具 (section)** - 自动化管理嵌入式项目中内存段的工具链，支持自动段分配和内存区域映射，适用于需要精确内存控制的嵌入式项目。

## 目录导航

- [双FIFO组件](./bfx/double_fifo/README_zhCN.md)
- [单入单出FIFO组件](./bfx/siso_fifo/README_zhCN.md)
- [多入多出FIFO组件](./bfx/mimo_fifo/README_zhCN.md)
- [状态机组件](./bfx/fsm/README_zhCN.md)
- [L2协议组件](./bfx/l2proto/README_zhCN.md)
- [命令行接口组件](./bfx/cli/README_zhCN.md)
- [Bootloader组件](./bfx/bootloader/README_zhCN.md)
- [段管理工具](./bfx/section/README_zhCN.md)

## 设计理念

BufferFlowX的设计遵循以下原则：

- **模块化**：每个组件都可以独立使用，便于集成到现有项目中
- **高效性**：注重性能优化，适用于资源受限的嵌入式环境
- **易用性**：提供简洁的API接口，降低使用复杂度
- **可扩展性**：设计具有良好的扩展性，支持功能定制

## 安装与使用

BufferFlowX采用模块化设计，你可以根据需要选择使用特定组件。每个组件都包含完整的头文件和实现文件，只需将相关文件添加到你的项目中即可。

对于CMake项目，可以使用BufferFlowX提供的CMake工具文件：

```cmake
include(${CMAKE_SOURCE_DIR}/bfx/bfx_cmake_util.cmake)
```
