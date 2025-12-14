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

/* Global variables ---------------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------*/

static inline void BFX_FsmGetTransitionInfo(BFX_FSM_HANDLE *handle, uint8_t stateId, 
                                           BFX_FSM_TRAN_RECORD const **tranTbl, uint8_t *tranTblCnt);
static inline uint8_t BFX_FsmProcessTransition(BFX_FSM_HANDLE *handle, BFX_FSM_TRAN_RECORD const *tranTbl, 
                                              uint8_t tranTblCnt, uint8_t event, void *arg, uint16_t argSize);

/* Exported function prototypes --------------------------------------------------*/

/* Private function definitions --------------------------------------------------*/

static inline uint8_t BFX_FsmGetNextState(BFX_FSM_STATE const * stateTbl, uint8_t nextState)
{
    if (stateTbl[nextState - 1].defaultStateID == nextState) {
        return nextState;
    }
    uint8_t indexNextState = nextState;
    while (stateTbl[indexNextState - 1].defaultStateID != indexNextState) {
        indexNextState = stateTbl[indexNextState - 1].defaultStateID;
    }
    return indexNextState;
}

static inline void BFX_FsmCallActionEvent(BFX_FSM_STATE const * stateHandle, uint8_t event, void *arg, uint16_t argSize)
{
    BFX_FSM_ACTION_CTX ctx = {
        .eventID = event,
    };
    stateHandle->actionTbl(&ctx, arg, argSize);
}

static inline void BFX_FsmGetTransitionInfo(BFX_FSM_HANDLE *handle, uint8_t stateId, 
                                           BFX_FSM_TRAN_RECORD const **tranTbl, uint8_t *tranTblCnt)
{
    *tranTbl = handle->stateTbl[stateId - 1].tranTbl;
    *tranTblCnt = handle->stateTbl[stateId - 1].tranTblCnt;
}

static inline uint8_t BFX_FsmProcessTransition(BFX_FSM_HANDLE *handle, BFX_FSM_TRAN_RECORD const *tranTbl, 
                                              uint8_t tranTblCnt, uint8_t event, void *arg, uint16_t argSize)
{
    if (tranTbl == NULL || tranTblCnt == 0) {
        return 1;
    }
    for (uint8_t i = 0; i < tranTblCnt; i++) {
        if (tranTbl[i].event == event) {
            handle->currentStateId = BFX_FsmGetNextState(handle->stateTbl, tranTbl[i].nextState);
            BFX_FsmCallActionEvent(&(handle->stateTbl[handle->currentStateId - 1]), event, arg, argSize);
            return 0;
        }
    }
    return 1;
}

/* Exported function definitions -------------------------------------------------*/

/**
 * @brief Process an event in the finite state machine.
 * 
 * @param handle Pointer to the FSM handle.
 * @param event The event to process.
 * @param arg Pointer to the argument associated with the event.
 * @param argSize Size of the argument in bytes.
 * @return uint8_t 0 if the event is processed successfully, 1 otherwise.
 */
uint8_t BFX_FsmProcessEvent(BFX_FSM_HANDLE *handle, uint8_t event, void *arg, uint16_t argSize)
{
    /* process event in current state */
    BFX_FSM_TRAN_RECORD const *tranTbl;
    uint8_t tranTblCnt;
    BFX_FsmGetTransitionInfo(handle, handle->currentStateId, &tranTbl, &tranTblCnt);
    if (BFX_FsmProcessTransition(handle, tranTbl, tranTblCnt, event, arg, argSize) == 0) {
        return 0;
    }

    /* process event in father states */
    uint8_t fatherState = handle->stateTbl[handle->currentStateId - 1].fatherStateID;
    while (fatherState != 0) {
        BFX_FsmGetTransitionInfo(handle, fatherState, &tranTbl, &tranTblCnt);
        if (BFX_FsmProcessTransition(handle, tranTbl, tranTblCnt, event, arg, argSize) == 0) {
            return 0;
        }
        fatherState = handle->stateTbl[fatherState - 1].fatherStateID;
    }
    return 1;
}

/**
 * @brief Get the current state ID of the finite state machine.
 * 
 * @param handle Pointer to the FSM handle.
 * @return uint8_t The current state ID.
 */
uint8_t BFX_FsmGetCurrentStateID(BFX_FSM_HANDLE *handle)
{
    return handle->currentStateId;
}

/**
 * @brief Reset the finite state machine to a specified state.
 * 
 * @param handle Pointer to the FSM handle.
 * @param stateID The state ID to reset to.
 * @return uint8_t 0 if the reset is successful, 1 otherwise.
 */
uint8_t BFX_FsmResetTo(BFX_FSM_HANDLE *handle, uint8_t stateID)
{
    handle->currentStateId = stateID;
    return 0;
}