/**
 * @file fifo_split.cc
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-11-08
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include <gtest/gtest.h>
#include "bfx_qfifo.h"

/* Config macros ------------------------------------------------------------------*/

/* Mock variables and functions  --------------------------------------------------*/

/* Test suites --------------------------------------------------------------------*/
class EmFifoTestSplit : public ::testing::Test {
protected:
    static constexpr uint16_t BUFFER_SIZE = 16;
    char buffer[BUFFER_SIZE + 3];
    BFX_QFIFO fifo;

    void SetUp() override {
        BFX_QFIFO_INIT(&fifo, buffer, BUFFER_SIZE);
        buffer[BUFFER_SIZE] = '\e';
        buffer[BUFFER_SIZE + 1] = '\e';
        buffer[BUFFER_SIZE + 2] = '\e';
    }
    void fillFifo(uint16_t dataSize) {
        char* data[2];
        uint16_t acquired[2];
        for (uint16_t i = 0; i < dataSize; i++) {
            BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, 1, data, acquired);
            ASSERT_EQ(acquired[0], 1);
            ASSERT_NE(data[0], nullptr);
            BFX_QFIFO_SEND_COMMIT(&fifo);
        }
    }
    void fetchFifo(uint16_t dataSize) {
        char* data[2];
        uint16_t acquired[2];
        for (uint16_t i = 0; i < dataSize; i++) {
            BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, 1, data, acquired);
            ASSERT_EQ(acquired[0], 1);
            ASSERT_NE(data[0], nullptr);
            BFX_QFIFO_RECV_COMMIT(&fifo);
        }
    }
    void checkBufferOverflow(void) {
        ASSERT_EQ(buffer[BUFFER_SIZE], '\e');
        ASSERT_EQ(buffer[BUFFER_SIZE + 1], '\e');
        ASSERT_EQ(buffer[BUFFER_SIZE + 2], '\e');
    }
};

/* Test cases ---------------------------------------------------------------------*/

TEST_F(EmFifoTestSplit, SendAndReceiveFromStartToMiddle) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char strToRecv[sizeof(strToSend)];
    char *sendPos[2];
    char *recvPos[2];
    uint16_t sendAcquiredSize[2];
    uint16_t recvAcquiredSize[2];
    const uint16_t SEND_SIZE = sizeof(strToSend);
    const uint16_t RECV_SIZE = sizeof(strToSend);

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    ASSERT_TRUE(sendPos[0] != NULL);
    ASSERT_EQ(sendPos[1], nullptr);
    ASSERT_EQ(sendAcquiredSize[0], SEND_SIZE);
    ASSERT_EQ(sendAcquiredSize[1], 0);

    memcpy(sendPos[0], strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);

    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_TRUE(recvPos[0] != NULL);
    ASSERT_EQ(recvPos[1], nullptr);
    ASSERT_EQ(recvAcquiredSize[0], RECV_SIZE);
    ASSERT_EQ(recvAcquiredSize[1], 0);

    memcpy(strToRecv, recvPos[0], RECV_SIZE);
    ASSERT_STREQ(strToRecv, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);
}

TEST_F(EmFifoTestSplit, SendAndReceiveFromStartToBound) {
    const char strToSend[BUFFER_SIZE - 1] = {'C', 'O', 'F', 'F', 'E', 'E', '\0'};
    char strToRecv[BUFFER_SIZE - 1];
    char *sendPos[2];
    char *recvPos[2];
    uint16_t sendAcquiredSize[2];
    uint16_t recvAcquiredSize[2];
    const uint16_t SEND_SIZE = BUFFER_SIZE - 1;
    const uint16_t RECV_SIZE = BUFFER_SIZE - 1;

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos[0], strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);

    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    memcpy(strToRecv, recvPos[0], RECV_SIZE);
    ASSERT_STREQ(strToRecv, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);

    checkBufferOverflow();
}

TEST_F(EmFifoTestSplit, SendAndReceiveFromMiddleToSplitBound) {
    const char strToSend[] = {'C', 'O', 'F', 'F', 'E', 'E', '\0'};
    char strToRecv[sizeof(strToSend)];
    char *sendPos[2];
    char *recvPos[2];
    uint16_t sendAcquiredSize[2];
    uint16_t recvAcquiredSize[2];
    const uint16_t SEND_SIZE = sizeof(strToSend);
    const uint16_t RECV_SIZE = sizeof(strToSend);
    const uint16_t FIRST_PART_SIZE = 3;
    const uint16_t SECOND_PART_SIZE = SEND_SIZE - FIRST_PART_SIZE;
    const uint16_t FILL_SIZE = BUFFER_SIZE - FIRST_PART_SIZE;

    fillFifo(FILL_SIZE);
    fetchFifo(FILL_SIZE);

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos[0], strToSend, FIRST_PART_SIZE);
    memcpy(sendPos[1], &strToSend[FIRST_PART_SIZE], SECOND_PART_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);

    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    memcpy(strToRecv, recvPos[0], FIRST_PART_SIZE);
    memcpy(&strToRecv[FIRST_PART_SIZE], recvPos[1], SECOND_PART_SIZE);
    ASSERT_STREQ(strToRecv, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);

    checkBufferOverflow();
}

