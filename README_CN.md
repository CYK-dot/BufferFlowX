# BufferFlowX
收录了常用的数据结构和DFX工具代码，为资源受限（RAM < 10KB）的MCU提供软件基础设施。</br>
![codecov](https://codecov.io/gh/CYK-dot/BufferFlowX/branch/main/graph/badge.svg)

# 项目结构
```
BufferFlowX/
├── bfx/                BufferFlowX源文件
│   ├── siso_fifo/      单生产者-单消费者 环形队列
│   ├── dfifo/          双缓冲区
│   ├── cli/            命令行
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

# 命令行
提供基本的带参数命令匹配功能，可用于运行时调试。</br>
由于调试往往不作为核心业务逻辑，功能设计的优先级为： 堆栈占用 > 灵活性 > 接口易用性 > 代码坏味道。

## 接口使用方式
```c
/**
 * @brief 基本命令行匹配
 * 
 * @param [IN/OUT]cmd 待匹配字符串，以\n结尾。
 * @param [IN]fmt 匹配表达式，使用$表示参数，例如"display adc $index value"
 * @param [OUT]paramStore 数组，存放参数所在的下标。例如paramStore[0]代表第一个参数在cmd中的下标
 * @param [IN]storeCnt 数组paramStore的下标总数。
 * @return 匹配到多少个参数。如果匹配失败，返回UINT16_MAX
 * 
 * @warning
 *      1. paramStore数组下标总数必须 大于等于 待匹配规则的token总数
 *      2. 输入的待匹配字符串必须是可写的地址。因为本函数会在参数末尾加上\0
 *      3. 本函数没有做数据合法性校验，传入无效参数后，可能造成未定义行为。
 *
 * @note 匹配规则
 *      1. 匹配表达式的约束
 *        1.1. 匹配表达式中，$前面必须是空格，例如"disp adc$value"是非法的，输入本函数后会造成未定义行为
 *        1.2. $后可以不带任何描述，例如"display adc $ -c $"是合法的，代表有两个参数，一个出现在adc后面，一个出现在-c后面
 *
 *      2. 什么样的待匹配字符串可以被匹配
 *        2.1. 待匹配字符串必须以\n结尾，例如"disp adc value"无法匹配上"disp adc value"，应换成"disp adc value\n"
 *        2.2. $后面的描述符不会参与匹配，仅用于增加可读性。例如输入命令"set pid.p= 15.5\n"可以匹配上"set pid.p= $p_value"
 *        2.3. 输入命令中多余的空格会被排除。例如命令"disp     adc value\n"可以匹配上"disp adc value"
 */
uint16_t BFX_CliRawMatch(char *cmd, const char *fmt, uint16_t *paramStore, uint16_t storeCnt)
```
| 匹配表达式 | 合法性 |
| --- | --- |
| "set led $id on" | 合法 |
| "set led$id on" | 非法，参数不应与关键字相邻 |
| "$uartIndex on" | 合法 |
| "get pid value $paramName" | 合法 |
| "$ $ $" | 合法，但没人会这样用 |

| 匹配表达式 | 待匹配命令 | 是否匹配 |
|------------|------------|-----------|
| "set led $status" | "set led on\n" | 是 |
| "set led $index on" | "set led1 on\n" | 否 |
| "set led $ on" | "set led 1 on\n" | 是 |
| "set led $ on" | "set led 1 on\r\n" | 否 |
| "read adc $index" | "read adc 1\r\n" | 是，但是读取到的参数是"1\r" |
