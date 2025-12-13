/**
 * @file bfx_fsm.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-13
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __bfx_fsm_H__
#define __bfx_fsm_H__

/* Header import ------------------------------------------------------------------*/
#include <stdint.h>

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

#define BFX_STATUS_FATHER_NONE 0

/* Exported typedef --------------------------------------------------------------*/

typedef struct tagBFX_FSM_TRAN_RECORD {
    uint8_t event;
    uint8_t nextState;
} BFX_FSM_TRAN_RECORD;

typedef struct tagBFX_FSM_ACTION_CTX {
    uint8_t eventID;
    uint8_t entryType;
} BFX_FSM_ACTION_CTX;

typedef void (*BFX_FSM_ACTION_CALLBACK)(BFX_FSM_ACTION_CTX *ctx, void *arg, uint16_t argSize);

typedef struct tagBFX_FSM_STATE {
    uint8_t stateID;
    uint8_t defaultStateID;
    uint8_t fatherStateID;
    uint8_t tranTblCnt;
    BFX_FSM_TRAN_RECORD const *tranTbl;
    BFX_FSM_ACTION_CALLBACK actionTbl;
} BFX_FSM_STATE;

typedef struct tagBFX_FSM_HANDLE {
    BFX_FSM_STATE const *stateTbl;
    uint8_t stateCnt;
    uint8_t currentStateId;
    uint8_t maxStateId; // todo
    uint8_t maxEventId; // todo
} BFX_FSM_HANDLE;

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

uint8_t BFX_FsmProcessEvent(BFX_FSM_HANDLE *handle, uint8_t event, void *arg, uint16_t argSize);
uint8_t BFX_FsmGetCurrentStateID(BFX_FSM_HANDLE *handle);
uint8_t BFX_FsmResetTo(BFX_FSM_HANDLE *handle, uint8_t stateID);

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif