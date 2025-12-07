/**
 * @file em_fifo.h
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
#ifndef __em_fifo_H__
#define __em_fifo_H__
#ifdef __cplusplus
extern "C" {
#endif

/* Header import ------------------------------------------------------------------*/
#include "stdint.h"
#include "em_types.h"

/* Config macros -----------------------------------------------------------------*/

/* Exported typedef --------------------------------------------------------------*/

/**
 * @brief SPSC asyn ring-fifo structure.
 */
typedef struct EM_FIFO {
    char *buf;
    int16_t size;
    uint16_t headReady;
    uint16_t tailReady;
    uint16_t headPend;
    uint16_t tailPend;
} EM_FIFO;

typedef struct tagEM_FIFO_PIECE {
    char *buf[2];
    uint16_t len[2];
} EM_FIFO_PIECE;

/* Export macros -----------------------------------------------------------------*/

/**
 * @brief Initialize the FIFO.
 * 
 * @param pFifo  [OUT] [EM_FIFO *] Pointer to the FIFO structure.
 * @param pBuf   [IN]  [char *]    Pointer to the buffer.
 * @param uiSize [IN]  [uint16_t]  Size of the buffer.
 */
#define EM_FIFO_INIT(pFifo, pBuf, uiSize) do { \
    (pFifo)->buf = (pBuf);                     \
    (pFifo)->size = (uiSize);                  \
    (pFifo)->headReady = 0;                    \
    (pFifo)->tailReady = 0;                    \
    (pFifo)->headPend = 0;                     \
    (pFifo)->tailPend = 0;                     \
} while(0)

/**
 * @brief Get the free size of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO.
 */
#define EM_FIFO_FREE_SIZE(pFifo)                                \
    ((pFifo)->tailReady > (pFifo)->headPend ?                   \
    (pFifo)->tailReady - (pFifo)->headPend - 1 :                \
    (pFifo)->size - ((pFifo)->headPend - (pFifo)->tailReady) - 1)

/**
 * @brief Get the free size of the FIFO without split.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO without split.
 */
#define EM_FIFO_FREE_NOSPLIT_SIZE(pFifo)                                 \
    ((pFifo)->tailReady > (pFifo)->headPend ?                            \
    (pFifo)->tailReady - (pFifo)->headPend - 1 :                         \
    (pFifo)->size - (pFifo)->headPend - ((pFifo)->tailReady == 0 ? 1 : 0))

/**
 * @brief Get the free size of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Free size of the FIFO with uncertain length.
 */
#define EM_FIFO_FREE_VARI_SIZE(pFifo)                            \
    ((pFifo)->tailReady > (pFifo)->headReady ?                   \
    (pFifo)->tailReady - (pFifo)->headReady - 1 :                \
    (pFifo)->size - ((pFifo)->headReady - (pFifo)->tailReady) - 1)

/**
 * @brief Get the received size of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO.
 */
#define EM_FIFO_RECV_SIZE(pFifo)                            \
    ((pFifo)->headReady >= (pFifo)->tailPend ?              \
    (pFifo)->headReady - (pFifo)->tailPend :                \
    (pFifo)->size - ((pFifo)->tailPend - (pFifo)->headReady))

/**
 * @brief Get the received size of the FIFO without split.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO without split.
 */
#define EM_FIFO_RECV_NOSPLIT_SIZE(pFifo)       \
    ((pFifo)->headReady >= (pFifo)->tailPend ? \
     (pFifo)->headReady - (pFifo)->tailPend :  \
     (pFifo)->size - (pFifo)->tailPend)

/**
 * @brief Get the received size of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @return [uint16_t] Received size of the FIFO with uncertain length.
 */
#define EM_FIFO_RECV_VARI_SIZE(pFifo)                        \
    ((pFifo)->headReady >= (pFifo)->tailReady ?              \
    (pFifo)->headReady - (pFifo)->tailReady :                \
    (pFifo)->size - ((pFifo)->tailReady - (pFifo)->headReady))

