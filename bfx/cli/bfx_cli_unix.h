/**
 * @file bfx_cli_unix.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-28
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __bfx_cli_unix_H__
#define __bfx_cli_unix_H__

/* Header import ------------------------------------------------------------------*/
#include <stdint.h>

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

/* Exported typedef --------------------------------------------------------------*/

typedef enum tagBFX_CLI_UNIX_EVENT {
    BFX_CLI_UNIX_EVENT_NONE = 0,
    BFX_CLI_UNIX_EVENT_PROMPT = 1,
    BFX_CLI_UNIX_EVENT_ECHO = 2,
} BFX_CLI_UNIX_EVENT;


typedef void (*BFX_CLI_UNIX_PROMPT_CB)(char *promptBuf, uint16_t promptMaxSize, uint16_t cmdIdx);
typedef void (*BFX_CLI_UNIX_CMD_CB)(char *echoBuf, uint16_t echoMaxSize, char *cmd);

typedef struct tagBFX_CLI_UNIX_CFG {
    char **cmdList;
    char *promptBuf;
    char *echoBuf;
    BFX_CLI_UNIX_PROMPT_CB promptCb;
    uint16_t cmdCnt;
    uint16_t promptMaxSize;
    uint16_t echoMaxSize;
} BFX_CLI_UNIX_CFG;

typedef struct tagBFX_CLI_UNIX_HANDLE {
    BFX_CLI_UNIX_CFG *cfg;
    uint16_t echoHeadIdx;
    uint16_t echoInsertIdx;
} BFX_CLI_UNIX_HANDLE;

typedef void* BFX_CLI_UNIX;

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

BFX_CLI_UNIX BFX_CliUnixCreateStatic(const BFX_CLI_UNIX_CFG *cfg, BFX_CLI_UNIX_HANDLE *handle);
BFX_CLI_UNIX_EVENT BFX_CliUnixFeed(BFX_CLI_UNIX cli, char input);

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif