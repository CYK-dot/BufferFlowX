/**
 * @file BFX_QFIFO.h
 * @author CYK-Dot
 * @brief SPSC asyn ring-fifo, can work in split/no-split/vari mode.
 * @version 0.1
 * @date 2025-11-08
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 * ---------------------------------------------------------------------------------
 * @details 
 * - fifo layout:
 *                      is reading
 *                         |
 *                         v
 *    +----tailReady-->-tailPend-->-+
 *    |                             |
 *    +-<--headPend-<--headReady-<--+
 *            ^
 *            |
 *         is writing
 * 
 * @details 
 * - split mode(default):
 *   - acquition can be split for two parts. one continues head pointer,
 *     the other roll to array start. 
 * 
 * - no-split mode:
 *   - acquition will not be split for two part, only continues head pointer.
 * 
 * - vari mode:
 *   - acquition length is uncertain. 
 *   - producer specify the length when commit.
 */
#ifndef __BFX_QFIFO_H__
#define __BFX_QFIFO_H__
#ifdef __cplusplus
extern "C" {
#endif

/* Header import ------------------------------------------------------------------*/
#include "stdint.h"

/* Config macros -----------------------------------------------------------------*/

#define BFX_QFIFO_LIKELY(x)   __builtin_expect(!!(x), 1)
#define BFX_QFIFO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define BFX_QFIFO_DMB()       __asm__ volatile("dmb" : : : "memory")

#define BFX_QFIFO_ALIGN_TYPE(x) __attribute__((aligned(x)))
#define BFX_QFIFO_WEAK_TYPE __attribute__((weak))
#define BFX_QFIFO_INLINE __attribute__((always_inline))

/* Exported typedef --------------------------------------------------------------*/

/**
 * @brief SPSC asyn ring-fifo structure.
 */
typedef struct BFX_QFIFO {
    char *buf;
    int16_t size;
    uint16_t headReady;
    uint16_t tailReady;
    uint16_t headPend;
    uint16_t tailPend;
} BFX_QFIFO;

typedef struct tagBFX_QFIFO_PIECE {
    char *buf[2];
    uint16_t len[2];
} BFX_QFIFO_PIECE;

/* Export static inline functions -----------------------------------------------------------------*/

/**
 * @brief Initialize the FIFO.
 * 
 * @param pFifo  [OUT] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @param pBuf   [IN]  [char *]    Pointer to the buffer.
 * @param uiSize [IN]  [uint16_t]  Size of the buffer.
 */
static inline void BFX_QFIFO_INIT(BFX_QFIFO *pFifo, char *pBuf, uint16_t uiSize) {
    pFifo->buf = pBuf;
    pFifo->size = uiSize;
    pFifo->headReady = 0;
    pFifo->tailReady = 0;
    pFifo->headPend = 0;
    pFifo->tailPend = 0;
}

/**
 * @brief Get the free size of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO.
 */
static inline uint16_t BFX_QFIFO_FREE_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->tailReady > pFifo->headPend) ?
           (pFifo->tailReady - pFifo->headPend - 1) :
           (pFifo->size - (pFifo->headPend - pFifo->tailReady) - 1);
}

/**
 * @brief Get the free size of the FIFO without split.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO without split.
 */
static inline uint16_t BFX_QFIFO_FREE_NOSPLIT_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->tailReady > pFifo->headPend) ?
           (pFifo->tailReady - pFifo->headPend - 1) :
           (pFifo->size - pFifo->headPend - ((pFifo->tailReady == 0) ? 1 : 0));
}

/**
 * @brief Get the free size of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO with uncertain length.
 */
static inline uint16_t BFX_QFIFO_FREE_VARI_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->tailReady > pFifo->headReady) ?
           (pFifo->tailReady - pFifo->headReady - 1) :
           (pFifo->size - (pFifo->headReady - pFifo->tailReady) - 1);
}

/**
 * @brief Get the received size of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO.
 */
static inline uint16_t BFX_QFIFO_RECV_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->headReady >= pFifo->tailPend) ?
           (pFifo->headReady - pFifo->tailPend) :
           (pFifo->size - (pFifo->tailPend - pFifo->headReady));
}

/**
 * @brief Get the received size of the FIFO without split.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO without split.
 */
static inline uint16_t BFX_QFIFO_RECV_NOSPLIT_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->headReady >= pFifo->tailPend) ?
           (pFifo->headReady - pFifo->tailPend) :
           (pFifo->size - pFifo->tailPend);
}

/**
 * @brief Get the received size of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO with uncertain length.
 */
static inline uint16_t BFX_QFIFO_RECV_VARI_SIZE(BFX_QFIFO *pFifo) {
    return (pFifo->headReady >= pFifo->tailReady) ?
           (pFifo->headReady - pFifo->tailReady) :
           (pFifo->size - (pFifo->tailReady - pFifo->headReady));
}

