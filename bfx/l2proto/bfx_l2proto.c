/**
 * @file bfx_l2proto.c
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-27
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "bfx_l2proto.h"

/* Private typedef ----------------------------------------------------------------*/

typedef enum tagBFX_PROTOL2_DECODE_EVENT {
    BFX_PROTOL2_DECODING = 0,
    BFX_PROTOL2_DECODED = 1,
    BFX_PROTOL2_DECODE_FAIL = 2,
} BFX_PROTOL2_DECODE_EVENT;

typedef enum tagBFX_PROTOL2_STATUS {
    BFX_PROTOL2_SETUP = 0,
    BFX_PROTOL2_PREAMBLE = 0,
    BFX_PROTOL2_HEAD = 1,
    BFX_PROTOL2_DATA = 2,
    BFX_PROTOL2_FCS = 3,
} BFX_PROTOL2_STATUS;

/* Private defines ----------------------------------------------------------------*/

/**
 * @brief Get the Bits to Bytes object
 * 
 * @param uiBitCnt Bit Count
 * @return uint8_t Byte Count
 */
#define BFX_GET_BITS_TO_BYTES(uiBitCnt) (((uiBitCnt) + 7) / 8)

/**
 * @brief using a Byte to overwrite the High Bits of Another Byte
 * 
 * @param ucByteToWrite Byte to Write
 * @param ucHighByte High Byte
 * @param ucHighBitCnt High Bit Count
 * 
 * @note 
 * example1: ucByteToWrite=0x0F, ucHighByte=0x0A, ucHighBitCnt=4,
 *           ucByteToWrite will be 0xAF.
 * example2: ucByteToWrite=0x1F, ucHighByte=0x0A, ucHighBitCnt=4,
 *          ucByteToWrite will be 0x0F.
 */
#define BFX_OVERWRITE_HIGH_BITS(ucByteToWrite, ucHighByte, ucHighBitCnt) do { \
    (ucByteToWrite) &= ~((1 << (ucHighBitCnt)) - 1);                          \
    (ucByteToWrite) |= ((ucHighByte) << (8 - (ucHighBitCnt)));                \
} while (0)

/**
 * @brief Strip the High Bits from a Byte
 * 
 * @param ucByteToStrip Byte to Strip
 * @param ucHighByte High Byte
 * @param ucHighBitCnt High Bit Count
 * 
 * @note 
 * example: ucByteToStrip=0xAF, ucHighBitCnt=4,
 *          ucByteToStrip will be 0x0F, ucHighByte will be 0x0A.
 */
#define BFX_STRIP_HIGH_BITS(ucByteToStrip, ucHighByte, ucHighBitCnt) do { \
    ucHighByte = (ucByteToStrip) >> (8 - (ucHighBitCnt));                 \
    (ucByteToStrip) &= ((1 << (8 - (ucHighBitCnt))) - 1);                 \
} while (0)

#define BFX_PROTOL2_RESET_RXBUFFER(rxBuffer) \
    rxBuffer->nextOffset = 0

/* Global variables ---------------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------*/

/* Exported function prototypes --------------------------------------------------*/

/* Private function definitions --------------------------------------------------*/

BFX_STATIC_INLINE uint8_t BFX_ProtoL2Decode_Preamble(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload)
{
    if (rxByte != BFX_L2_PREAMBLE_BYTE) {
        return BFX_PROTOL2_DECODE_FAIL;
    }

    rxBuffer->nextOffset++;
    if (rxBuffer->nextOffset >= desc->preambleByteCnt) {
        payload->data = NULL;
        payload->dataLen = 0;
        payload->usr = 0;
        return BFX_PROTOL2_DECODED;
    }
    return BFX_PROTOL2_DECODING;
}

