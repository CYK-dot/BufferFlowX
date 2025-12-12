/**
 * @file bfx_cli.h
 * @author CYK-Dot
 * @brief basic cli matcher
 * @version 0.1
 * @date 2025-12-07
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __BFX_CLI_RAW_H__
#define __BFX_CLI_RAW_H__

/* Header import ------------------------------------------------------------------*/
#include <stdint.h>
#include <limits.h>

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

/* Exported typedef --------------------------------------------------------------*/

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

uint16_t BFX_CliRawMatch(char *cmd, const char *fmt, uint16_t *paramIndex, uint16_t paramMaxCnt);

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif