/**
 * @file bfx_cli.c
 * @author CYK-Dot
 * @brief basic cli matcher
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include "bfx_cli_raw.h"

/* Private typedef ----------------------------------------------------------------*/

/* Private defines ----------------------------------------------------------------*/

#define BFX_CLI_TOKEN_MATCH 0
#define BFX_CLI_TOKEN_CMD_NOT_MATCH 1

#define BFX_CLI_CMD_TOKEN_END(ucToken) ((ucToken) == '\n' || (ucToken) == ' ')
#define BFX_CLI_FMT_TOKEN_END(ucToken) ((ucToken) == '\0' || (ucToken) == ' ')
#define BFX_CLI_TOKEN_END(ucToken) ((ucToken) == '\0' || (ucToken) == ' ' || (ucToken) == '\n')
#define BFX_CLI_IS_FMT_TOKEN_PARAM(pucToken) ((pucToken)[0] == '$')

/* Global variables ---------------------------------------------------------------*/

/* Private function prototypes ---------------------------------------------------*/

/* Exported function prototypes --------------------------------------------------*/

/* Private function definitions --------------------------------------------------*/

/**
 * @brief simplified strlen function
 */
static inline uint16_t prvBFX_Strlen(const char *str)
{
    uint16_t i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

/**
 * @brief get token length
 */
static inline uint16_t prvBFX_CliGetTokenLen(const char *token)
{
    uint16_t i = 0;
    while (!BFX_CLI_TOKEN_END(token[i])) {
        i++;
    }
    return i;
}

/**
 * @brief get token count
 */
static inline uint16_t prvBFX_CliGetToken(const char *str, uint16_t *paramStore, uint16_t storeCnt)
{
    uint16_t tokenCount = 0;
    uint16_t i = 0;
    while ( i < prvBFX_Strlen(str)) {
        if (str[i] == ' ') {
            i++;
            continue;
        } else if (str[i] == '\n' || str[i] == '\0') {
            break;
        } else if (tokenCount == storeCnt) {
            return UINT16_MAX;
        }
        paramStore[tokenCount++] = i;
        i += prvBFX_CliGetTokenLen(&(str[i]));
    }
    return tokenCount;
}

/**
 * @brief match token with format
 */
static inline uint8_t prvBFX_CliTokenMatch(const char *tokenFmt, const char *tokenCmd)
{
    uint16_t i = 0;
    while (!BFX_CLI_FMT_TOKEN_END(tokenFmt[i])) {
        if (tokenFmt[i] != tokenCmd[i]) {
            return BFX_CLI_TOKEN_CMD_NOT_MATCH;
        }
        i++;
    }
    if (BFX_CLI_CMD_TOKEN_END(tokenCmd[i])) {
        return BFX_CLI_TOKEN_MATCH;
    }
    return BFX_CLI_TOKEN_CMD_NOT_MATCH;
}

/* Exported function definitions -------------------------------------------------*/

/**
 * @brief match command with format
 * 
 * @param cmd command string
 * @param fmt format string
 * @param paramStore store parameter index
 * @param storeCnt size of parameter store
 * @return parameter count when match, UINT16_MAX when not match
 * 
 * @warning
 *      1. store count should be no less than token count in format string.
 *      2. command string should be writable.
 *      3. command string should ends with \n or \n\r, \r\n is not supported.
 */
uint16_t BFX_CliRawMatch(char *cmd, const char *fmt, uint16_t *paramStore, uint16_t storeCnt)
{
    /* get token count */
    uint16_t fmtTokenCnt = prvBFX_CliGetToken(fmt, paramStore, storeCnt);
    if (fmtTokenCnt == UINT16_MAX) {
        return UINT16_MAX;
    }
    uint16_t cmdTokenCnt = prvBFX_CliGetToken(cmd, paramStore, storeCnt);
    if (cmdTokenCnt == UINT16_MAX) {
        return UINT16_MAX;
    }
    if (cmdTokenCnt != fmtTokenCnt) {
        return UINT16_MAX;
    }

    /* compare token and record parameters */
    char *tokenFmt = (char *)fmt;
    uint16_t paramCnt = 0;
    for (uint16_t i = 0; i < cmdTokenCnt; i++) {
        /* match parameters */
        if (BFX_CLI_IS_FMT_TOKEN_PARAM(tokenFmt)) {
            paramStore[paramCnt++] = paramStore[i];
            uint16_t paramSpaceIndex = prvBFX_CliGetTokenLen(&cmd[paramStore[i]]);
            cmd[paramStore[i] + paramSpaceIndex] = '\0';
            goto BFX_CLI_TOKEN_EVENT_JUDGE;
        }
        /* match keys */
        uint8_t matchStat = prvBFX_CliTokenMatch(tokenFmt, &cmd[paramStore[i]]);
        if (matchStat != BFX_CLI_TOKEN_MATCH) {
            return UINT16_MAX;
        }
        /* judge if reach end */
        BFX_CLI_TOKEN_EVENT_JUDGE:
            tokenFmt += prvBFX_CliGetTokenLen(tokenFmt);
            if (tokenFmt[0] != '\0') {
                tokenFmt++;
                continue;
            }
    }
    return paramCnt;
}