BFX_STATIC_INLINE uint8_t BFX_ProtoL2Decode_Head(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload)
{
    /* fill head filed */
    rxBuffer->buf[rxBuffer->nextOffset] = rxByte;
    rxBuffer->nextOffset++;
    if (rxBuffer->nextOffset < desc->headByteCnt) {
        return BFX_PROTOL2_DECODING;
    }
    /* get usr field from head */
    uint8_t headIndex = rxBuffer->nextOffset - desc->headByteCnt;
    BFX_STRIP_HIGH_BITS(rxBuffer->buf[headIndex], payload->usr, 8 - desc->lenBitCnt % 8);
    /* get dataLen field from head */
    uint8_t dataLenBytes = (desc->lenBitCnt + 7) / 8;
    desc->ntoh(rxBuffer->buf, desc->headByteCnt);
    memcpy(&payload->dataLen, &rxBuffer->buf[headIndex], dataLenBytes);
    /* check dataLen field */
    if (payload->dataLen >= (1 << desc->lenBitCnt)) {
        rxBuffer->status = 0;
        rxBuffer->nextOffset = 0;
        return BFX_PROTOL2_DECODE_FAIL;
    }
    return BFX_PROTOL2_DECODED;
}

BFX_STATIC_INLINE uint8_t BFX_ProtoL2Decode_Data(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload)
{
    rxBuffer->buf[rxBuffer->nextOffset] = rxByte;
    rxBuffer->nextOffset++;
    if (rxBuffer->nextOffset < payload->dataLen) {
        return BFX_PROTOL2_DECODING;
    }
    return BFX_PROTOL2_DECODED;
}

BFX_STATIC_INLINE uint8_t BFX_ProtoL2Decode_Fcs(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload)
{
    rxBuffer->buf[rxBuffer->nextOffset] = rxByte;
    rxBuffer->nextOffset++;
    if (rxBuffer->nextOffset < (payload->dataLen + desc->fcsByteCnt)) {
        return BFX_PROTOL2_DECODING;
    }

    desc->ntoh(&rxBuffer->buf[payload->dataLen], desc->fcsByteCnt);
    desc->fcsCalc(rxBuffer->buf,
        payload->dataLen,
        &rxBuffer->buf[payload->dataLen + desc->fcsByteCnt],
        desc->fcsByteCnt
    );
    if (memcmp(&rxBuffer->buf[payload->dataLen], 
                &rxBuffer->buf[payload->dataLen + desc->fcsByteCnt], 
                desc->fcsByteCnt) != 0) {
        return BFX_PROTOL2_DECODE_FAIL;
    }
    return BFX_PROTOL2_DECODED;
}

/* Exported function definitions -------------------------------------------------*/

/**
 * @brief Encode L2 Packet
 * 
 * @param[in] desc L2 Protocol Description
 * @param[in] payload L2 Packet Payload
 * @param[out] outBuf Output Buffer
 * @param[in] outMaxSize Output Buffer Max Size
 * @return uint16_t Packet Length
 */
uint16_t BFX_ProtoL2Encode(const BFX_PROTO_L2_DESC *desc,
    const BFX_PROTO_L2_PKT *payload,
    uint8_t *outBuf, uint16_t outMaxSize)
{
    #ifdef BFX_L2_PARAM_CHECK_ENABLE
    if (BFX_UNLIKELY(desc == NULL || outBuf == NULL || payload == NULL)) {
        return BFX_PROTOL2_EVENT_PARAM_ERROR;
    }
    uint16_t pktLen = BFX_ProtoL2GetPktLen(desc, payload->dataLen);
    if (BFX_UNLIKELY(outMaxSize < pktLen)) {
        return BFX_PROTOL2_EVENT_PARAM_ERROR;
    }
    if (BFX_UNLIKELY(payload->dataLen >= (1 << desc->lenBitCnt))) {
        return BFX_PROTOL2_EVENT_PARAM_ERROR;
    }
    #endif
    uint16_t idx = 0;
    /* preamble field */
    memset(outBuf, BFX_L2_PREAMBLE_BYTE, desc->preambleByteCnt);
    idx += desc->preambleByteCnt;

    /* head field */
    memset(&outBuf[idx], 0, desc->headByteCnt);
    uint8_t lenByteCnt = BFX_GET_BITS_TO_BYTES(desc->lenBitCnt);
    uint8_t *headField = &outBuf[idx];
    memcpy(headField, (uint8_t *)&payload->dataLen, lenByteCnt);
    desc->hton(headField, lenByteCnt);
    BFX_OVERWRITE_HIGH_BITS(headField[0], payload->usr, 8 - desc->lenBitCnt % 8);
    idx += desc->headByteCnt;

    /* data field */
    memcpy(&outBuf[idx], payload->data, payload->dataLen);
    idx += payload->dataLen;

    /* fcs field */
    desc->fcsCalc(payload->data, payload->dataLen, &outBuf[idx], desc->fcsByteCnt);
    desc->hton(&outBuf[idx], desc->fcsByteCnt);
    idx += desc->fcsByteCnt;
    return idx;
}

