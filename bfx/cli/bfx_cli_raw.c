/**
 * @file bfx_cli.c
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include "bfx_cli_raw.h"
#include <string.h>

/* Private typedef ----------------------------------------------------------------*/

/* Private defines ----------------------------------------------------------------*/

#define BFX_CLI_STAT_KEY   0
#define BFX_CLI_STAT_PARAM 1

#define BFX_CLI_TOKEN_MATCH 0
#define BFX_CLI_TOKEN_CMD_REACH_END 1
#define BFX_CLI_TOKEN_CMD_NOT_MATCH 2
#define BFX_CLI_TOKEN_FMT_REACH_END 3
#define BFX_CLI_TOKEN_MATCH_TO_END 4

#define BFX_CLI_IS_TOKEN_END(ch) \
    ((ch) == '\0' || (ch) == ' ' || (ch) == '$' || (ch) == '\n')

/* Global variables ---------------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------*/

/* Exported function prototypes --------------------------------------------------*/

/* Private function definitions --------------------------------------------------*/

/* Exported function definitions -------------------------------------------------*/

/**
 * @brief match command token with format token
 * 
 * @param cmdToken command token
 * @param fmtToken format token
 * @return uint16_t token length
 */
static inline uint8_t BFX_CliMatchKeyToken(const char *cmdToken, const char *fmtToken,
    uint16_t *tokenLen)
{
    uint16_t i = 0;
    while (!BFX_CLI_IS_TOKEN_END(fmtToken[i])) {
        if (BFX_CLI_IS_TOKEN_END(cmdToken[i])) {
            return BFX_CLI_TOKEN_CMD_REACH_END;
        }
        if (cmdToken[i] != fmtToken[i]) {
            return BFX_CLI_TOKEN_CMD_NOT_MATCH;
        }
        i++;
    }
    if (BFX_CLI_IS_TOKEN_END(cmdToken[i])) {
        *tokenLen = i;
        return BFX_CLI_TOKEN_MATCH;
    }
    return BFX_CLI_TOKEN_FMT_REACH_END;
}

static inline uint8_t BFX_CliMatchKeyParam(const char *cmd, const char *fmt,
    uint16_t *cmdTokenIndex, uint16_t *fmtTokenIndex)
{
    /* forward to next token */
    while (!BFX_CLI_IS_TOKEN_END(cmd[*cmdTokenIndex])) {
        (*cmdTokenIndex)++;
    }
    while (!BFX_CLI_IS_TOKEN_END(fmt[*fmtTokenIndex])) {
        (*fmtTokenIndex)++;
    }
    /* judge event */
    if (cmd[*cmdTokenIndex] == '\0' && fmt[*fmtTokenIndex] != '\n') {
        return BFX_CLI_TOKEN_FMT_REACH_END;
    } else if (cmd[*cmdTokenIndex] != '\0' && fmt[*fmtTokenIndex] == '\n') {
        return BFX_CLI_TOKEN_CMD_REACH_END;
    } else if (cmd[*cmdTokenIndex] == '\n' && fmt[*fmtTokenIndex] == '\0') {
        return BFX_CLI_TOKEN_MATCH_TO_END;
    } else {
        (*cmdTokenIndex)++;
        (*fmtTokenIndex)++;
        return BFX_CLI_TOKEN_MATCH;
    }
}

/**
 * @brief match command with expression
 * 
 * @param cmd command string
 * @param expr expression string
 * @param paramIndex parameter index array
 * @param paramMaxCnt maximum parameter count
 * @return uint16_t parameter count
 * 
 * @note 
 * valid format should be:
 *  1. use '$' to mark parameter position
 *  2. '$' can be left close to keys, but should not be right close to keys
 *  3. parameter should not close to each other
 *  4. no more than 2 spaces
 *  5. '$' have no escape character, so should not use '$' to make keys
 * valid examples
 *  1. set led$index on
 *  2. display adc$peri -$channel on
 * invalid examples
 *  1. set  led on
 *  2. set$indexA$indexB on
 */
uint16_t BFX_CliMatch(const char *cmd, const char *fmt, uint16_t *paramIndex, uint16_t paramMaxCnt)
{
    uint8_t fmtStat = BFX_CLI_STAT_KEY;
    uint16_t fmtTokenIndex = 0;
    uint16_t cmdTokenIndex = 0;
    uint16_t tokenLen = 0;
    uint16_t tokenCnt = 0;
    uint8_t tokenEvent;

    while(cmdTokenIndex < strlen(cmd)) {
        if (fmtStat == BFX_CLI_STAT_KEY) {
            /* token status change */
            if (fmt[fmtTokenIndex] == '$') {
                fmtTokenIndex++;
                fmtStat = BFX_CLI_STAT_PARAM;
                continue;
            }
            /* token matcher */
            tokenEvent = BFX_CliMatchKeyToken(&cmd[cmdTokenIndex], &fmt[fmtTokenIndex], &tokenLen);
            if (tokenEvent != BFX_CLI_TOKEN_MATCH) {
                return UINT16_MAX;
            } else {
                cmdTokenIndex += tokenLen + 1;
                fmtTokenIndex += tokenLen + 1;
            }
        } else {
            /* token matcher */
            paramIndex[tokenCnt++] = cmdTokenIndex;
            if (tokenCnt > paramMaxCnt) {
                return UINT16_MAX;
            }
            tokenEvent = BFX_CliMatchKeyParam(cmd, fmt, &cmdTokenIndex, &fmtTokenIndex);
            if (tokenEvent == BFX_CLI_TOKEN_MATCH_TO_END) {
                break;
            } else if (tokenEvent != BFX_CLI_TOKEN_MATCH) {
                return UINT16_MAX;
            }
            /* token status change */
            fmtStat = BFX_CLI_STAT_KEY;
        }
    }
    return tokenCnt;
}