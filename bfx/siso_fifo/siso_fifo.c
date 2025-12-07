/**
 * @file siso_fifo.c
 * @brief 实现FIFO的创建、销毁、发送、接收、声明完成等功能
 * @author CYK-Dot
 * @date 2025-05-13
 * @version 1.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "siso_fifo.h"

uint16_t uxCriticalNesting = 0;

// 可用连续空间（可跨界）
#define DAP_FIFO_FREE_SPACE(fifo) \
  ((fifo)->indexReadTail > (fifo)->indexWriteHead ? \
   (fifo)->indexReadTail - (fifo)->indexWriteHead - 1 : \
   (fifo)->memSize - ((fifo)->indexWriteHead - (fifo)->indexReadTail) - 1)

// 可用连续空间（不允许跨界）
#define DAP_FIFO_FREE_NOSPLIT_SPACE(fifo) \
    ((fifo)->indexReadTail > (fifo)->indexWriteHead ? \
     (fifo)->indexReadTail - (fifo)->indexWriteHead - 1 : \
     (fifo)->memSize - (fifo)->indexWriteHead - ((fifo)->indexReadTail == 0 ? 1 : 0))

// 已使用连续空间（可跨界）
#define DAP_FIFO_USED_SPACE(fifo) \
    ((fifo)->indexWriteTail >= (fifo)->indexReadHead ? \
    (fifo)->indexWriteTail - (fifo)->indexReadHead : \
    (fifo)->memSize - ((fifo)->indexReadHead - (fifo)->indexWriteTail))

// 已使用连续空间（不允许跨界）
#define DAP_FIFO_USED_NOSPLIT_SPACE(fifo) \
    ((fifo)->indexWriteTail >= (fifo)->indexReadHead ? \
     (fifo)->indexWriteTail - (fifo)->indexReadHead : \
     (fifo)->memSize - (fifo)->indexReadHead)

/**
 * @brief 使用静态申请的方式创建FIFO
 * @param fifoSize FIFO的大小，单位为字节，实际可用大小尾fifosize-1
 * @param staticHandle 静态的FIFO句柄
 * @param fifomemory 静态缓冲区所在的位置
 * @return 返回FIFO句柄
 */
DAPFifo_t DAPFifoCreateStatic(size_t fifoSize,DAPFifoHandle_t *staticHandle,uint8_t *fifoMemory)
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (staticHandle == NULL || fifoMemory == NULL || fifoSize == 0) {
        return NULL;
    }
    #endif
    
    staticHandle->mem = fifoMemory;
    staticHandle->memSize = fifoSize;
    staticHandle->indexWriteHead = 0;
    staticHandle->indexWriteTail = 0;
    staticHandle->indexReadHead = 0;
    staticHandle->indexReadTail = 0;
    
    return (DAPFifo_t)staticHandle;
}

/**
 * @brief 使用动态申请的方式创建FIFO
 * @param fifoSize FIFO的大小，单位为字节，实际可用大小尾fifosize-1
 * @return 返回FIFO句柄
 */
DAPFifo_t DAPFifoCreate(size_t fifoSize)
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (fifoSize == 0) {
        return NULL;
    }
    #endif
    
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)FifoPortMalloc(sizeof(DAPFifoHandle_t));
    if (handle == NULL) {
        return NULL;
    }
    
    uint8_t *mem = (uint8_t *)FifoPortMalloc(fifoSize);
    if (mem == NULL) {
        FifoPortFree(handle);
        return NULL;
    }
    
    handle->mem = mem;
    handle->memSize = fifoSize;
    handle->indexWriteHead = 0;
    handle->indexWriteTail = 0;
    handle->indexReadHead = 0;
    handle->indexReadTail = 0;
    
    return (DAPFifo_t)handle;
}

/**
 * @brief 销毁FIFO  
 * @param fifo FIFO句柄
 * @warning 请勿传入静态FIFO句柄
 */
void DAPFifoDestroy(DAPFifo_t fifo)
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (fifo == NULL) {
        return;
    }
    #endif
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;
    FifoPortFree(handle->mem);
    FifoPortFree(handle);
}

/**
 * @brief 向FIFO申请发送数据
 * @param fifo FIFO句柄
 * @param size 需要发送的数据大小
 * @param memAcquired 返回的内存指针
 * @return 返回发送的数据大小
 */