/**
 * @brief Acquire the send buffer of the FIFO without split.
 * 
 * @param pFifo          [IN]  [EM_FIFO *] Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]  Size of the buffer.
 * @param pData          [OUT] [char *]    Pointer to the buffer.
 * @param uiAcquiredSize [OUT] [uint16_t]  Pointer to the acquired size, can be smaller than uiSize.
 */
#define EM_FIFO_SEND_ACQUIRE_NOSPLIT(pFifo, uiSize, pData, uiAcquiredSize)       \
do {                                                                             \
    if (EM_UNLIKELY(((pFifo)->headReady != (pFifo)->headPend) ||                 \
        (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo)) == 0)) {                              \
            (pData) = NULL;                                                      \
            (uiAcquiredSize) = 0;                                                \
            break;                                                               \
    }                                                                            \
    (uiAcquiredSize) = (uiSize) < (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo)) ?           \
                        (uiSize) : (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo));           \
    (pData) = &((pFifo)->buf[(pFifo)->headReady]);                               \
    (pFifo)->headPend = ((pFifo)->headReady + (uiAcquiredSize)) % (pFifo)->size; \
} while(0)

/**
 * @brief Acquire the send buffer of the FIFO with split.
 * 
 * @param pFifo          [IN] [EM_FIFO *]    Pointer to the FIFO structure.
 * @param uiSize         [IN] [uint16_t]     Size of the buffer.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
#define EM_FIFO_SEND_ACQUIRE_SPLIT(pFifo, uiSize, ppData, ppAcquiredSize)           \
do {                                                                                \
    if (EM_UNLIKELY(((pFifo)->headReady != (pFifo)->headPend) ||                    \
        (EM_FIFO_FREE_SIZE(pFifo)) == 0)) {                                         \
            (ppData)[0] = NULL;                                                     \
            (ppData)[1] = NULL;                                                     \
            (ppAcquiredSize)[0] = 0;                                                \
            (ppAcquiredSize)[1] = 0;                                                \
            break;                                                                  \
    }                                                                               \
    uint16_t acquiredSize = (uiSize) < (EM_FIFO_FREE_SIZE(pFifo)) ?                 \
                            (uiSize) : (EM_FIFO_FREE_SIZE(pFifo));                  \
    if ((acquiredSize) <= (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo))) {                     \
        (ppAcquiredSize)[0] = (acquiredSize);                                       \
        (ppAcquiredSize)[1] = 0;                                                    \
        (ppData)[1] = NULL;                                                         \
    } else {                                                                        \
        (ppAcquiredSize)[0] = (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo));                   \
        (ppAcquiredSize)[1] = (acquiredSize) - (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo));  \
        (ppData)[1] = &((pFifo)->buf[0]);                                           \
    }                                                                               \
    (ppData)[0] = &((pFifo)->buf[(pFifo)->headReady]);                              \
    (pFifo)->headPend = ((pFifo)->headReady + (acquiredSize)) % (pFifo)->size;      \
} while(0)

/**
 * @brief Acquire the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo          [IN] [EM_FIFO *]    Pointer to the FIFO structure.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
#define EM_FIFO_SEND_ACQUIRE_VARI(pFifo, ppData, ppAcquiredSize)            \
do {                                                                        \
    if (EM_UNLIKELY((pFifo)->headReady != (pFifo)->headPend)) {             \
            (ppData)[0] = NULL;                                             \
            (ppData)[1] = NULL;                                             \
            (ppAcquiredSize)[0] = 0;                                        \
            (ppAcquiredSize)[1] = 0;                                        \
            break;                                                          \
    }                                                                       \
    uint16_t noSplitFreeSize = (EM_FIFO_FREE_NOSPLIT_SIZE(pFifo));          \
    uint16_t allFreeSize = (EM_FIFO_FREE_SIZE(pFifo));                      \
    (ppAcquiredSize)[0] = noSplitFreeSize;                                  \
    (ppAcquiredSize)[1] = allFreeSize - noSplitFreeSize;                    \
    (ppData)[1] = &((pFifo)->buf[0]);                                       \
    (ppData)[0] = &((pFifo)->buf[(pFifo)->headReady]);                      \
    (pFifo)->headPend = ((pFifo)->headReady + allFreeSize) % (pFifo)->size; \
} while(0)

/**
 * @brief Commit the send buffer of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 */
