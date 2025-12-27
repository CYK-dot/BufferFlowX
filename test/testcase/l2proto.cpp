/**
 * @file proto_l2.cpp
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-26
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>

#include "bfx_l2proto.h"

/* Helper functions ---------------------------------------------------------------*/

/* Config macros ------------------------------------------------------------------*/

/* Mock variables and functions  --------------------------------------------------*/

void TestFcsCalc(const uint8_t *data, size_t len, uint8_t *fcs, uint8_t fcsSize) {
    uint8_t result = 0;
    for (size_t i = 0; i < len; i++) {
        result += data[i];
    }
    fcs[0] = result;
}

void TestHton(uint8_t *data, size_t len) {
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t tmp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = tmp;
    }
}

void TestNtoh(uint8_t *data, size_t len) {
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t tmp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = tmp;
    }
}

/* Helper functions ---------------------------------------------------------------*/

BFX_PROTO_L2_DESC createDefaultL2Desc() {
    BFX_PROTO_L2_DESC desc = {
        .fcsCalc = TestFcsCalc,
        .hton = TestHton,
        .ntoh = TestNtoh,
        .preambleByteCnt = 1,
        .headByteCnt = 2,
        .lenBitCnt = 12,
        .fcsByteCnt = 1
    };
    return desc;
}

BFX_PROTO_L2_PKT createTestPayload(uint8_t *testData, size_t dataLen, uint8_t usr) {
    BFX_PROTO_L2_PKT payload = {
        .data = testData,
        .dataLen = static_cast<uint16_t>(dataLen),
        .usr = usr
    };
    return payload;
}

uint16_t encodeTestPacket(const BFX_PROTO_L2_DESC *desc, const BFX_PROTO_L2_PKT *payload, uint8_t *encodedBuf, size_t bufSize) {
    memset(encodedBuf, 0xFF, bufSize);
    return BFX_ProtoL2Encode(desc, payload, encodedBuf, static_cast<uint16_t>(bufSize));
}

BFX_PROTO_L2_RX_BUFFER createRxBuffer(uint8_t *buf, size_t bufSize) {
    BFX_PROTO_L2_RX_BUFFER rxBuffer = {0};
    BFX_ProtoL2SetupRxBuffer(&rxBuffer, buf, static_cast<uint16_t>(bufSize));
    return rxBuffer;
}

bool decodePacket(const BFX_PROTO_L2_DESC *desc, const uint8_t *encodedData, uint16_t encodedLen, 
                 BFX_PROTO_L2_RX_BUFFER *rxBuffer, BFX_PROTO_L2_PKT *decodedPayload) {
    BFX_PROTO_L2_EVENT event;
    bool packetDecoded = false;
    
    for (int i = 0; i < encodedLen; i++) {
        event = BFX_ProtoL2Decode(desc, encodedData[i], rxBuffer, decodedPayload);
        if (event == BFX_PROTOL2_EVENT_ENCODED_PKT) {
            packetDecoded = true;
            break;
        }
    }
    
    return packetDecoded;
}

#define compareDecodeAssert(decodedPayload, payload) do { \
    EXPECT_EQ(decodedPayload.dataLen, payload.dataLen); \
    EXPECT_EQ(decodedPayload.usr, payload.usr); \
    for (int i = 0; i < payload.dataLen; i++) { \
        EXPECT_EQ(decodedPayload.data[i], payload.data[i]); \
    } \
} while (0)

/* Test cases ---------------------------------------------------------------------*/

TEST(L2ProtoTest, EncodeOverflowDetect) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode tx pkt */
    uint8_t encodedBuf[100];
    memset(encodedBuf, 0xFF, sizeof(encodedBuf));
    uint16_t encodedLen = encodeTestPacket(&desc, &payload, &encodedBuf[1], sizeof(encodedBuf) - 2);
    EXPECT_GT(encodedLen, sizeof(testData));
    /* overflow detect */
    EXPECT_EQ(encodedBuf[0], 0xFF);
    EXPECT_EQ(encodedBuf[encodedLen + 1], 0xFF);
}

TEST(L2ProtoTest, EncodeDecodeNormalFlow) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode tx pkt */
    uint8_t encodedBuf[100];
    uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
    EXPECT_GT(encodedLen, sizeof(testData));
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    /* decode rx pkt */
    BFX_PROTO_L2_PKT decodedPayload = {0};
    bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
    ASSERT_TRUE(packetDecoded);
    compareDecodeAssert(decodedPayload, payload);
}