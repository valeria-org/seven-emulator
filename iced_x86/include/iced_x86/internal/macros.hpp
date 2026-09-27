// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// NOT PART OF THE PUBLIC API. Macros used by inline code in the public headers.

#pragma once

// ICED_X86_INTERNAL_FORCE_INLINE: force-inlined when optimizing. When not optimizing (-O0), every inlined copy
// gets its own stack slots (the stack frame could be several KB) so they're normal (not inlined) functions then.
// ICED_X86_INTERNAL_ALWAYS_INLINE: always force-inlined (small functions, eg. the decoder's read_u8())
#if (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#define ICED_X86_INTERNAL_FORCE_INLINE inline __attribute__((always_inline))
#define ICED_X86_INTERNAL_NOINLINE __attribute__((noinline))
#define ICED_X86_INTERNAL_COLD __attribute__((cold, noinline))
#elif defined(__GNUC__) || defined(__clang__)
#define ICED_X86_INTERNAL_FORCE_INLINE inline
#define ICED_X86_INTERNAL_NOINLINE __attribute__((noinline))
#define ICED_X86_INTERNAL_COLD __attribute__((cold, noinline))
#elif defined(_MSC_VER)
#define ICED_X86_INTERNAL_FORCE_INLINE __forceinline
#define ICED_X86_INTERNAL_NOINLINE __declspec(noinline)
#define ICED_X86_INTERNAL_COLD __declspec(noinline)
#else
#define ICED_X86_INTERNAL_FORCE_INLINE inline
#define ICED_X86_INTERNAL_NOINLINE
#define ICED_X86_INTERNAL_COLD
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ICED_X86_INTERNAL_ALWAYS_INLINE inline __attribute__((always_inline))
#define ICED_X86_INTERNAL_LIKELY(x) __builtin_expect(!!(x), 1)
#define ICED_X86_INTERNAL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
#define ICED_X86_INTERNAL_ALWAYS_INLINE __forceinline
#define ICED_X86_INTERNAL_LIKELY(x) (x)
#define ICED_X86_INTERNAL_UNLIKELY(x) (x)
#else
#define ICED_X86_INTERNAL_ALWAYS_INLINE inline
#define ICED_X86_INTERNAL_LIKELY(x) (x)
#define ICED_X86_INTERNAL_UNLIKELY(x) (x)
#endif

// ICED_X86_INTERNAL_HOT_INLINE: same as ICED_X86_INTERNAL_FORCE_INLINE unless optimizing for size (-Os/-Oz): then it's a
// normal inline function (the compiler decides). Used by big hot functions that should be inlined into the caller's loop,
// eg. Decoder::decode_out().
#if (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE_SIZE__)
#define ICED_X86_INTERNAL_HOT_INLINE inline
#else
#define ICED_X86_INTERNAL_HOT_INLINE ICED_X86_INTERNAL_FORCE_INLINE
#endif