#define EM_FIFO_SEND_COMMIT(pFifo) \
    (pFifo)->headReady = (pFifo)->headPend

/**
 * @brief Commit the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo  [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @param uiSize [IN] [uint16_t]  Size of the commited length.
 */
#define EM_FIFO_SEND_COMMIT_VARI(pFifo, uiSize)                         \
do {                                                                    \
    if (EM_UNLIKELY((pFifo)->headReady == (pFifo)->headPend ||          \
        (uiSize > (EM_FIFO_FREE_VARI_SIZE(pFifo))))) {                  \
        break;                                                          \
    }                                                                   \
    (pFifo)->headReady = ((pFifo)->headReady + uiSize) % (pFifo)->size; \
    (pFifo)->headPend = (pFifo)->headReady;                             \
} while(0)

/**
 * @brief Undo the send buffer of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 */
#define EM_FIFO_SEND_UNDO(pFifo)         \
    (pFifo)->headPend = (pFifo)->headReady

/**
 * @brief Acquire the receive buffer of the FIFO without split.
 * 
 * @param pFifo          [IN]  [EM_FIFO *] Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]  Size of the buffer.
 * @param pData          [OUT] [char *]    Pointer to the buffer.
 * @param uiAcquiredSize [OUT] [uint16_t]  Pointer to the acquired size, can be smaller than uiSize.
 */
#define EM_FIFO_RECV_ACQUIRE_NOSPLIT(pFifo, uiSize, pData, uiAcquiredSize)       \
do {                                                                             \
    if (EM_UNLIKELY(((pFifo)->tailReady != (pFifo)->tailPend ||                  \
        (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo)) == 0))) {                             \
            (pData) = NULL;                                                      \
            (uiAcquiredSize) = 0;                                                \
            break;                                                               \
    }                                                                            \
    (uiAcquiredSize) = (uiSize) < (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo)) ?           \
                        (uiSize) : (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo));           \
    (pData) = &((pFifo)->buf[(pFifo)->tailReady]);                               \
    (pFifo)->tailPend = ((pFifo)->tailReady + (uiAcquiredSize)) % (pFifo)->size; \
} while(0)

/**
 * @brief Acquire the receive buffer of the FIFO with split.
 * 
 * @param pFifo          [IN]  [EM_FIFO *]   Pointer to the FIFO structure.
 * @param uiSize         [IN]  [uint16_t]    Size of the buffer.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
#define EM_FIFO_RECV_ACQUIRE_SPLIT(pFifo, uiSize, ppData, ppAcquiredSize)           \
do {                                                                                \
    if (EM_UNLIKELY(((pFifo)->tailReady != (pFifo)->tailPend ||                     \
        (EM_FIFO_RECV_SIZE(pFifo)) == 0))) {                                        \
            (ppData)[0] = NULL;                                                     \
            (ppData)[1] = NULL;                                                     \
            (ppAcquiredSize)[0] = 0;                                                \
            (ppAcquiredSize)[1] = 0;                                                \
            break;                                                                  \
    }                                                                               \
    uint16_t acquiredSize = (uiSize) < (EM_FIFO_RECV_SIZE(pFifo)) ?                 \
                        (uiSize) : (EM_FIFO_RECV_SIZE(pFifo));                      \
    if ((acquiredSize) <= EM_FIFO_RECV_NOSPLIT_SIZE(pFifo)) {                       \
        (ppAcquiredSize)[0] = (acquiredSize);                                       \
        (ppAcquiredSize)[1] = 0;                                                    \
        (ppData)[1] = NULL;                                                         \
    } else {                                                                        \
        (ppAcquiredSize)[0] = (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo));                   \
        (ppAcquiredSize)[1] = (acquiredSize) - (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo));  \
        (ppData)[1] = &((pFifo)->buf[0]);                                           \
    }                                                                               \
    (ppData)[0] = &((pFifo)->buf[(pFifo)->tailReady]);                              \
    (pFifo)->tailPend = ((pFifo)->tailReady + (acquiredSize)) % (pFifo)->size;      \
} while(0)

/**
 * @brief Acquire the receive buffer of the FIFO with uncertain length.
 * 
 * @param pFifo          [IN] [EM_FIFO *]    Pointer to the FIFO structure.
 * @param ppData         [OUT] [char *[2]]   Pointer to the buffer array.
 * @param ppAcquiredSize [OUT] [uint16_t[2]] Pointer to the acquired size array, can be smaller than uiSize.
 */
