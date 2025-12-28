# BufferFlowX
[![codecov](https://codecov.io/gh/CYK-Dot/BufferFlowX/branch/main/graph/badge.svg)](https://codecov.io/gh/CYK-Dot/BufferFlowX)

Project is under development.......
[中文文档](./README_CN.md) | [English Document](./README.md)
## Introduction

BufferFlowX is a lightweight embedded system framework that provides a series of efficient C language components to solve common problems in embedded development.</br>
The project includes multiple independently usable modules and a complete test project, which can evolve independently.

## Core Functions and Application Scenarios

BufferFlowX provides several core components, each targeting specific embedded development needs:

1. **Double FIFO Component (double_fifo)** - Provides a double buffer mechanism for lock-free concurrent data transfer, suitable for scenarios that require decoupling between data production and consumption, such as DMA transmission, screen rendering, etc.

2. **Single Input Single Output FIFO Component (siso_fifo)** - Implements a single producer single consumer ring buffer, supporting multiple transmission modes (split, no-split, vari), suitable for serial communication, data stream processing and other scenarios.

3. **Multiple Input Multiple Output FIFO Component (mimo_fifo)** - Extends the traditional FIFO concept to support multiple producers and consumers operating simultaneously without locks.

4. **State Machine Component (fsm)** - A toolchain based on PlantUML state diagrams to generate C code, supporting nested states and event handling, simplifying the implementation of complex state logic.

5. **L2 Protocol Component (l2proto)** - Provides data frame framing and synchronization functions for serial communications such as SPI/UART, as well as error control mechanisms, suitable for applications requiring reliable serial communication.

6. **Command Line Interface Component (cli)** - Provides command line parsing, command registration, parameter processing and other functions, supports wildcard matching and AT commands, making it easy to implement interactive command line interfaces in embedded systems.

7. **Bootloader Component** - Provides image loading and management functions, supporting loading images from different media through dependency injection, suitable for systems requiring firmware updates and multi-image management.

8. **Section Management Tool (section)** - An automated toolchain for managing memory sections in embedded projects, supporting automatic section allocation and memory area mapping, suitable for embedded projects requiring precise memory control.

## Directory Navigation

- [Double FIFO Component](./bfx/double_fifo/README_zhCN.md)
- [Single Input Single Output FIFO Component](./bfx/siso_fifo/README_zhCN.md)
- [Multiple Input Multiple Output FIFO Component](./bfx/mimo_fifo/README_zhCN.md)
- [State Machine Component](./bfx/fsm/README_zhCN.md)
- [L2 Protocol Component](./bfx/l2proto/README_zhCN.md)
- [Command Line Interface Component](./bfx/cli/README_zhCN.md)
- [Bootloader Component](./bfx/bootloader/README_zhCN.md)
- [Section Management Tool](./bfx/section/README_zhCN.md)

## Design Philosophy

BufferFlowX is designed following these principles:

- **Modularity**: Each component can be used independently, making it easy to integrate into existing projects
- **Efficiency**: Focus on performance optimization, suitable for resource-constrained embedded environments
- **Usability**: Provide simple API interfaces to reduce usage complexity
- **Extensibility**: Designed with good extensibility, supporting feature customization

## Installation and Usage

BufferFlowX adopts a modular design, you can choose to use specific components as needed. Each component contains complete header files and implementation files, just add the relevant files to your project.

For CMake projects, you can use the CMake utility files provided by BufferFlowX:

```cmake
include(${CMAKE_SOURCE_DIR}/bfx/bfx_cmake_util.cmake)
```