TEST_F(EmFifoTestSplit, ProduceOverHead) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos[2];
    uint16_t sendAcquiredSize[2];
    const uint16_t SEND_SIZE = BUFFER_SIZE - 1;

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos[0], strToSend, sizeof(strToSend));

    uint16_t freeSize = BFX_QFIFO_FREE_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    BFX_QFIFO_SEND_COMMIT(&fifo);
    freeSize = BFX_QFIFO_FREE_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    checkBufferOverflow();
}

TEST_F(EmFifoTestSplit, ConsumeOverHead) {
    char *recvPos[2];
    uint16_t recvAcquiredSize[2];
    const uint16_t RECV_SIZE = BUFFER_SIZE / 2;

    fillFifo(RECV_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);

    uint16_t freeSize = BFX_QFIFO_RECV_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    BFX_QFIFO_RECV_COMMIT(&fifo);
    freeSize = BFX_QFIFO_RECV_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);
}

TEST_F(EmFifoTestSplit, ProducerOverflow) {
    char *sendPos[2];
    uint16_t sendAcquiredSize[2];
    const uint16_t SEND_SIZE = BUFFER_SIZE * 2;

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    ASSERT_EQ(sendAcquiredSize[0], BUFFER_SIZE - 1);
    ASSERT_TRUE(sendPos[0] != NULL);
}

TEST_F(EmFifoTestSplit, ConsumerOverflow) {
    char *recvPos[2];
    uint16_t recvAcquiredSize[2];
    const uint16_t FILL_SIZE = 5;
    const uint16_t RECV_SIZE = FILL_SIZE * 2;

    fillFifo(FILL_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize[0], FILL_SIZE);
    ASSERT_TRUE(recvPos[0] != NULL);
}

TEST_F(EmFifoTestSplit, RefuseMultiProducer) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos1[2];
    char *sendPos2[2];
    uint16_t sendAcquiredSize1[2];
    uint16_t sendAcquiredSize2[2];
    const uint16_t SEND_SIZE = sizeof(strToSend);

    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos1, sendAcquiredSize1);
    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos2, sendAcquiredSize2);
    ASSERT_TRUE(sendPos1[0] != NULL);
    ASSERT_TRUE(sendPos2[0] == NULL);
    ASSERT_EQ(sendAcquiredSize1[0], SEND_SIZE);
    ASSERT_EQ(sendAcquiredSize1[1], 0);
    ASSERT_EQ(sendAcquiredSize2[0], 0);
    ASSERT_EQ(sendAcquiredSize2[1], 0);

    BFX_QFIFO_SEND_COMMIT(&fifo);
    BFX_QFIFO_SEND_ACQUIRE_SPLIT(&fifo, SEND_SIZE, sendPos2, sendAcquiredSize2);
    ASSERT_TRUE(sendPos2[0] != NULL);
    ASSERT_EQ(sendAcquiredSize2[0], SEND_SIZE);
}

TEST_F(EmFifoTestSplit, RefuseMultiConsumer) {
    char *recvPos1[2];
    char *recvPos2[2];
    uint16_t recvAcquiredSize1[2];
    uint16_t recvAcquiredSize2[2];
    const uint16_t RECV_SIZE = 5;

    fillFifo(2 * RECV_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos1, recvAcquiredSize1);
    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos2, recvAcquiredSize2);
    ASSERT_TRUE(recvPos1[0] != NULL);
    ASSERT_TRUE(recvPos2[0] == NULL);
    ASSERT_EQ(recvAcquiredSize1[0], RECV_SIZE);
    ASSERT_EQ(recvAcquiredSize2[0], 0);

    BFX_QFIFO_RECV_COMMIT(&fifo);
    BFX_QFIFO_RECV_ACQUIRE_SPLIT(&fifo, RECV_SIZE, recvPos2, recvAcquiredSize2);
    ASSERT_TRUE(recvPos2[0] != NULL);
    ASSERT_EQ(recvAcquiredSize2[0], RECV_SIZE);
}