/**
 * @brief Acquire the send buffer of the FIFO without split.
 * 
 * @param pFifo          [IN]  [BFX_QFIFO *] Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]  Size of the buffer.
 * @param pData          [OUT] [char *]    Pointer to the buffer.
 * @param uiAcquiredSize [OUT] [uint16_t]  Pointer to the acquired size, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(BFX_QFIFO *pFifo, uint16_t uiSize, char **pData, uint16_t *uiAcquiredSize) {
    if (BFX_QFIFO_UNLIKELY(((pFifo)->headReady != (pFifo)->headPend) ||
        (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo)) == 0)) {
            *pData = NULL;
            *uiAcquiredSize = 0;
            return;
    }
    *uiAcquiredSize = (uiSize) < (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo)) ?
                        (uiSize) : (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo));
    *pData = &((pFifo)->buf[(pFifo)->headReady]);
    (pFifo)->headPend = ((pFifo)->headReady + (*uiAcquiredSize)) % (pFifo)->size;
}

/**
 * @brief Acquire the send buffer of the FIFO with split.
 * 
 * @param pFifo          [IN] [BFX_QFIFO *]    Pointer to the FIFO structure.
 * @param uiSize         [IN] [uint16_t]     Size of the buffer.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_SEND_ACQUIRE_SPLIT(BFX_QFIFO *pFifo, uint16_t uiSize, char *ppData[2], uint16_t ppAcquiredSize[2]) {
    if (BFX_QFIFO_UNLIKELY(((pFifo)->headReady != (pFifo)->headPend) ||
        (BFX_QFIFO_FREE_SIZE(pFifo)) == 0)) {
            ppData[0] = NULL;
            ppData[1] = NULL;
            ppAcquiredSize[0] = 0;
            ppAcquiredSize[1] = 0;
            return;
    }
    uint16_t acquiredSize = (uiSize) < (BFX_QFIFO_FREE_SIZE(pFifo)) ?
                            (uiSize) : (BFX_QFIFO_FREE_SIZE(pFifo));
    if ((acquiredSize) <= (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo))) {
        ppAcquiredSize[0] = (acquiredSize);
        ppAcquiredSize[1] = 0;
        ppData[1] = NULL;
    } else {
        ppAcquiredSize[0] = (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo));
        ppAcquiredSize[1] = (acquiredSize) - (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo));
        ppData[1] = &((pFifo)->buf[0]);
    }
    ppData[0] = &((pFifo)->buf[(pFifo)->headReady]);
    (pFifo)->headPend = ((pFifo)->headReady + (acquiredSize)) % (pFifo)->size;
}

/**
 * @brief Acquire the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo          [IN] [BFX_QFIFO *]    Pointer to the FIFO structure.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_SEND_ACQUIRE_VARI(BFX_QFIFO *pFifo, char *ppData[2], uint16_t ppAcquiredSize[2]) {
    if (BFX_QFIFO_UNLIKELY((pFifo)->headReady != (pFifo)->headPend)) {
            ppData[0] = NULL;
            ppData[1] = NULL;
            ppAcquiredSize[0] = 0;
            ppAcquiredSize[1] = 0;
            return;
    }
    uint16_t noSplitFreeSize = (BFX_QFIFO_FREE_NOSPLIT_SIZE(pFifo));
    uint16_t allFreeSize = (BFX_QFIFO_FREE_SIZE(pFifo));
    ppAcquiredSize[0] = noSplitFreeSize;
    ppAcquiredSize[1] = allFreeSize - noSplitFreeSize;
    ppData[1] = &((pFifo)->buf[0]);
    ppData[0] = &((pFifo)->buf[(pFifo)->headReady]);
    (pFifo)->headPend = ((pFifo)->headReady + allFreeSize) % (pFifo)->size;
}

/**
 * @brief Commit the send buffer of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 */
static inline void BFX_QFIFO_SEND_COMMIT(BFX_QFIFO *pFifo) {
    pFifo->headReady = pFifo->headPend;
}

/**
 * @brief Commit the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo  [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @param uiSize [IN] [uint16_t]  Size of the commited length.
 */
static inline void BFX_QFIFO_SEND_COMMIT_VARI(BFX_QFIFO *pFifo, uint16_t uiSize) {
    if (BFX_QFIFO_UNLIKELY((pFifo)->headReady == (pFifo)->headPend ||
        (uiSize > (BFX_QFIFO_FREE_VARI_SIZE(pFifo))))) {
        return;
    }
    (pFifo)->headReady = ((pFifo)->headReady + uiSize) % (pFifo)->size;
    (pFifo)->headPend = (pFifo)->headReady;
}

/**
 * @brief Undo the send buffer of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 */
static inline void BFX_QFIFO_SEND_UNDO(BFX_QFIFO *pFifo) {
    pFifo->headPend = pFifo->headReady;
}