__attribute__ ((section(".text_ram"))) int DAPFifoSendAcquire(DAPFifo_t fifo, size_t size, void *memAcquired[2])
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (fifo == NULL || memAcquired == NULL || size == 0) {
        return DAP_FIFO_INVALID_PARAM;
    }
    #endif
    FifoPortEnterCriticalFromAll();
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;

    // 不允许其他人同时写入
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (handle->indexWriteHead != handle->indexWriteTail) {
        FifoPortExitCriticalFromAll();
        volatile int i = 0;
        return DAP_FIFO_NOT_ALLOWED;
    }
    #endif
    // 空间不足
    if (size > DAP_FIFO_FREE_SPACE(handle)) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NO_SPACE;
    }
    

    size_t head = handle->indexWriteHead;
    size_t tail = handle->indexReadTail;
    size_t capacity = handle->memSize;
    size_t first_chunk = 0;

    if (head >= tail) {
        size_t right = capacity - head;
        if (right >= size) {
            memAcquired[0] = &handle->mem[head];
            memAcquired[1] = NULL;
            first_chunk = size;
        } 
        else {
            memAcquired[0] = &handle->mem[head];
            memAcquired[1] = &handle->mem[0];
            first_chunk = right;
        }
    } 
    else {
        memAcquired[0] = &handle->mem[head];
        memAcquired[1] = NULL;
        first_chunk = size;
    }

    handle->indexWriteHead = (head + size) % capacity;

    FifoPortExitCriticalFromAll();
    return (int)first_chunk;
}

/**
 * @brief 向FIFO申请发送数据，不进行拆分
 * @param fifo FIFO句柄
 * @param size 需要发送的数据大小
 * @param memAcquired 返回的内存指针
 * @return 返回发送的数据大小
 */
int DAPFifoSendAcquireNoSplit(DAPFifo_t fifo, size_t size, void *memAcquired[2])
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (fifo == NULL || memAcquired == NULL || size == 0) {
        return DAP_FIFO_INVALID_PARAM;
    }
    #endif
    FifoPortEnterCriticalFromAll();
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;

    // 不允许其他人同时写入
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (handle->indexWriteHead != handle->indexWriteTail) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NOT_ALLOWED;
    }
    #endif
    // 空间不足
    if (size > DAP_FIFO_FREE_NOSPLIT_SPACE(handle)) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NO_SPACE;
    }
    
    
    size_t head = handle->indexWriteHead;
    size_t tail = handle->indexReadTail;
    size_t capacity = handle->memSize;
    size_t first_chunk = 0;

    if (head >= tail) {
        size_t right = capacity - head;
        if (right >= size) {
            memAcquired[0] = &handle->mem[head];
            first_chunk = size;
        } 
        else if (tail > size) {
            memAcquired[0] = &handle->mem[0];
            head = 0;
            first_chunk = size;
        } 
        else {
            FifoPortExitCriticalFromAll();
            return DAP_FIFO_NOT_ALLOWED;
        }
    } 
    else {
        if ((tail - head) > size) {
            memAcquired[0] = &handle->mem[head];
            first_chunk = size;
        } 
        else {
            FifoPortExitCriticalFromAll();
            return DAP_FIFO_NOT_ALLOWED;
        }
    }

    memAcquired[1] = NULL;
    handle->indexWriteHead = (head + size) % capacity;

    FifoPortExitCriticalFromAll();
    return (int)first_chunk;
}

/**
 * @brief 向FIFO声明数据发送完成
 * @param fifo FIFO句柄
 * @param memAcquired 返回的内存指针
 * @return 返回发送的数据大小
 */
__attribute__ ((section(".text_ram"))) int DAPFifoSendComplete(DAPFifo_t fifo, const void *memAcquired[2])
{
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (fifo == NULL || memAcquired == NULL || memAcquired[0] == NULL) {
        return DAP_FIFO_INVALID_PARAM;
    }
    #endif
    FifoPortEnterCriticalFromAll();
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;

    if (handle->indexWriteTail != handle->indexWriteHead) {
        handle->indexWriteTail = handle->indexWriteHead;
    }

    FifoPortExitCriticalFromAll();
    return 0;
}

/**
 * @brief 向FIFO申请接收数据
 * @param fifo FIFO句柄
 * @param size 需要接收的数据大小
 * @param memAcquired 返回的内存指针
 * @return 返回接收的数据大小
 */
__attribute__ ((section(".text_ram"))) int DAPFifoRecvAcquire(DAPFifo_t fifo, size_t size, void *memAcquired[2])
{
    if (fifo == NULL || memAcquired == NULL || size == 0) {
        return DAP_FIFO_INVALID_PARAM;
    }
    FifoPortEnterCriticalFromAll();
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;

    // 不允许其他人同时读取
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (handle->indexReadHead != handle->indexReadTail) {
        FifoPortExitCriticalFromAll();
        volatile int i = 0;
        return DAP_FIFO_NOT_ALLOWED;
    }
    #endif
    // 空间不足
    if (size > DAP_FIFO_USED_SPACE(handle)) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NO_DATA;
    }
    
    size_t head = handle->indexReadHead;
    size_t tail = handle->indexWriteTail;
    size_t capacity = handle->memSize;
    size_t first_chunk = 0;

    if (head >= tail) {
        size_t right = capacity - head;
        if (right >= size) {
            memAcquired[0] = &handle->mem[head];
            memAcquired[1] = NULL;
            first_chunk = size;
        } 
        else {
            memAcquired[0] = &handle->mem[head];
            memAcquired[1] = &handle->mem[0];
            first_chunk = right;
        }
    } 
    else {
        memAcquired[0] = &handle->mem[head];
        memAcquired[1] = NULL;
        first_chunk = size;
    }
    handle->indexReadHead = (head + size) % capacity;
    
    FifoPortExitCriticalFromAll();
    return (int)first_chunk;
}

/**
 * @brief 向FIFO申请接收数据，不进行拆分
 * @param fifo FIFO句柄
 * @param size 需要接收的数据大小
 * @param memAcquired 返回的内存指针
 * @return 返回接收的数据大小
 */
int DAPFifoRecvAcquireNoSplit(DAPFifo_t fifo, size_t size, void *memAcquired[2])
{
    if (fifo == NULL || memAcquired == NULL || size == 0) {
        return DAP_FIFO_INVALID_PARAM;
    }
    FifoPortEnterCriticalFromAll();
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;

    // 不允许其他人同时读取
    #if DAP_FIFO_DYNAMIC_CHECK == 1
    if (handle->indexReadHead != handle->indexReadTail) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NOT_ALLOWED;
    }
    #endif
    // 空间不足
    if (size > DAP_FIFO_USED_NOSPLIT_SPACE(handle)) {
        FifoPortExitCriticalFromAll();
        return DAP_FIFO_NO_DATA;
    }
    
    size_t head = handle->indexReadHead;
    size_t tail = handle->indexWriteTail;
    size_t capacity = handle->memSize;
    size_t first_chunk = 0;

    if (head >= tail) {
        size_t right = capacity - head;
        if (right >= size) {
            memAcquired[0] = &handle->mem[head];
            first_chunk = size;
        } 
        else if (tail > size) {
            memAcquired[0] = &handle->mem[0];
            head = 0;
            first_chunk = size;
        } 
        else {
            return DAP_FIFO_NOT_ALLOWED;
        }
    } 
    else {
        if ((tail - head) <= handle->memSize) {
            memAcquired[0] = &handle->mem[head];
            first_chunk = size;
        } else {
            return DAP_FIFO_NOT_ALLOWED;
        }
    }

    memAcquired[1] = NULL;
    handle->indexReadHead = (head + size) % capacity;
    
    FifoPortExitCriticalFromAll();
    return (int)first_chunk;
}

/**
 * @brief 向FIFO声明数据接收完成
 * @param fifo FIFO句柄
 * @param memAcquired 返回的内存指针
 * @return 返回接收的数据大小
 */
__attribute__ ((section(".text_ram"))) int DAPFifoRecvComplete(DAPFifo_t fifo, const void *memAcquired[2])
{
    if (fifo == NULL || memAcquired == NULL || memAcquired[0] == NULL) {
        return DAP_FIFO_INVALID_PARAM;
    }
    DAPFifoHandle_t *handle = (DAPFifoHandle_t *)fifo;
    FifoPortEnterCriticalFromAll();
    
    if (handle->indexReadTail != handle->indexReadHead) {
        handle->indexReadTail = handle->indexReadHead;
    }
    
    FifoPortExitCriticalFromAll();
    return 0;
}

