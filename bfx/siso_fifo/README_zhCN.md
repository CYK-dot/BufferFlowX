# siso_fifo - 单入单出异步环形FIFO组件

## 简介

siso_fifo（Single Input Single Output FIFO）是BufferFlowX框架中的单入单出异步环形FIFO组件。该组件提供了高效的单生产者单消费者缓冲区实现，支持多种数据传输模式，包括split、no-split和vari模式，适用于需要高性能异步数据传输的场景。

## 主要功能

- **SPSC异步环形FIFO**：单生产者单消费者模式，确保线程安全
- **多种传输模式**：支持split（可分段）、no-split（不可分段）和vari（可变长度）三种传输模式
- **动态缓冲区管理**：支持动态获取发送和接收缓冲区
- **灵活的数据操作**：提供数据发送、接收、提交、撤销等操作
- **内存高效**：采用环形缓冲区设计，减少内存拷贝

## 核心概念

### 数据结构

- **BFX_QFIFO**：FIFO控制结构，包含缓冲区指针、大小和头尾指针
  - `buf`：指向缓冲区的指针
  - `size`：缓冲区大小
  - `headReady`：已准备好发送的数据头位置
  - `tailReady`：已准备好接收的数据尾位置
  - `headPend`：待提交的发送头位置
  - `tailPend`：待提交的接收尾位置

- **BFX_QFIFO_PIECE**：FIFO数据片段结构，用于处理可能分段的数据
  - `buf[2]`：最多两个缓冲区片段的指针数组
  - `len[2]`：对应片段的长度数组

### 传输模式

1. **No-Split模式**：数据块不能跨越缓冲区边界，保证数据连续性
2. **Split模式**：数据块可以跨越缓冲区边界，分为两个片段传输
3. **Vari模式**：可变长度模式，支持动态获取整个可用缓冲区

## 目录结构

```
siso_fifo/
├── bfx_qfifo.h      # FIFO组件头文件，包含所有API定义
└── README_zhCN.md   # 本文档
```

## 快速开始

### 1. 初始化FIFO

```c
#include "bfx_qfifo.h"

// 定义缓冲区
char my_buffer[1024];
BFX_QFIFO my_fifo;

// 初始化FIFO
BFX_QfifoInit(&my_fifo, my_buffer, sizeof(my_buffer));
```

### 2. 发送数据

```c
char *send_buffer;
uint16_t acquired_size;

// 获取发送缓冲区（No-Split模式）
BFX_QfifoSendAcquireNoSplit(&my_fifo, 100, &send_buffer, &acquired_size);
if (send_buffer != NULL) {
    // 填充数据到send_buffer
    memcpy(send_buffer, data, acquired_size);
    
    // 提交发送
    BFX_QfifoSendCommit(&my_fifo);
}
```

### 3. 接收数据

```c
char *recv_buffer;
uint16_t recv_size;

// 获取接收缓冲区（No-Split模式）
BFX_QfifoRecvAcquireNoSplit(&my_fifo, 100, &recv_buffer, &recv_size);
if (recv_buffer != NULL) {
    // 从recv_buffer读取数据
    process_data(recv_buffer, recv_size);
    
    // 提交接收
    BFX_QfifoRecvCommit(&my_fifo);
}
```

## 详细使用指南

### 初始化FIFO

使用`BFX_QfifoInit`函数初始化FIFO结构，指定缓冲区和大小。

### 数据发送流程

1. 使用`BFX_QfifoSendAcquire...`函数获取发送缓冲区
2. 将数据写入缓冲区
3. 使用`BFX_QfifoSendCommit`提交数据，使其对消费者可见

### 数据接收流程

1. 使用`BFX_QfifoRecvAcquire...`函数获取接收缓冲区
2. 从缓冲区读取数据
3. 使用`BFX_QfifoRecvCommit`提交接收，释放缓冲区空间

### 三种传输模式的使用场景

- **No-Split模式**：适用于需要连续内存块的数据传输
- **Split模式**：适用于大数据块传输，可有效利用缓冲区空间
- **Vari模式**：适用于动态长度数据传输

## 高级特性

### 可变长度操作

- `BFX_QfifoSendAcquireVari`和`BFX_QfifoRecvAcquireVari`：动态获取整个可用缓冲区
- `BFX_QfifoSendCommitVari`和`BFX_QfifoRecvCommitVari`：提交指定长度的数据

### 操作撤销

- `BFX_QfifoSendUndo`：撤销未提交的发送操作
- `BFX_QfifoRecvUndo`：撤销未提交的接收操作

### 状态查询

- `BFX_QfifoFreeSize`：查询可用发送空间
- `BFX_QfifoRecvSize`：查询可接收数据量

## 最佳实践

1. **选择合适的传输模式**：根据数据特性和性能要求选择No-Split、Split或Vari模式
2. **错误检查**：始终检查获取缓冲区操作的返回值
3. **内存对齐**：确保缓冲区内存对齐以获得最佳性能
4. **避免阻塞**：在实时系统中考虑非阻塞操作
5. **资源管理**：确保FIFO在使用前后正确初始化和清理

## 常见问题解答

**Q: 为什么需要三种不同的传输模式？**
A: 不同的传输模式适用于不同的应用场景。No-Split模式保证数据连续性，Split模式最大化空间利用率，Vari模式支持动态长度操作。

**Q: 如何选择合适的缓冲区大小？**
A: 缓冲区大小应根据数据流量和内存限制来选择，通常建议是2的幂次方以提高性能。

**Q: 这个FIFO是否线程安全？**
A: 该FIFO设计为SPSC（单生产者单消费者），在单线程生产者和单线程消费者模式下是安全的。

## 故障排除

**问题：获取缓冲区时返回NULL**
解决方案：检查FIFO状态，可能缓冲区已满（发送）或为空（接收）

**问题：数据传输性能不理想**
解决方案：考虑使用合适的传输模式，或调整缓冲区大小

**问题：内存使用异常**
解决方案：确保在使用后正确提交或撤销操作，避免内存泄漏

---
*作者：CYK-Dot*  
*版本：0.1*  
*许可证：MIT*