/**
 * @brief Acquire the receive buffer of the FIFO without split.
 * 
 * @param pFifo          [IN]  [BFX_QFIFO *] Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]  Size of the buffer.
 * @param pData          [OUT] [char *]    Pointer to the buffer.
 * @param uiAcquiredSize [OUT] [uint16_t]  Pointer to the acquired size, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(BFX_QFIFO *pFifo, uint16_t uiSize, char **pData, uint16_t *uiAcquiredSize) {
    if (BFX_QFIFO_UNLIKELY(((pFifo)->tailReady != (pFifo)->tailPend ||
        (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo)) == 0))) {
            *pData = NULL;
            *uiAcquiredSize = 0;
            return;
    }
    *uiAcquiredSize = (uiSize) < (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo)) ?
                        (uiSize) : (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo));
    *pData = &((pFifo)->buf[(pFifo)->tailReady]);
    (pFifo)->tailPend = ((pFifo)->tailReady + (*uiAcquiredSize)) % (pFifo)->size;
}

/**
 * @brief Acquire the receive buffer of the FIFO with split.
 * 
 * @param pFifo          [IN]  [BFX_QFIFO *]   Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]    Size of the buffer.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_RECV_ACQUIRE_SPLIT(BFX_QFIFO *pFifo, uint16_t uiSize, char *ppData[2], uint16_t ppAcquiredSize[2]) {
    if (BFX_QFIFO_UNLIKELY(((pFifo)->tailReady != (pFifo)->tailPend ||
        (BFX_QFIFO_RECV_SIZE(pFifo)) == 0))) {
            ppData[0] = NULL;
            ppData[1] = NULL;
            ppAcquiredSize[0] = 0;
            ppAcquiredSize[1] = 0;
            return;
    }
    uint16_t acquiredSize = (uiSize) < (BFX_QFIFO_RECV_SIZE(pFifo)) ?
                        (uiSize) : (BFX_QFIFO_RECV_SIZE(pFifo));
    if ((acquiredSize) <= BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo)) {
        ppAcquiredSize[0] = (acquiredSize);
        ppAcquiredSize[1] = 0;
        ppData[1] = NULL;
    } else {
        ppAcquiredSize[0] = (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo));
        ppAcquiredSize[1] = (acquiredSize) - (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo));
        ppData[1] = &((pFifo)->buf[0]);
    }
    ppData[0] = &((pFifo)->buf[(pFifo)->tailReady]);
    (pFifo)->tailPend = ((pFifo)->tailReady + (acquiredSize)) % (pFifo)->size;
}

/**
 * @brief Acquire the receive buffer of the FIFO with uncertain length.
 * 
 * @param pFifo          [IN] [BFX_QFIFO *]    Pointer to the FIFO structure.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
static inline void BFX_QFIFO_RECV_ACQUIRE_VARI(BFX_QFIFO *pFifo, char *ppData[2], uint16_t ppAcquiredSize[2]) {
    if (BFX_QFIFO_UNLIKELY((pFifo)->tailReady != (pFifo)->tailPend)) {
            ppData[0] = NULL;
            ppData[1] = NULL;
            ppAcquiredSize[0] = 0;
            ppAcquiredSize[1] = 0;
            return;
    }
    uint16_t noSplitRecvSize = (BFX_QFIFO_RECV_NOSPLIT_SIZE(pFifo));
    uint16_t allRecvSize = (BFX_QFIFO_RECV_SIZE(pFifo));
    ppAcquiredSize[0] = noSplitRecvSize;
    ppAcquiredSize[1] = allRecvSize - noSplitRecvSize;
    ppData[1] = &((pFifo)->buf[0]);
    ppData[0] = &((pFifo)->buf[(pFifo)->tailReady]);
    (pFifo)->tailPend = ((pFifo)->tailReady + allRecvSize) % (pFifo)->size;
}

/**
 * @brief Commit the receive buffer of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 */
static inline void BFX_QFIFO_RECV_COMMIT(BFX_QFIFO *pFifo) {
    pFifo->tailReady = pFifo->tailPend;
}

/**
 * @brief Commit the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 * @param size  [IN] [uint16_t]  Size of the commited length.
 */
static inline void BFX_QFIFO_RECV_COMMIT_VARI(BFX_QFIFO *pFifo, uint16_t size) {
    if (BFX_QFIFO_UNLIKELY((pFifo->tailReady == (pFifo)->tailPend) ||
        (size > (BFX_QFIFO_RECV_VARI_SIZE(pFifo))))) {
        return;
    }
    (pFifo)->tailReady = ((pFifo)->tailReady + size) % (pFifo)->size;
    (pFifo)->tailPend = (pFifo)->tailReady;
}

/**
 * @brief Undo the receive buffer of the FIFO.
 * 
 * @param pFifo [IN] [BFX_QFIFO *] Pointer to the FIFO structure.
 */
static inline void BFX_QFIFO_RECV_UNDO(BFX_QFIFO *pFifo) {
    pFifo->tailPend = pFifo->tailReady;
}

/**
 * @brief Get the byte at the specified index of the FIFO piece.
 * 
 * @param hPiece [IN] [BFX_QFIFO_PIECE *] Pointer to the FIFO piece structure.
 * @param i      [IN] [uint16_t]        Index of the byte to get.
 * @return char Byte at the specified index.
 */
static inline char BFX_QFIFO_PIECE_GET_BYTE(BFX_QFIFO_PIECE *hPiece, uint16_t i) {
    return ((i < hPiece->len[0]) ? hPiece->buf[0][i] : hPiece->buf[1][i - hPiece->len[0]]);
}

#ifdef __cplusplus
}
#endif
#endif