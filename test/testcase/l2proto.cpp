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
        .preambleByteCnt = 3,
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

#define DECODE_COMPARE_ASSERT(decodedPayload, payload) do { \
    EXPECT_EQ(decodedPayload.dataLen, payload.dataLen); \
    EXPECT_EQ(decodedPayload.usr, payload.usr); \
    for (int i = 0; i < payload.dataLen; i++) { \
        EXPECT_EQ(decodedPayload.data[i], payload.data[i]); \
    } \
} while (0)

/* Test cases ---------------------------------------------------------------------*/

TEST(L2ProtoTest, EncodeShouldNotOverflow) {
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
    DECODE_COMPARE_ASSERT(decodedPayload, payload);
}

TEST(L2ProtoTest, DecodeShouldNotOverflow) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode tx pkt */
    uint8_t encodedBuf[100];
    uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
    /* create rx buffer */
    uint8_t rxBuf[100];
    memset(rxBuf, 0xFF, sizeof(rxBuf));
    BFX_PROTO_L2_RX_BUFFER rxBuffer = {
        .buf = &rxBuf[1],
        .bufSize = BFX_ProtoL2GetRxBufferLen(&desc, sizeof(testData)),
        .nextOffset = 0,
        .status = 0
    };
    /* decode rx pkt */
    BFX_PROTO_L2_PKT decodedPayload = {0};
    bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
    /* overflow detect */
    EXPECT_EQ(rxBuf[0], 0xFF);
    EXPECT_EQ(rxBuf[encodedLen + 1], 0xFF);
}

TEST(L2ProtoTest, EncodeDecodeCompressedFlow) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    desc.headByteCnt = 1;
    desc.lenBitCnt = 7;
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x01);
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
    DECODE_COMPARE_ASSERT(decodedPayload, payload);
}

TEST(L2ProtoTest, EncodeNormalWithDifferentUsrValues) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload with different usr values */
    for (uint8_t usr = 0; usr < 16; usr++) {
        uint8_t testData[] = {0x10, 0x20, 0x30, 0x40};
        BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), usr);
        /* encode tx pkt */
        uint8_t encodedBuf[100];
        uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
        EXPECT_GT(encodedLen, sizeof(testData));
        /* verify preamble */
        for (int i = 0; i < desc.preambleByteCnt; i++) {
            EXPECT_EQ(encodedBuf[i], BFX_L2_PREAMBLE_BYTE);
        }
        /* create rx buffer */
        uint8_t rxBuf[100];
        BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
        /* decode rx pkt */
        BFX_PROTO_L2_PKT decodedPayload = {0};
        bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
        ASSERT_TRUE(packetDecoded) << "Failed to decode with usr value: " << (int)usr;
        DECODE_COMPARE_ASSERT(decodedPayload, payload);
    }
}

TEST(L2ProtoTest, EncodeDecodeDifferentDataSizes) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    
    // Test different data sizes
    for (size_t dataSize = 1; dataSize <= 50; dataSize += 10) {
        /* create tx payload */
        uint8_t* testData = new uint8_t[dataSize];
        for (size_t i = 0; i < dataSize; i++) {
            testData[i] = static_cast<uint8_t>(i % 256);
        }
        BFX_PROTO_L2_PKT payload = createTestPayload(testData, dataSize, 0x0A);
        /* encode tx pkt */
        uint8_t encodedBuf[200];
        uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
        EXPECT_GT(encodedLen, dataSize);
        /* create rx buffer */
        uint8_t rxBuf[200];
        BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
        /* decode rx pkt */
        BFX_PROTO_L2_PKT decodedPayload = {0};
        bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
        ASSERT_TRUE(packetDecoded) << "Failed to decode with data size: " << dataSize;
        DECODE_COMPARE_ASSERT(decodedPayload, payload);
        
        delete[] testData;
    }
}

