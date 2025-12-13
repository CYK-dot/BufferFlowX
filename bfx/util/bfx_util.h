/**
 * @file bfx_util.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-13
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __bfx_util_H__
#define __bfx_util_H__

/* Header import ------------------------------------------------------------------*/
#include <stddef.h>

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

#define contain_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Exported typedef --------------------------------------------------------------*/

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported function -------------------------------------------------------------*/

/* C++ ---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif