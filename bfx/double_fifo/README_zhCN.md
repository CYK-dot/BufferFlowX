# 双FIFO组件 (bfx_dfifo_raw)

## 简介

双FIFO组件（bfx_dfifo_raw）是BufferFlowX框架中的双缓冲FIFO实现，提供了一种高效的双缓冲区机制，用于在数据生产和消费之间进行解耦。该组件通过两个独立的缓冲区（A和B）交替使用，实现了无锁的并发数据传输。

## 主要功能

- **双缓冲机制**：使用两个独立的缓冲区交替进行读写操作
- **无锁设计**：通过状态机管理避免了锁的使用，提高并发性能
- **内存连续分配**：两个缓冲区在连续的内存空间中分配
- **状态管理**：提供完整的缓冲区状态管理（空闲、写入、占用、读取）
- **自动切换**：支持缓冲区的自动切换和负载均衡

## 核心概念

### 状态枚举

- `BFX_DFIFO_STAT_FREE`：缓冲区空闲状态
- `BFX_DFIFO_STAT_WR`：缓冲区写入状态
- `BFX_DFIFO_STAT_OCP`：缓冲区占用状态（写入完成，等待读取）
- `BFX_DFIFO_STAT_RD`：缓冲区读取状态

### 控制块结构

`BFX_DFIFO_CB` 结构体包含双FIFO的控制信息：

- `sliceSize`：单个缓冲区大小
- `aStat`：A缓冲区状态
- `bStat`：B缓冲区状态
- `lastFin`：最后完成的缓冲区（A或B）

## 目录结构

```
bfx/double_fifo/
└── bfx_dfifo_raw.h     # 头文件，包含所有实现
```

## 快速开始

### 1. 初始化双FIFO

```c
#include "bfx_dfifo_raw.h"

uint8_t buffer[1024];  // 总内存空间
BFX_DFIFO_CB cb;       // 控制块

BFX_DfifoInit(buffer, sizeof(buffer), &cb);
```

### 2. 写入数据

```c
// 获取写入缓冲区
uint8_t *write_buffer = BFX_DfifoSendAcquire(buffer, &cb);
if (write_buffer != NULL) {
    // 在write_buffer中写入数据
    // ...
    
    // 提交写入完成
    BFX_DfifoSendComplete(&cb);
}
```

### 3. 读取数据

```c
// 获取读取缓冲区
uint8_t *read_buffer = BFX_DfifoRecvAcquire(buffer, &cb);
if (read_buffer != NULL) {
    // 从read_buffer中读取数据
    // ...
    
    // 完成读取操作
    BFX_DfifoRecvComplete(&cb);
}
```

## 详细使用指南

### 缓冲区分配

双FIFO将提供的内存空间平均分成两个部分，每个部分大小为 `memSize/2`。A缓冲区位于内存起始位置，B缓冲区位于 `mem + sliceSize` 位置。

### 状态转换流程

1. **写入流程**：
   - 调用 `BFX_DfifoSendAcquire()` 获取写入缓冲区
   - 状态从 `FREE` 转为 `WR`
   - 写入完成后调用 `BFX_DfifoSendComplete()`
   - 状态从 `WR` 转为 `OCP`

2. **读取流程**：
   - 调用 `BFX_DfifoRecvAcquire()` 获取读取缓冲区
   - 状态从 `OCP` 转为 `RD`
   - 读取完成后调用 `BFX_DfifoRecvComplete()`
   - 状态从 `RD` 转为 `FREE`

### 并发控制

双FIFO通过以下机制实现并发控制：

- 每个缓冲区有独立的状态标志
- 支持同时写入一个缓冲区和读取另一个缓冲区
- 防止多个线程同时访问同一缓冲区

### 同步数据传输示例

如果不需要异步或零拷贝特性，可以封装函数使Acquire和Complete位于同一上下文中：

```c
/* 同步发送数据 */
void ExampleSyncSend(uint8_t *mem, BFX_DFIFO_CB *cb, uint8_t *dataToSend, uint16_t dataLen)
{
    if (dataLen > cb->sliceSize) {
        return;
    }
    uint8_t *acquiredMem = BFX_DfifoSendAcquire(mem, cb);
    if (acquiredMem == NULL) {
        return;
    }
    memcpy(acquiredMem, dataToSend, dataLen);
    BFX_DfifoSendComplete(cb);
}
```

## 高级特性

### 状态管理机制

