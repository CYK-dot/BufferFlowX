/**
 * @file dfifo.cpp
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/** Test Design --------------------------------------------------------------------
 * @Graybox-Features
 * A-free
 *  (free,free):
 *  (free,wr):  wr/finW/rd/wr/finR
 *  (free,ocp): wr/finW/rd/wr/finR /finW
 *  (free,rd):  wr/finW/rd/wr/finR /finW/rd
 * A-wr
 *  (wr,wr): invalid
 *  (wr,free):wr/finW/wr
 *  (wr,ocp): wr/finW/wr/finW/rd/finR/wr
 *  (wr,rd):  wr/finW/wr/finW/rd/finR/wr  /rd
 * A-ocp
 *  (ocp,free): wr/finW
 *  (ocp,wr):   wr/finW/wr
 *  (ocp,ocp,A):wr/finW/wr/finW
 *  (ocp,ocp,B):wr/finW/rd/finR/wr/finW/wr/finW
 *  (ocp,rd):   wr/finW/rd/finR/wr/finW/wr/finW/rd
 * A-rd
 *  (rd,rd): invalid
 *  (rd,free):wr/finW/rd
 *  (rd,wr):  wr/finW/rd /wr
 *  (rd,ocp): wr/finW/rd /wr/finW
 * 
 * @Equivalence-Class
 *  - bufferA status
 *      - free
 *      - wr
 *      - ocp
 *      - rd
 *  - bufferB status
 *      - free
 *      - wr
 *      - ocp
 *      - rd
 *
 * @TestSuites
 * - readTest
 * - writeTest
**/

/* Header import ------------------------------------------------------------------*/
#include <gtest/gtest.h>
#include "bfx_dfifo_raw.h"

/* Config macros ------------------------------------------------------------------*/

/* Mock variables and functions  --------------------------------------------------*/

/* Test suites --------------------------------------------------------------------*/
class TestDfifo : public ::testing::Test {
protected:
    void SetUp() override {
        BFX_DfifoInit(mem, 128, &cb);
    }
    void TearDown() override {
    }
    uint8_t mem[128];
    BFX_DFIFO_CB cb;
    uint8_t *Write(void) {
        return BFX_DfifoSendAcquire(mem, &cb);
    }
    void WriteFin(void) {
        BFX_DfifoSendComplete(&cb);
    }
    uint8_t *Read(void) {
        return BFX_DfifoRecvAcquire(mem, &cb);
    }
    void ReadFin(void) {
        BFX_DfifoRecvComplete(&cb);
    }
    void Reset(void) {
        BFX_DfifoClear(mem, &cb);
    }
    void ToOcpOcpA(void) {
        Reset();
        Write();
        WriteFin();
        Write();
        WriteFin();
    }
};

#define CheckSliceA(buffer) \
    ASSERT_EQ(mem, buffer) << "this buffer isnot first slice"
#define CheckSliceB(buffer) \
    ASSERT_EQ(&mem[64], buffer) << "this buffer isnot second slice"
#define CheckSliceNo(buffer) \
    ASSERT_EQ(NULL, buffer) << "this buffer isnot invalid slice"

/* Test cases ---------------------------------------------------------------------*/
TEST_F(TestDfifo, FreeFree) {
    Reset();
    uint8_t *slice = Write();
    CheckSliceA(slice);

    Reset();
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, FreeWr) {
    uint8_t *slice;
    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    ReadFin();
    slice = Write();
    CheckSliceA(slice);

    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    ReadFin();
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, FreeOcp) {
    uint8_t *slice;
    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    WriteFin();
    ReadFin();
    slice = Write();
    CheckSliceA(slice);

    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    WriteFin();
    ReadFin();
    slice = Read();
    CheckSliceB(slice);
}

