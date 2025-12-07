/**
 * @file bfx_dfifo.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025
 */
#ifndef __BFX_DFIFO_RAW_H__
#define __BFX_DFIFO_RAW_H__

/* header import -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* macros and enums ----------------------------------------------------------------------*/
typedef enum tagBFX_DFIFO_STAT {
    BFX_DFIFO_STAT_FREE = 0,
    BFX_DFIFO_STAT_WR,
    BFX_DFIFO_STAT_OCP,
    BFX_DFIFO_STAT_RD
} BFX_DFIFO_STAT;

typedef enum tagBFX_DFIFO_LAST_FIN {
    BFX_DFIFO_LAST_A = 0,
    BFX_DFIFO_LAST_B,
} BFX_DFIFO_LAST_FIN;

typedef struct tagBFX_DFIFO_CB {
    uint16_t sliceSize;
    uint8_t aStat : 3;
    uint8_t bStat : 3;
    uint8_t lastFin : 2;
    uint8_t resv;
} BFX_DFIFO_CB;

#ifdef __cplusplus
extern "C" {
#endif

/* private functions ---------------------------------------------------------------------*/
#define BFX_DFIFO_IS_A_FREE(pxCb) \
    ((pxCb)->aStat == BFX_DFIFO_STAT_FREE)

#define BFX_DFIFO_IS_A_WRITING(pxCb) \
    ((pxCb)->aStat == BFX_DFIFO_STAT_WR)

#define BFX_DFIFO_IS_A_COMPLETE(pxCb) \
    ((pxCb)->aStat >= BFX_DFIFO_STAT_OCP)

#define BFX_DFIFO_IS_A_OCCUPIED(pxCb) \
    ((pxCb)->aStat == BFX_DFIFO_STAT_OCP)

#define BFX_DFIFO_IS_A_RD(pxCb) \
    ((pxCb)->aStat == BFX_DFIFO_STAT_RD)

#define BFX_DFIFO_IS_B_FREE(pxCb) \
    ((pxCb)->bStat == BFX_DFIFO_STAT_FREE)

#define BFX_DFIFO_IS_B_WRITING(pxCb) \
    ((pxCb)->bStat == BFX_DFIFO_STAT_WR)

#define BFX_DFIFO_IS_B_COMPLETE(pxCb) \
    ((pxCb)->bStat >= BFX_DFIFO_STAT_OCP)

#define BFX_DFIFO_IS_B_OCCUPIED(pxCb) \
    ((pxCb)->bStat == BFX_DFIFO_STAT_OCP)

#define BFX_DFIFO_IS_B_RD(pxCb) \
    ((pxCb)->bStat == BFX_DFIFO_STAT_RD)

#define BFX_DFIFO_IS_ALL_COMPLETE(pxCb) \
    ((pxCb)->aStat >= BFX_DFIFO_STAT_OCP && (pxCb)->bStat >= BFX_DFIFO_STAT_OCP)

#define BFX_DFIFO_IS_ALL_OCCUPIED(pxCb) \
    ((pxCb)->aStat == BFX_DFIFO_STAT_OCP && (pxCb)->bStat == BFX_DFIFO_STAT_OCP)

static inline uint8_t *prvBFX_DfifoOccupyWrA(uint8_t *mem, BFX_DFIFO_CB *cb)
{
    cb->lastFin = BFX_DFIFO_LAST_A;
    cb->aStat = BFX_DFIFO_STAT_WR;
    return mem;
}

static inline uint8_t *prvBFX_DfifoOccupyWrB(uint8_t *mem, BFX_DFIFO_CB *cb)
{
    cb->lastFin = BFX_DFIFO_LAST_B;
    cb->bStat = BFX_DFIFO_STAT_WR;
    return &mem[cb->sliceSize];
}

/* export functions ----------------------------------------------------------------------*/

/**
 * @brief setup double fifo
 * 
 * @param mem fifo storage, contains AB fifo in continous memory
 * @param memSize total storage size
 * @param cb control block
 * 
 * @note usable fifoSize = memSize/2
 */
static inline void BFX_DfifoInit(uint8_t *mem, uint16_t memSize, BFX_DFIFO_CB *cb)
{
    for (uint16_t i = 0; i <memSize; i++) {
        mem[i] = 0;
    }
    cb->sliceSize = memSize / 2;
    cb->aStat = BFX_DFIFO_STAT_FREE;
    cb->bStat = BFX_DFIFO_STAT_FREE;
    cb->lastFin = BFX_DFIFO_LAST_A;
}

/**
 * @brief reset fifo
 * 
 * @param mem fifo storage
 * @param cb control block
 */
static inline void BFX_DfifoClear(uint8_t *mem, BFX_DFIFO_CB *cb)
{
    for (uint16_t i = 0; i < cb->sliceSize * 2; i++) {
        mem[i] = 0;
    }
    cb->aStat = BFX_DFIFO_STAT_FREE;
    cb->bStat = BFX_DFIFO_STAT_FREE;
    cb->lastFin = BFX_DFIFO_LAST_A;
}

/**
 * @brief acquire continous space for write
 * 
 * @param mem fifo storage
 * @param cb control block
 * @return uint8_t* buffer to write. NULL when failed
 */
static inline uint8_t *BFX_DfifoSendAcquire(uint8_t *mem, BFX_DFIFO_CB *cb)
{
    if (BFX_DFIFO_IS_A_FREE(cb)) {
        return prvBFX_DfifoOccupyWrA(mem, cb);
    } else if (BFX_DFIFO_IS_B_FREE(cb)) {
        return prvBFX_DfifoOccupyWrB(mem, cb);
    } else if (BFX_DFIFO_IS_ALL_OCCUPIED(cb)) {
        if (cb->lastFin == BFX_DFIFO_LAST_A) {
            return prvBFX_DfifoOccupyWrB(mem, cb);
        } else {
            return prvBFX_DfifoOccupyWrA(mem, cb);
        }
    } else if (BFX_DFIFO_IS_A_OCCUPIED(cb) && BFX_DFIFO_IS_B_RD(cb)) {
        return prvBFX_DfifoOccupyWrA(mem, cb);
    } else if (BFX_DFIFO_IS_B_OCCUPIED(cb) && BFX_DFIFO_IS_A_RD(cb)) {
        return prvBFX_DfifoOccupyWrB(mem, cb);
    }
    return NULL;
}

/**
 * @brief commit buffer acquired before
 * 
 * @param cb control block
 */
static inline void BFX_DfifoSendComplete(BFX_DFIFO_CB *cb)
{
    if (BFX_DFIFO_IS_A_WRITING(cb)) {
        cb->aStat = BFX_DFIFO_STAT_OCP;
    } else if (BFX_DFIFO_IS_B_WRITING(cb)) {
        cb->bStat = BFX_DFIFO_STAT_OCP;
    }
}

/**
 * @brief acquire continous space for read
 * 
 * @param mem fifo storage
 * @param cb control block
 * @return uint8_t* buffer to read. NULL when failed
 */
static inline uint8_t *BFX_DfifoRecvAcquire(uint8_t *mem, BFX_DFIFO_CB *cb)
{
    if (BFX_DFIFO_IS_A_RD(cb) || BFX_DFIFO_IS_B_RD(cb)) {
        return NULL;
    } else if (BFX_DFIFO_IS_ALL_OCCUPIED(cb)) {
        if (cb->lastFin == BFX_DFIFO_LAST_A) {
            cb->bStat = BFX_DFIFO_STAT_RD;
            return &mem[cb->sliceSize];
        } else {
            cb->aStat = BFX_DFIFO_STAT_RD;
            return mem;
        }
    } else if (BFX_DFIFO_IS_A_OCCUPIED(cb)) {
        cb->aStat = BFX_DFIFO_STAT_RD;
        return mem;
    } else if (BFX_DFIFO_IS_B_OCCUPIED(cb)) {
        cb->bStat = BFX_DFIFO_STAT_RD;
        return &mem[cb->sliceSize];
    }
    return NULL;
}

/**
 * @brief commit buffer acquired before
 * 
 * @param cb control block
 */
static inline void BFX_DfifoRecvComplete(BFX_DFIFO_CB *cb)
{
    if (cb->aStat == BFX_DFIFO_STAT_RD) {
        cb->aStat = BFX_DFIFO_STAT_FREE;
    } else if (cb->bStat == BFX_DFIFO_STAT_RD) {
        cb->bStat = BFX_DFIFO_STAT_FREE;
    }
}

#ifdef __cplusplus
}
#endif
#endif