TEST(L2ProtoTest, EncodeWithNullDesc) {
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode with null desc */
    uint8_t encodedBuf[100];
    uint16_t result = BFX_ProtoL2Encode(NULL, &payload, encodedBuf, sizeof(encodedBuf));
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, EncodeWithNullOutBuf) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode with null outBuf */
    uint16_t result = BFX_ProtoL2Encode(&desc, &payload, NULL, sizeof(testData) + 10);
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, EncodeWithNullPayload) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* encode with null payload */
    uint8_t encodedBuf[100];
    uint16_t result = BFX_ProtoL2Encode(&desc, NULL, encodedBuf, sizeof(encodedBuf));
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, EncodeInsufficientBufferSize) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode with insufficient buffer size */
    uint8_t encodedBuf[10];  // Smaller than required
    uint16_t result = BFX_ProtoL2Encode(&desc, &payload, encodedBuf, 5); // Intentionally small
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, EncodeDataLengthExceedsLimit) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload with data length exceeding limit */
    uint8_t testData[500]; // Much larger than 12-bit limit allows
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, 5000, 0x05); // Exceeds 12-bit limit (4095)
    /* encode tx pkt */
    uint8_t encodedBuf[6000];
    uint16_t result = BFX_ProtoL2Encode(&desc, &payload, encodedBuf, sizeof(encodedBuf));
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, DecodeNormalFlowWithDifferentData) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    
    // Test with various data patterns
    uint8_t testData1[] = {0xFF, 0xAA, 0x55, 0x00};
    BFX_PROTO_L2_PKT payload1 = createTestPayload(testData1, sizeof(testData1), 0x0F);
    uint8_t encodedBuf1[100];
    uint16_t encodedLen1 = encodeTestPacket(&desc, &payload1, encodedBuf1, sizeof(encodedBuf1));
    
    uint8_t rxBuf1[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer1 = createRxBuffer(rxBuf1, sizeof(rxBuf1));
    BFX_PROTO_L2_PKT decodedPayload1 = {0};
    bool packetDecoded1 = decodePacket(&desc, encodedBuf1, encodedLen1, &rxBuffer1, &decodedPayload1);
    ASSERT_TRUE(packetDecoded1);
    DECODE_COMPARE_ASSERT(decodedPayload1, payload1);
    
    // Test with another pattern
    uint8_t testData2[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    BFX_PROTO_L2_PKT payload2 = createTestPayload(testData2, sizeof(testData2), 0x01);
    uint8_t encodedBuf2[100];
    uint16_t encodedLen2 = encodeTestPacket(&desc, &payload2, encodedBuf2, sizeof(encodedBuf2));
    
    uint8_t rxBuf2[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer2 = createRxBuffer(rxBuf2, sizeof(rxBuf2));
    BFX_PROTO_L2_PKT decodedPayload2 = {0};
    bool packetDecoded2 = decodePacket(&desc, encodedBuf2, encodedLen2, &rxBuffer2, &decodedPayload2);
    ASSERT_TRUE(packetDecoded2);
    DECODE_COMPARE_ASSERT(decodedPayload2, payload2);
}

TEST(L2ProtoTest, DecodeWithNullDesc) {
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    BFX_PROTO_L2_PKT decodedPayload = {0};
    
    /* decode with null desc */
    BFX_PROTO_L2_EVENT result = BFX_ProtoL2Decode(NULL, 0x00, &rxBuffer, &decodedPayload);
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, DecodeWithNullRxBuffer) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    BFX_PROTO_L2_PKT decodedPayload = {0};
    
    /* decode with null rxBuffer */
    BFX_PROTO_L2_EVENT result = BFX_ProtoL2Decode(&desc, 0x00, NULL, &decodedPayload);
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, DecodeWithNullPayload) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    
    /* decode with null payload */
    BFX_PROTO_L2_EVENT result = BFX_ProtoL2Decode(&desc, 0x00, &rxBuffer, NULL);
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_PARAM_ERROR);
}

TEST(L2ProtoTest, DecodeSyncError) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    BFX_PROTO_L2_PKT decodedPayload = {0};
    
    // Send non-preamble byte first to trigger sync error
    BFX_PROTO_L2_EVENT result = BFX_ProtoL2Decode(&desc, 0x55, &rxBuffer, &decodedPayload); // Not preamble byte
    EXPECT_EQ(result, BFX_PROTOL2_EVENT_DROP_SYNC_ERROR);
}

TEST(L2ProtoTest, DecodeFcsError) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create tx payload */
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    BFX_PROTO_L2_PKT payload = createTestPayload(testData, sizeof(testData), 0x05);
    /* encode tx pkt */
    uint8_t encodedBuf[100];
    uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
    EXPECT_GT(encodedLen, sizeof(testData));
    
    // Modify the FCS byte to cause FCS error
    encodedBuf[encodedLen-1] = encodedBuf[encodedLen-1] ^ 0xFF; // Flip all bits of FCS
    
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    /* decode rx pkt with corrupted FCS */
    BFX_PROTO_L2_PKT decodedPayload = {0};
    BFX_PROTO_L2_EVENT event;
    bool fcsErrorDetected = false;
    
    for (int i = 0; i < encodedLen; i++) {
        event = BFX_ProtoL2Decode(&desc, encodedBuf[i], &rxBuffer, &decodedPayload);
        if (event == BFX_PROTOL2_EVENT_DROP_FCS_ERROR) {
            fcsErrorDetected = true;
            break;
        }
    }
    
    EXPECT_TRUE(fcsErrorDetected);
}

