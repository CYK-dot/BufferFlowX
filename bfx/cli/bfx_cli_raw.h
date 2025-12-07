/**
 * @file bfx_cli.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __BFX_CLI_RAW_H__
#define __BFX_CLI_RAW_H__

/* Header import ------------------------------------------------------------------*/
#include <stdint.h>
#include "bfx_dfifo_raw.h"

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

/* Exported typedef --------------------------------------------------------------*/

typedef void (*MINCLI_callback)(char **args, uint8_t argc);

typedef struct tagMINCLI_DESC {
    char *expr;
    MINCLI_callback cb;
} MINCLI_DESC;

typedef struct tagMINCLI_CBUF {
    char *mem;
    uint16_t memSize;
    uint16_t cbIndex;
    BFX_DFIFO_CB dFifo;
    uint16_t writeIndex;
    uint16_t resv;
} MINCLI_CBUF;

typedef struct tagBFX_CLI_INIT_STRUCT {
    uint8_t *store;
    uint16_t storeMaxSize;
    uint16_t paramMaxCnt;
    uint16_t paramMaxLen;
} BFX_CLI_INIT_STRUCT;

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

uint16_t BFX_CliInit(MINCLI_CBUF *handle, const BFX_CLI_INIT_STRUCT *conf);
uint8_t BFX_CliSend(MINCLI_CBUF *handle, const char *str, uint16_t len);
uint8_t BFX_CliProcess(MINCLI_CBUF *handle, MINCLI_DESC *cmdList, uint8_t listCnt);

uint16_t BFX_CliMatch(const char *cmd, const char *fmt, uint16_t *paramIndex, uint16_t paramMaxCnt);

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif