/**
 * @file siso_fifo.h
 * @brief 实现FIFO的创建、销毁、发送、接收、声明完成等功能
 * @author CYK-Dot
 * @date 2025-05-13
 * @version 1.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 配置宏 -----------------------------------------------------*/

#define DAP_FIFO_DYNAMIC_CHECK 0

/* 头文件 -----------------------------------------------------*/

#include <stdlib.h>

/* 系统调用 ---------------------------------------------------*/

#define FifoPortMalloc malloc
#define FifoPortFree   free
#define FifoPortEnterCriticalFromAll() do {} while(0)
#define FifoPortExitCriticalFromAll() do {} while(0)

/* 错误码 -----------------------------------------------------*/

#define DAP_FIFO_OK 0
#define DAP_FIFO_ERROR -1
#define DAP_FIFO_INVALID_PARAM -2
#define DAP_FIFO_NO_SPACE -3
#define DAP_FIFO_NO_DATA -4 
#define DAP_FIFO_NOT_ALLOWED -5


/* 导出类型 ---------------------------------------------------*/

/**
 * @brief 缓冲区对象
 */
typedef void* DAPFifo_t;

/**
 * @brief 缓冲区实例
 */
typedef struct 
{
    uint8_t *mem;
    size_t   memSize;
    size_t   indexWriteHead;
    size_t   indexWriteTail;
    size_t   indexReadHead;
    size_t   indexReadTail;
}DAPFifoHandle_t;

/* 导出函数 ---------------------------------------------------*/
extern uint16_t uxCriticalNesting;
DAPFifo_t DAPFifoCreateStatic(size_t fifoSize,DAPFifoHandle_t *staticHandle,uint8_t *fifoMemory);
DAPFifo_t DAPFifoCreate(size_t fifosize);
void DAPFifoDestroy(DAPFifo_t fifo);
int DAPFifoSendAcquire(DAPFifo_t fifo, size_t size, void *memAcquired[2]);
int DAPFifoSendAcquireNoSplit(DAPFifo_t fifo, size_t size, void *memAcquired[2]);
int DAPFifoSendComplete(DAPFifo_t fifo, const void *memAcquired[2]);
int DAPFifoRecvAcquire(DAPFifo_t fifo, size_t size, void *memAcquired[2]);
int DAPFifoRecvAcquireNoSplit(DAPFifo_t fifo, size_t size, void *memAcquired[2]);
int DAPFifoRecvComplete(DAPFifo_t fifo,const void *memAcquired[2]);

#ifdef __cplusplus
}
#endif