双缓冲区的精髓在于两个状态机之间的协调。每个缓冲区都有四种状态，总是从FREE→WR→OCP→RD→FREE，循环进行。

| (A状态, B状态) | 调用函数 | 结果 | 函数返回值 | 讲解 |
| --- | --- | --- | --- | --- |
| (FREE, FREE) | SendAcquire | (WR,FREE) | BufferA | 均空时申请内存，优先选A |
| (WR, FREE) | SendComplete | (OCP,FREE) | None | 发送完成，切换为OCP状态 |
| (OCP, FREE) | SendAcquire | (OCP,WR) | BufferB | A中有数据了，而B无人占用，故向B中写数据 |
| (OCP, FREE) | RecvAcquire | (RD,FREE) | BufferA | 仅A中有数据，因此返回A给使用者 |
| (FREE, OCP) | RecvAcquire | (FREE,RD) | BufferB | 仅B中有数据，因此返回B给使用者 |
| (WR, OCP) | RecvAcquire | (WR,RD) | BufferB | 生产者占用A，不干扰消费者从B中读取数据 |
| (WR,FREE) | SendAcquire | (WR,FREE) | NULL | 不允许同时向A和B中写数据 |
| (RD, OCP) | RecvAcquire | (RD, OCP) | NULL | 不允许同时从A和B中读数据 |

### 特殊情况处理

有一种特殊情况：当生产者不停地写数据，而消费者一直不读数据，导致A和B都处于OCP状态。
为了确保消费者总是能读取最老的那个数据，双缓冲区还会额外存储一个变量lastFin，用于指代最新的数据在哪个缓冲区中。

| lastFin | 调用函数 |函数返回值 | 讲解 |
| --- | --- | --- | --- |
| A | RecvAcquire | BufferB | A中存有最新数据，因此B存放的是老数据 |
| B | RecvAcquire | BufferA | B中存有最新数据，因此A存放的是老数据 |
| A | SendAcquire | BufferB | 申请写入数据，自然要覆盖最老的。函数调用后，状态从(OCP,OCP)变为(OCP,WR) |
| B | SendAcquire | BufferA | 状态从(OCP,OCP)变为(WR,OCP) |

### 应用场景

双缓冲区，又称AB缓冲区，其通过两个缓冲区交替使用消除互斥，让读写可以同时进行。
例如，可以把双缓冲区用于驱动屏幕，渲染器在一个缓冲区中写数据，而驱动则读取另一个缓冲区，将其展示在屏幕上，二者同时进行。

### 异步零拷贝设计

双缓冲区为异步零拷贝设计，无论读写，总是先申请(Acquire)，后提交(Complete)。
异步零拷贝最开始是为DMA设计的，可以在DMA开始前申请数据，在DMA中断中提交数据，避免了二次拷贝。
当然，用法自然不局限于DMA，而是任何需要异步或是零拷贝的场景

## 高级特性

### 负载均衡

当两个缓冲区都处于占用状态时，双FIFO会根据 `lastFin` 标志选择上次完成的缓冲区进行覆盖写入，实现负载均衡。

### 内存安全

- 提供完整的内存初始化功能
- 防止缓冲区越界访问
- 确保状态转换的原子性

## 最佳实践

1. **内存分配**：确保提供足够大的连续内存空间
2. **状态检查**：在访问缓冲区前检查其状态
3. **完成操作**：务必在读写操作完成后调用相应的完成函数
4. **错误处理**：检查获取缓冲区函数的返回值

## 常见问题解答

### Q: 为什么需要双缓冲区设计？

A: 双缓冲区设计允许生产者和消费者在不同缓冲区上同时工作，避免了传统的单缓冲区中生产者和消费者必须互斥访问的问题，提高了并发性能。

### Q: 如何选择合适的缓冲区大小？

A: 缓冲区大小应根据数据传输量和频率来选择。较大的缓冲区可以减少切换次数，但会增加内存占用；较小的缓冲区内存占用少，但可能增加切换开销。

## 故障排除

### 缓冲区获取失败

- 检查是否所有缓冲区都处于读取状态，导致无法写入
- 确认是否正确调用了完成函数释放缓冲区

### 数据丢失

- 确认写入后是否调用了 `BFX_DfifoSendComplete()` 
- 确认读取后是否调用了 `BFX_DfifoRecvComplete()`
- 检查是否存在多个线程同时操作同一缓冲区