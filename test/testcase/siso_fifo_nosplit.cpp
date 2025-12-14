/**
 * @file fifo.cc
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

class EmFifoTestNoSplit : public ::testing::Test {
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
        char* data;
        uint16_t acquired;
        for (uint16_t i = 0; i < dataSize; i++) {
            BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, 1, data, acquired);
            ASSERT_EQ(acquired, 1);
            ASSERT_NE(data, nullptr);
            BFX_QFIFO_SEND_COMMIT(&fifo);
        }
    }
    void fetchFifo(uint16_t dataSize) {
        char* data;
        uint16_t acquired;
        for (uint16_t i = 0; i < dataSize; i++) {
            BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, 1, data, acquired);
            ASSERT_EQ(acquired, 1);
            ASSERT_NE(data, nullptr);
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

TEST_F(EmFifoTestNoSplit, Initialization) {
    ASSERT_EQ(fifo.buf, buffer);
    uint16_t freeSize = BFX_QFIFO_FREE_SIZE(&fifo);
    ASSERT_EQ(freeSize, BUFFER_SIZE - 1);

    uint16_t recvSize = BFX_QFIFO_RECV_SIZE(&fifo);
    ASSERT_EQ(recvSize, 0);

    uint16_t freeNoSplitSize = BFX_QFIFO_FREE_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeNoSplitSize, BUFFER_SIZE - 1);

    uint16_t recvSizeNoSplit = BFX_QFIFO_RECV_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(recvSizeNoSplit, 0);
}

TEST_F(EmFifoTestNoSplit, SendAndReceiveFromStartToMiddle) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE = sizeof(strToSend);
    const uint16_t RECV_SIZE = sizeof(strToSend);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, RECV_SIZE);
    ASSERT_STREQ(recvPos, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);
}

TEST_F(EmFifoTestNoSplit, SendAndReceiveFromMiddleToMiddle) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE = sizeof(strToSend);
    const uint16_t RECV_SIZE = sizeof(strToSend);
    const uint16_t FILL_SIZE = 5;

    fillFifo(FILL_SIZE);
    fetchFifo(FILL_SIZE);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, RECV_SIZE);
    ASSERT_STREQ(recvPos, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);
}

TEST_F(EmFifoTestNoSplit, SendAndReceiveFromMiddleToBound) {
    const char strToSend[] = {'!', '@', '#', '$', '%', '\0'};
    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE = sizeof(strToSend);
    const uint16_t RECV_SIZE = sizeof(strToSend);
    const uint16_t FILL_SIZE = BUFFER_SIZE - SEND_SIZE - 1;

    fillFifo(FILL_SIZE);
    fetchFifo(FILL_SIZE);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, RECV_SIZE);
    ASSERT_STREQ(recvPos, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);

    checkBufferOverflow();
}

TEST_F(EmFifoTestNoSplit, SendAndReceiveFromMiddleToBoundMiddle) {
    const char strToSendFrist[] = {'H', 'E', 'L', 'L', 'O', ' ', '\0'};
    const char strToSendSecond[] = {'W', 'O', 'R', 'L', 'D', '\0'};

    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE_FIRST = sizeof(strToSendFrist);
    const uint16_t SEND_SIZE_SECOND = BUFFER_SIZE - SEND_SIZE_FIRST - 1;
    const uint16_t RECV_SIZE_FIRST = SEND_SIZE_FIRST;
    const uint16_t RECV_SIZE_SECOND = SEND_SIZE_SECOND;
    const uint16_t FILL_SIZE = BUFFER_SIZE - SEND_SIZE_FIRST;

    fillFifo(FILL_SIZE);
    fetchFifo(FILL_SIZE);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE_FIRST, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSendFrist, SEND_SIZE_FIRST);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE_SECOND, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSendSecond, sizeof(strToSendSecond));
    BFX_QFIFO_SEND_COMMIT(&fifo);

    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE_FIRST, recvPos, recvAcquiredSize);
    ASSERT_STREQ(recvPos, strToSendFrist);
    BFX_QFIFO_RECV_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE_SECOND, recvPos, recvAcquiredSize);
    ASSERT_STREQ(recvPos, strToSendSecond);
    BFX_QFIFO_RECV_COMMIT(&fifo);

    checkBufferOverflow();
}

TEST_F(EmFifoTestNoSplit, SendAndReceiveFromStartToBound) {
    const char strToSend[BUFFER_SIZE - 1] = {'B', 'E', 'E','F', '\0'};
    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE = BUFFER_SIZE - 1;
    const uint16_t RECV_SIZE = BUFFER_SIZE - 1;

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, SEND_SIZE);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, RECV_SIZE);
    ASSERT_STREQ(recvPos, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);

    checkBufferOverflow();
}

TEST_F(EmFifoTestNoSplit, ProduceTwiceReciveOnce) {
    const char strToSend[] = {'H', 'E', 'L','L', 'O', 'W', 'O', 'R', 'L', 'D', '\0'};
    char *sendPos;
    char *recvPos;
    uint16_t sendAcquiredSize;
    uint16_t recvAcquiredSize;
    const uint16_t SEND_SIZE_FIRST = sizeof(strToSend) / 2;
    const uint16_t SEND_SIZE_SECOND = sizeof(strToSend) - SEND_SIZE_FIRST;
    const uint16_t RECV_SIZE = sizeof(strToSend);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE_FIRST, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, SEND_SIZE_FIRST);
    BFX_QFIFO_SEND_COMMIT(&fifo);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE_SECOND, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend + SEND_SIZE_FIRST, SEND_SIZE_SECOND);
    BFX_QFIFO_SEND_COMMIT(&fifo);
    
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, RECV_SIZE);
    ASSERT_STREQ(recvPos, strToSend);
    BFX_QFIFO_RECV_COMMIT(&fifo);
}

TEST_F(EmFifoTestNoSplit, ProduceOverHead) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos;
    uint16_t sendAcquiredSize;
    const uint16_t SEND_SIZE = BUFFER_SIZE - 1;

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    memcpy(sendPos, strToSend, sizeof(strToSend));

    uint16_t freeSize = BFX_QFIFO_FREE_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    BFX_QFIFO_SEND_COMMIT(&fifo);
    freeSize = BFX_QFIFO_FREE_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    checkBufferOverflow();
}

TEST_F(EmFifoTestNoSplit, ConsumeOverHead) {
    char *recvPos;
    uint16_t recvAcquiredSize;
    const uint16_t RECV_SIZE = BUFFER_SIZE / 2;

    fillFifo(RECV_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);

    uint16_t freeSize = BFX_QFIFO_RECV_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);

    BFX_QFIFO_RECV_COMMIT(&fifo);
    freeSize = BFX_QFIFO_RECV_NOSPLIT_SIZE(&fifo);
    ASSERT_EQ(freeSize, 0);
}

TEST_F(EmFifoTestNoSplit, ProducerOverflow) {
    char *sendPos;
    uint16_t sendAcquiredSize;
    const uint16_t SEND_SIZE = BUFFER_SIZE * 2;

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos, sendAcquiredSize);
    ASSERT_EQ(sendAcquiredSize, BUFFER_SIZE - 1);
    ASSERT_TRUE(sendPos != NULL);
}

TEST_F(EmFifoTestNoSplit, ConsumerOverflow) {
    char *recvPos;
    uint16_t recvAcquiredSize;
    const uint16_t FILL_SIZE = 5;
    const uint16_t RECV_SIZE = FILL_SIZE * 2;

    fillFifo(FILL_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos, recvAcquiredSize);
    ASSERT_EQ(recvAcquiredSize, FILL_SIZE);
    ASSERT_TRUE(recvPos != NULL);
}

TEST_F(EmFifoTestNoSplit, RefuseMultiProducer) {
    const char strToSend[] = {'1', '2', '3', '\0'};
    char *sendPos1;
    char *sendPos2;
    uint16_t sendAcquiredSize1;
    uint16_t sendAcquiredSize2;
    const uint16_t SEND_SIZE = sizeof(strToSend);

    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos1, sendAcquiredSize1);
    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos2, sendAcquiredSize2);
    ASSERT_TRUE(sendPos1 != NULL);
    ASSERT_TRUE(sendPos2 == NULL);
    ASSERT_EQ(sendAcquiredSize1, SEND_SIZE);
    ASSERT_EQ(sendAcquiredSize2, 0);

    BFX_QFIFO_SEND_COMMIT(&fifo);
    BFX_QFIFO_SEND_ACQUIRE_NOSPLIT(&fifo, SEND_SIZE, sendPos2, sendAcquiredSize2);
    ASSERT_TRUE(sendPos2 != NULL);
    ASSERT_EQ(sendAcquiredSize2, SEND_SIZE);
}

TEST_F(EmFifoTestNoSplit, RefuseMultiConsumer) {
    char *recvPos1;
    char *recvPos2;
    uint16_t recvAcquiredSize1;
    uint16_t recvAcquiredSize2;
    const uint16_t RECV_SIZE = 5;

    fillFifo(2 * RECV_SIZE);
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos1, recvAcquiredSize1);
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos2, recvAcquiredSize2);
    ASSERT_TRUE(recvPos1 != NULL);
    ASSERT_TRUE(recvPos2 == NULL);
    ASSERT_EQ(recvAcquiredSize1, RECV_SIZE);
    ASSERT_EQ(recvAcquiredSize2, 0);

    BFX_QFIFO_RECV_COMMIT(&fifo);
    BFX_QFIFO_RECV_ACQUIRE_NOSPLIT(&fifo, RECV_SIZE, recvPos2, recvAcquiredSize2);
    ASSERT_TRUE(recvPos2 != NULL);
    ASSERT_EQ(recvAcquiredSize2, RECV_SIZE);
}