TEST_F(TestDfifo, FreeRd) {
    uint8_t *slice;
    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    WriteFin();
    ReadFin();
    Read();
    slice = Write();
    CheckSliceA(slice);

    Reset();
    Write();
    WriteFin();
    Read();
    Write();
    WriteFin();
    ReadFin();
    Read();
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, WrFree) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)
    */
    Reset();
    Write();
    slice = Write();
    CheckSliceB(slice);

    Reset();
    Write();
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, WrOcp) {
    uint8_t *slice;
    /*
    (free,free) (wr,free) (ocp,free) (ocp,wr) (ocp,ocp) (rd,ocp) (free,ocp) (wr,ocp)
    */
    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Read(); // (rd,ocp)
    ReadFin(); // (free,ocp)
    Write(); // (wr,ocp)
    slice = Write();
    CheckSliceNo(slice);

    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Read(); // (rd,ocp)
    ReadFin(); // (free,ocp)
    Write(); // (wr,ocp)
    slice = Read();
    CheckSliceB(slice);
}

TEST_F(TestDfifo, WrRd) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(ocp,wr)(ocp,ocp)(rd,ocp)(free,ocp)(wr,ocp)(wr,rd)
    */
    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Read(); // (rd,ocp)
    ReadFin(); // (free,ocp)
    Write(); // (wr,ocp)
    Read(); // (wr,rd)
    slice = Write();
    CheckSliceNo(slice);

    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Read(); // (rd,ocp)
    ReadFin(); // (free,ocp)
    Write(); // (wr,ocp)
    Read(); // (wr,rd)
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, OcpFree) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)
    */
    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    slice = Write();
    CheckSliceB(slice);

    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    slice = Read();
    CheckSliceA(slice);
}

TEST_F(TestDfifo, OcpWr) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(ocp,wr)
    */
    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Write(); // (ocp,wr)
    slice = Write();
    CheckSliceNo(slice);

    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Write(); // (ocp,wr)
    slice = Read();
    CheckSliceA(slice);
}

TEST_F(TestDfifo, OcpOcpA) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(ocp,wr)(ocp,ocp,A)
    */
    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    slice = Write();
    CheckSliceA(slice);

    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    slice = Read();
    CheckSliceA(slice);
}

TEST_F(TestDfifo, OcpOcpB) {
    uint8_t *slice;
    /*
    (ocp,ocp,A)(wr,ocp)(ocp,ocp,B)
    */
    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Write(); // (wr,ocp)
    WriteFin(); // (ocp,ocp,B)
    slice = Write();
    CheckSliceB(slice);

    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    slice = Read();
    CheckSliceA(slice);
}

TEST_F(TestDfifo, OcpRd) {
    uint8_t *slice;
    /*
    (ocp,ocp,A)(wr,ocp)(ocp,ocp,B)(ocp,rd)
    */
    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Write(); // (wr,ocp)
    WriteFin(); // (ocp,ocp,B)
    Read(); // (ocp,rd)
    slice = Write();
    CheckSliceA(slice);

    Reset();
    ToOcpOcpA(); // (ocp,ocp)
    Write(); // (wr,ocp)
    WriteFin(); // (ocp,ocp,B)
    Read(); // (ocp,rd)
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, RdFree) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(rd,free)
    */
    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    slice = Write();
    CheckSliceB(slice);

    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, RdWr) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(rd,free)(rd,wr)
    */
    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    Write(); // (rd,wr)
    slice = Write();
    CheckSliceNo(slice);

    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    Write(); // (rd,wr)
    slice = Read();
    CheckSliceNo(slice);
}

TEST_F(TestDfifo, RdOcp) {
    uint8_t *slice;
    /*
    (free,free)(wr,free)(ocp,free)(rd,free)(rd,wr)(rd,ocp)
    */
    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    Write(); // (rd,wr)
    WriteFin(); // (rd,ocp)
    slice = Write();
    CheckSliceB(slice);

    Reset();
    Write(); // (wr,free)
    WriteFin(); // (ocp,free)
    Read(); // (rd,free)
    Write(); // (rd,wr)
    WriteFin(); // (rd,ocp)
    slice = Write();
    CheckSliceB(slice);
}