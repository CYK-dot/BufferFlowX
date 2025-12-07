# BufferFlowX
收录了自己常用的MCU数据结构，以队列为主，大多为纯头文件设计。

# 项目结构
```
BufferFlowX/
├── bfx/                BufferFlowX源文件
│   ├── siso_fifo/      单生产者-单消费者 环形队列
│   ├── dfifo/          双缓冲区
├── test/               BufferFlowX测试工程
│   ├── build/          测试用例编译输出目录
│   ├── lib/            gtest静态库，编译环境Windows/MinGW64/GCC15.2.0，gtest版本1.14.0
│   ├── testcase/       测试用例源文件
│   ├── CMakeLists.txt  测试用例CMakeLists文件
├── README_CN.md
├── README.md
```

# SISO_FIFO
一个简单的单生产者-单消费者环形队列。采用异步设计，并且允许申请时不指定申请长度，而在提交时确定。

# DFIFO
双缓冲区，又称AB缓冲区，其通过两个缓冲区交替使用消除互斥，让读写可以同时进行。</br>
例如，可以把DFIFO用于驱动屏幕，渲染器在一个缓冲区中写数据，而驱动则读取另一个缓冲区，将其展示在屏幕上，二者同时进行</br>

## 接口使用方式
DFIFO为异步零拷贝设计，无论读写，总是先申请(Acquire)，后提交(Complete)。</br>
异步零拷贝最开始是为DMA设计的，可以在DMA开始前申请数据，在DMA中断中提交数据，避免了二次拷贝。</br>
当然，用法自然不局限于DMA，而是任何需要异步或是零拷贝的场景。</br>
如果不需要异步或是零拷贝怎么办？自行把函数做二次封装，让Acquire和Complete位于同一上下文即可。</br>
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

## 机制介绍
每个缓冲区都有四种状态，总是从free->wr->ocp->rd->free，循环进行。</br>
双缓冲区的精髓在于两个状态机之间的协调。其部分真值表如下所示。</br>
| (A状态, B状态) | 调用函数 | 结果 | 函数返回值 | 讲解 |
| --- | --- | --- | --- | --- |
| (free, free) | SendAcquire | (wr,free) | BufferA | 均空时申请内存，优先选A |
| (wr, free) | SendComplete | (ocp,free) | None | 发送完成，切换为ocp状态 |
| (ocp, free) | SendAcquire | (ocp,wr) | BufferB | A中有数据了，而B无人占用，故向B中写数据 |
| (ocp, free) | RecvAcquire | (rd,free) | BufferA | 仅A中有数据，因此返回A给使用者 |
| (free, ocp) | RecvAcquire | (free,rd) | BufferB | 仅B中有数据，因此返回B给使用者 |
| (wr, ocp) | RecvAcquire | (wr,rd) | BufferB | 生产者占用A，不干扰消费者从B中读取数据 |
| (wr,free) | SendAcquire | (wr,free) | NULL | 不允许同时向A和B中写数据 |
| (rd, ocp) | RecvAcquire | (rd, ocp) | NULL | 不允许同时从A和B中读数据 |

有一种特殊情况：当生产者不停地写数据，而消费者一直不读数据，导致A和B都处于ocp状态。</br>
为了确保消费者总是能读取最老的那个数据，双缓冲区还会额外存储一个变量lastBuffer，用于指代最新的数据在哪个缓冲区中。</br>
| lastBuffer | 调用函数 |函数返回值 | 讲解 |
| --- | --- | --- | --- |
| A | RecvAcquire | BufferB | A中存有最新数据，因此B存放的是老数据 |
| B | RecvAcquire | BufferA | B中存有最新数据，因此A存放的是老数据 |
| A | SendAcquire | BufferB | 申请写入数据，自然要覆盖最老的。函数调用后，状态从(ocp,ocp)变为(ocp,wr) |
| B | SendAcquire | BufferA | 状态从(ocp,ocp)变为(wr,ocp) |