/**
 * @brief Decode L2 Packet
 * 
 * @param[in] desc L2 Protocol Description
 * @param[in] rxByte Received Byte
 * @param[in] rxBuffer L2 RX Buffer
 * @param[out] payload L2 Packet Payload
 * @return BFX_PROTO_L2_EVENT
 */
BFX_PROTO_L2_EVENT BFX_ProtoL2Decode(const BFX_PROTO_L2_DESC *desc,
    uint8_t rxByte, BFX_PROTO_L2_RX_BUFFER *rxBuffer,
    BFX_PROTO_L2_PKT *payload)
{
    #ifdef BFX_L2_PARAM_CHECK_ENABLE
    if (BFX_UNLIKELY(desc == NULL || rxBuffer == NULL || payload == NULL)) {
        return BFX_PROTOL2_EVENT_PARAM_ERROR;
    }
    #endif

    switch (rxBuffer->status) {
        uint8_t event;
        case BFX_PROTOL2_PREAMBLE: {
            event = BFX_ProtoL2Decode_Preamble(desc, rxByte, rxBuffer, payload);
            if (event == BFX_PROTOL2_DECODED) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                rxBuffer->status = BFX_PROTOL2_HEAD;
            } else if (event == BFX_PROTOL2_DECODE_FAIL) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                return BFX_PROTOL2_EVENT_DROP_SYNC_ERROR;
            }
        } break;
        case BFX_PROTOL2_HEAD: {
            event = BFX_ProtoL2Decode_Head(desc, rxByte, rxBuffer, payload);
            if (event == BFX_PROTOL2_DECODED) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                rxBuffer->status = BFX_PROTOL2_DATA;
            } else if (event == BFX_PROTOL2_DECODE_FAIL) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                rxBuffer->status = BFX_PROTOL2_PREAMBLE;
                return BFX_PROTOL2_EVENT_DROP_TOO_LONG;
            }
        } break;
        case BFX_PROTOL2_DATA: {
            event = BFX_ProtoL2Decode_Data(desc, rxByte, rxBuffer, payload);
            if (event == BFX_PROTOL2_DECODED) {
                rxBuffer->status = BFX_PROTOL2_FCS;
            }
        } break;
        case BFX_PROTOL2_FCS: {
            event = BFX_ProtoL2Decode_Fcs(desc, rxByte, rxBuffer, payload);
            if (event == BFX_PROTOL2_DECODED) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                rxBuffer->status = BFX_PROTOL2_PREAMBLE;
                payload->data = rxBuffer->buf;
                return BFX_PROTOL2_EVENT_ENCODED_PKT;
            } else if (event == BFX_PROTOL2_DECODE_FAIL) {
                BFX_PROTOL2_RESET_RXBUFFER(rxBuffer);
                rxBuffer->status = BFX_PROTOL2_PREAMBLE;
                return BFX_PROTOL2_EVENT_DROP_FCS_ERROR;
            }
        } break;
    }
    return BFX_PROTOL2_EVENT_NONE;
}