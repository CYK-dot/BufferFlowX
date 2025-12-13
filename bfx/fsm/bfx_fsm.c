/**
 * @file bfx_fsm.c
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-13
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include "bfx_fsm.h"
#include <stddef.h>

/* Private typedef ----------------------------------------------------------------*/

/* Private defines ----------------------------------------------------------------*/

#define BFX_FsmGetNextState(pxStateTbl, uiNextState, uiRealNextState) do {    \
    if (pxStateTbl[uiNextState - 1].defaultStateID == uiNextState) {          \
        uiRealNextState = uiNextState;                                        \
        break;                                                                \
    }                                                                         \
    uint8_t indexNextState = uiNextState;                                     \
    while (pxStateTbl[indexNextState - 1].defaultStateID != indexNextState) { \
        indexNextState = pxStateTbl[indexNextState - 1].defaultStateID;       \
    }                                                                         \
    uiRealNextState = indexNextState;                                         \
} while (0)

#define BFX_FsmCallActionEvent(pxStateHandle, ucEvent, pxArg, uiArgSize) do { \
    BFX_FSM_ACTION_CTX ctx = {                                                \
        .eventID = ucEvent,                                                   \
    };                                                                        \
    pxStateHandle.actionTbl(&ctx, pxArg, uiArgSize);                          \
} while (0)

/* Global variables ---------------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------*/

/* Exported function prototypes --------------------------------------------------*/

/* Private function definitions --------------------------------------------------*/

/* Exported function definitions -------------------------------------------------*/

uint8_t BFX_FsmProcessEvent(BFX_FSM_HANDLE *handle, uint8_t event, void *arg, uint16_t argSize)
{
    // 在当前节点试图处理该事件
    BFX_FSM_TRAN_RECORD const *tranTbl = handle->stateTbl[handle->currentStateId - 1].tranTbl;
    uint8_t tranTblCnt = handle->stateTbl[handle->currentStateId - 1].tranTblCnt;

    for (uint8_t i = 0; i < tranTblCnt; i++) {
        if (tranTbl[i].event == event) {
            BFX_FsmGetNextState(handle->stateTbl, tranTbl[i].nextState, handle->currentStateId);
            BFX_FsmCallActionEvent(handle->stateTbl[handle->currentStateId - 1], event, arg, argSize);
            return 0;
        }
    }
    // 上升到父节点
    uint8_t fatherState = handle->stateTbl[handle->currentStateId - 1].fatherStateID;
    while (fatherState != 0) {
        tranTbl = handle->stateTbl[fatherState - 1].tranTbl;
        tranTblCnt = handle->stateTbl[fatherState - 1].tranTblCnt;
        for (uint8_t i = 0; i < tranTblCnt; i++) {
            if (tranTbl[i].event == event) {
                BFX_FsmGetNextState(handle->stateTbl, tranTbl[i].nextState, handle->currentStateId);
                BFX_FsmCallActionEvent(handle->stateTbl[handle->currentStateId - 1], event, arg, argSize);
                return 0;
            }
        }
        fatherState = handle->stateTbl[fatherState - 1].fatherStateID;
    }
    return 1;
}

uint8_t BFX_FsmGetCurrentStateID(BFX_FSM_HANDLE *handle)
{
    return handle->currentStateId;
}

uint8_t BFX_FsmResetTo(BFX_FSM_HANDLE *handle, uint8_t stateID)
{
    handle->currentStateId = stateID;
    return 0;
}