#define EM_FIFO_RECV_ACQUIRE_VARI(pFifo, ppData, ppAcquiredSize)            \
do {                                                                        \
    if (EM_UNLIKELY((pFifo)->tailReady != (pFifo)->tailPend)) {             \
            (ppData)[0] = NULL;                                             \
            (ppData)[1] = NULL;                                             \
            (ppAcquiredSize)[0] = 0;                                        \
            (ppAcquiredSize)[1] = 0;                                        \
            break;                                                          \
    }                                                                       \
    uint16_t noSplitRecvSize = (EM_FIFO_RECV_NOSPLIT_SIZE(pFifo));          \
    uint16_t allRecvSize = (EM_FIFO_RECV_SIZE(pFifo));                      \
    (ppAcquiredSize)[0] = noSplitRecvSize;                                  \
    (ppAcquiredSize)[1] = allRecvSize - noSplitRecvSize;                    \
    (ppData)[1] = &((pFifo)->buf[0]);                                       \
    (ppData)[0] = &((pFifo)->buf[(pFifo)->tailReady]);                      \
    (pFifo)->tailPend = ((pFifo)->tailReady + allRecvSize) % (pFifo)->size; \
} while(0)

/**
 * @brief Commit the receive buffer of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 */
#define EM_FIFO_RECV_COMMIT(pFifo)       \
    (pFifo)->tailReady = (pFifo)->tailPend

/**
 * @brief Commit the send buffer of the FIFO with uncertain length.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 * @param size  [IN] [uint16_t]  Size of the commited length.
 */
#define EM_FIFO_RECV_COMMIT_VARI(pFifo, size)                           \
do {                                                                    \
    if (EM_UNLIKELY((pFifo->tailReady == (pFifo)->tailPend) ||          \
        (size > (EM_FIFO_RECV_VARI_SIZE(pFifo))))) {                    \
        break;                                                          \
    }                                                                   \
    (pFifo)->tailReady = ((pFifo)->tailReady + size) % (pFifo)->size;   \
    (pFifo)->tailPend = (pFifo)->tailReady;                             \
} while(0)

/**
 * @brief Undo the receive buffer of the FIFO.
 * 
 * @param pFifo [IN] [EM_FIFO *] Pointer to the FIFO structure.
 */
#define EM_FIFO_RECV_UNDO(pFifo)        \
    (pFifo)->tailPend = (pFifo)->tailReady

/**
 * @brief Get the byte at the specified index of the FIFO piece.
 * 
 * @param hPiece [IN] [EM_FIFO_PIECE *] Pointer to the FIFO piece structure.
 * @param i      [IN] [uint16_t]        Index of the byte to get.
 * @return char Byte at the specified index.
 */
#define EM_FIFO_PIECE_GET_BYTE(hPiece, i) \
    (((i) < hPiece->len[0]) ? hPiece->buf[0][(i)] : hPiece->buf[1][(i) - hPiece->len[0]])

#ifdef __cplusplus
}
#endif
#endif