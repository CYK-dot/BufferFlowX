/**
 * @file bfx_def.h
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-25
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */
#ifndef __bfx_def_H__
#define __bfx_def_H__

/* Header import ------------------------------------------------------------------*/

/* Config macros -----------------------------------------------------------------*/

/* Export macros -----------------------------------------------------------------*/

#if defined(__GNUC__) || defined(__clang__)
#define BFX_HOT_PATH __attribute__((hot))
#define BFX_INLINE __attribute__((always_inline))
#define BFX_STATIC_INLINE __attribute__((always_inline)) static inline
#define BFX_LIKELY(x)   __builtin_expect(!!(x), 1)
#define BFX_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define BFX_DMB() __asm__ volatile("dmb" : : : "memory")
#define BFX_ALIGN_TYPE(x) __attribute__((aligned(x)))
#define BFX_WEAK_TYPE __attribute__((weak))
#else
#define BFX_HOT_PATH
#define BFX_INLINE
#define BFX_LIKELY(x) (x)
#define BFX_UNLIKELY(x) (x)
#define BFX_DMB()
#define BFX_ALIGN_TYPE(x)
#define BFX_WEAK_TYPE
#endif

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