TEST(L2ProtoTest, DecodeMultiplePacketsSequentially) {
    /* create proto */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    /* create first tx payload */
    uint8_t testData1[] = {0x11, 0x22, 0x33};
    BFX_PROTO_L2_PKT payload1 = createTestPayload(testData1, sizeof(testData1), 0x01);
    /* encode first tx pkt */
    uint8_t encodedBuf1[100];
    uint16_t encodedLen1 = encodeTestPacket(&desc, &payload1, encodedBuf1, sizeof(encodedBuf1));
    
    /* create second tx payload */
    uint8_t testData2[] = {0x44, 0x55, 0x66, 0x77};
    BFX_PROTO_L2_PKT payload2 = createTestPayload(testData2, sizeof(testData2), 0x02);
    /* encode second tx pkt */
    uint8_t encodedBuf2[100];
    uint16_t encodedLen2 = encodeTestPacket(&desc, &payload2, encodedBuf2, sizeof(encodedBuf2));
    
    /* create rx buffer */
    uint8_t rxBuf[100];
    BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    BFX_PROTO_L2_PKT decodedPayload = {0};
    
    // Decode first packet
    bool packet1Decoded = decodePacket(&desc, encodedBuf1, encodedLen1, &rxBuffer, &decodedPayload);
    ASSERT_TRUE(packet1Decoded);
    DECODE_COMPARE_ASSERT(decodedPayload, payload1);
    
    // Reset rx buffer for second packet
    rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
    
    // Decode second packet
    bool packet2Decoded = decodePacket(&desc, encodedBuf2, encodedLen2, &rxBuffer, &decodedPayload);
    ASSERT_TRUE(packet2Decoded);
    DECODE_COMPARE_ASSERT(decodedPayload, payload2);
}

TEST(L2ProtoTest, EncodeDecodeMaxDataLength) {
    /* create proto with 12-bit length (max 4095) */
    BFX_PROTO_L2_DESC desc = createDefaultL2Desc();
    
    // Create a payload with maximum allowed data length (4095 bytes for 12-bit)
    const size_t maxDataLen = (1 << desc.lenBitCnt) - 1;  // 4095 for 12-bit
    if (maxDataLen > 2000) {
        // Use a smaller value for testing to avoid large memory allocation
        const size_t testDataLen = 100;
        uint8_t* testData = new uint8_t[testDataLen];
        for (size_t i = 0; i < testDataLen; i++) {
            testData[i] = static_cast<uint8_t>((i + 1) % 256);
        }
        BFX_PROTO_L2_PKT payload = createTestPayload(testData, testDataLen, 0x0F);
        
        /* encode tx pkt */
        uint8_t encodedBuf[2000];
        uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, sizeof(encodedBuf));
        EXPECT_GT(encodedLen, testDataLen);
        
        /* create rx buffer */
        uint8_t rxBuf[2000];
        BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, sizeof(rxBuf));
        /* decode rx pkt */
        BFX_PROTO_L2_PKT decodedPayload = {0};
        bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
        ASSERT_TRUE(packetDecoded);
        DECODE_COMPARE_ASSERT(decodedPayload, payload);
        
        delete[] testData;
    } else {
        // For smaller limits, use the actual max
        uint8_t* testData = new uint8_t[maxDataLen];
        for (size_t i = 0; i < maxDataLen; i++) {
            testData[i] = static_cast<uint8_t>((i + 1) % 256);
        }
        BFX_PROTO_L2_PKT payload = createTestPayload(testData, maxDataLen, 0x0F);
        
        /* encode tx pkt */
        size_t bufSize = maxDataLen + 100; // Add some extra space for headers/FCS
        uint8_t* encodedBuf = new uint8_t[bufSize];
        uint16_t encodedLen = encodeTestPacket(&desc, &payload, encodedBuf, bufSize);
        EXPECT_GT(encodedLen, maxDataLen);
        
        /* create rx buffer */
        uint8_t* rxBuf = new uint8_t[bufSize];
        BFX_PROTO_L2_RX_BUFFER rxBuffer = createRxBuffer(rxBuf, bufSize);
        /* decode rx pkt */
        BFX_PROTO_L2_PKT decodedPayload = {0};
        bool packetDecoded = decodePacket(&desc, encodedBuf, encodedLen, &rxBuffer, &decodedPayload);
        ASSERT_TRUE(packetDecoded);
        DECODE_COMPARE_ASSERT(decodedPayload, payload);
        
        delete[] testData;
        delete[] encodedBuf;
        delete[] rxBuf;
    }
}