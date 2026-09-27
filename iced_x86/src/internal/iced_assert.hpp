// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdlib>

namespace iced_x86::internal {
[[noreturn]] inline void iced_assert_failed() noexcept { std::abort(); }
} // namespace iced_x86::internal

// Same as Rust's `iced_assert!()`: always checked (also in release builds), aborts if false.
#define ICED_ASSERT(cond) \
	do { \
		if (!(cond)) \
			::iced_x86::internal::iced_assert_failed(); \
	} while (0)

// Same as Rust's `debug_assert!()`: only checked in debug builds (`NDEBUG` not defined)
#ifdef NDEBUG
#define ICED_DEBUG_ASSERT(cond) \
	do { \
	} while (0)
#else
#define ICED_DEBUG_ASSERT(cond) ICED_ASSERT(cond)
#endif

// Same as Rust's `unreachable!()`
#define ICED_UNREACHABLE() ::iced_x86::internal::iced_assert_failed()

#if defined(__GNUC__) || defined(__clang__)
#define ICED_LIKELY(x) __builtin_expect(!!(x), 1)
#define ICED_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define ICED_FORCE_INLINE inline __attribute__((always_inline))
#define ICED_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define ICED_LIKELY(x) (x)
#define ICED_UNLIKELY(x) (x)
#define ICED_FORCE_INLINE __forceinline
#define ICED_NOINLINE __declspec(noinline)
#else
#define ICED_LIKELY(x) (x)
#define ICED_UNLIKELY(x) (x)
#define ICED_FORCE_INLINE inline
#define ICED_NOINLINE
#endif
