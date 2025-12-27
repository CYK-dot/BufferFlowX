/**
 * @file bfx_l2proto.h
 * @author CYK-Dot
 * @brief Layer 2 Protocol for BFX
 * @version 0.1
 * @date 2025-12-24
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __bfx_l2proto_H__
#define __bfx_l2proto_H__

/* Header import ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "bfx_def.h"

/* Config macros -----------------------------------------------------------------*/

#define BFX_L2_PREAMBLE_BYTE 0xAA
#define BFX_L2_PARAM_CHECK_ENABLE

/* Export macros -----------------------------------------------------------------*/

/* Exported typedef --------------------------------------------------------------*/

typedef void (*BFX_PROTO_FCS_PUT)(const uint8_t *data, size_t len, uint8_t *fcs, uint8_t fcsSize);
typedef void (*BFX_PROTO_HTON)(uint8_t *data, size_t len);
typedef void (*BFX_PROTO_NTOH)(uint8_t *data, size_t len);

/**
 * @brief Layer 2 Protocol Description
 * @note L2 frame:
 * || Preamble || usr | len || data || FCS ||
 */
typedef struct tagBFX_PROTO_L2_DESC {
    BFX_PROTO_FCS_PUT fcsCalc;
    BFX_PROTO_HTON hton; ///< net should always use big endian
    BFX_PROTO_NTOH ntoh;
    uint8_t preambleByteCnt;
    uint8_t headByteCnt;
    uint8_t lenBitCnt;
    uint8_t fcsByteCnt;
} BFX_PROTO_L2_DESC;

/**
 * @brief Layer 2 RX Buffer
 */
typedef struct tagBFX_PROTO_L2_RX_BUFFER {
    uint8_t *buf;
    uint16_t bufSize;
    uint16_t nextOffset;
    uint8_t status;
} BFX_PROTO_L2_RX_BUFFER;

/**
 * @brief Layer 2 Packet Payload
 */
typedef struct tagBFX_PROTO_L2_PKT {
    uint8_t *data;     ///< @note reference to rkPkt in rxBuffer
    uint16_t dataLen;
    uint8_t usr;
} BFX_PROTO_L2_PKT;

typedef enum tagBFX_PROTO_L2_EVENT {
    BFX_PROTOL2_EVENT_NONE = 0,
    BFX_PROTOL2_EVENT_PARAM_ERROR = 1,
    BFX_PROTOL2_EVENT_ENCODED_PKT = 2,
    BFX_PROTOL2_EVENT_DROP_SYNC_ERROR = 3,
    BFX_PROTOL2_EVENT_DROP_TOO_LONG = 4,
    BFX_PROTOL2_EVENT_DROP_FCS_ERROR = 5,
} BFX_PROTO_L2_EVENT;

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

/**
 * @brief Get the L2 Packet Length
 * 
 * @param desc L2 Protocol Description
 * @param dataLen Data Length
 * @return uint16_t L2 Packet Length
 */
BFX_STATIC_INLINE uint16_t BFX_ProtoL2GetPktLen(const BFX_PROTO_L2_DESC *desc, uint16_t dataLen)
{
    return desc->preambleByteCnt + desc->headByteCnt + dataLen + desc->fcsByteCnt;
}

/**
 * @brief Get the Max L2 Packet Length
 * 
 * @param desc L2 Protocol Description
 * @return uint16_t Max L2 Packet Length
 */
BFX_STATIC_INLINE uint16_t BFX_ProtoL2GetMaxPktLen(const BFX_PROTO_L2_DESC *desc)
{
    return BFX_ProtoL2GetPktLen(desc, (1 << desc->lenBitCnt) - 1);
}

/**
 * @brief Setup L2 RX Buffer
 * 
 * @param rxBuffer L2 RX Buffer
 * @param buf RX Buffer
 * @param bufSize RX Buffer Size
 */
BFX_STATIC_INLINE void BFX_ProtoL2SetupRxBuffer(BFX_PROTO_L2_RX_BUFFER *rxBuffer, uint8_t *buf, uint16_t bufSize)
{
    rxBuffer->buf = buf;
    rxBuffer->bufSize = bufSize;
    rxBuffer->nextOffset = 0;
    rxBuffer->status = 0;
}

uint16_t BFX_ProtoL2Encode(const BFX_PROTO_L2_DESC *desc,
    const BFX_PROTO_L2_PKT *payload,
    uint8_t *outBuf, uint16_t outMaxSize);

BFX_PROTO_L2_EVENT BFX_ProtoL2Decode(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload);

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif