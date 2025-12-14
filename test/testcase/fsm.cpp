/**
 * @file fsm.cpp
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include <gtest/gtest.h>
#include "bfx_fsm.h"
#include "generated/FsmTest.h"

/* Config macros ------------------------------------------------------------------*/

/* Mock variables and functions  --------------------------------------------------*/

/* Test suites --------------------------------------------------------------------*/

/* Test cases ---------------------------------------------------------------------*/

TEST(fsm, SetUp) {
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_SETUP);
}

TEST(fsm, ZeroLayerTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_BOOTLOADER);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

TEST(fsm, ZeroLayerMultiTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_ERROCCUR, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_COREDUMP);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

TEST(fsm, CrossLayerTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_FILELOADED, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_RUNMAIN_LEDON);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

TEST(fsm, SameLayerTransferAfterCrossLayerTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_FILELOADED, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_TMR200MS, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_RUNMAIN_LEDOFF);

    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_TMR200MS, NULL, 0);
    state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_RUNMAIN_LEDON);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

TEST(fsm, TurnToFatherTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_FILELOADED, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_TMR200MS, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_ERROCCUR, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_COREDUMP);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

void BFX_FsmTest_Runmain_Ledon_ActionCb(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize)
{
    EXPECT_EQ(ctx->eventID, FSMTEST_TMR200MS);
    char *str = (char *)arg;
    EXPECT_STREQ(str, "ThisIsRuntimeEventData");
}
TEST(fsm, FetchEventData) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_FILELOADED, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_TMR200MS,
        (void *)"ThisIsRuntimeEventData", sizeof("ThisIsRuntimeEventData"));
    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}

TEST(fsm, TurnToGrandFatherTransfer) {
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_SELFCHECKDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_FILELOADED, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_TMR200MS, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_BOTTONPRESSED, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_IICINITDONE, NULL, 0);
    (void)BFX_FsmProcessEvent(&g_FsmTest_fsmHandle, FSMTEST_ERROCCUR, NULL, 0);
    uint8_t state = BFX_FsmGetCurrentStateID(&g_FsmTest_fsmHandle);
    EXPECT_EQ(state, FSMTEST_COREDUMP);

    BFX_FsmResetTo(&g_FsmTest_fsmHandle, FSMTEST_SETUP);
}
