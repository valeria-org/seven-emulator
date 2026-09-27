// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// NOT PART OF THE PUBLIC API. Helpers used by the overload sets that take `std::int32_t`, `std::uint32_t`,
// `std::int64_t` and `std::uint64_t` args (`Instruction::with1()`..`with5()` and the `CodeAssembler` instruction
// methods). It's a public header only because those inline templates use it.

#pragma once

#include <cstdint>
#include <type_traits>

namespace iced_x86::internal {

/// `true` if `T` is an integer type that isn't one of the exact overload types (`std::int32_t`, `std::uint32_t`,
/// `std::int64_t`, `std::uint64_t`) and isn't promoted to `int` (smaller types, eg. `short`, `char` are promoted
/// to `int` and already pick the `std::int32_t` overload). Passing such an arg to an overload set taking all four
/// types is ambiguous, eg. `long long` / `unsigned long long` when `std::int64_t` is `long` (Linux, 64-bit), `long` /
/// `unsigned long` when `std::int64_t` is `long long` (Windows, macOS, 32-bit Linux) and `std::size_t` on macOS.
template <typename T>
inline constexpr bool is_other_int_v = std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) >= sizeof(int) &&
									   sizeof(T) <= 8 && !std::is_same_v<T, std::int32_t> && !std::is_same_v<T, std::uint32_t> &&
									   !std::is_same_v<T, std::int64_t> && !std::is_same_v<T, std::uint64_t>;

/// `true` if at least one of the types is an `is_other_int_v` type
template <typename... Args>
inline constexpr bool any_other_int_v = (is_other_int_v<Args> || ...);

/// Enables the template overload that converts the `is_other_int_v` args and calls the non-template overload
template <typename... Args>
using EnableIfOtherIntArgs = std::enable_if_t<any_other_int_v<Args...>, int>;

/// The exact overload type with the same size and signedness as `T`
template <typename T>
using IntArgType = std::conditional_t<sizeof(T) == 8, std::conditional_t<std::is_signed_v<T>, std::int64_t, std::uint64_t>,
									  std::conditional_t<std::is_signed_v<T>, std::int32_t, std::uint32_t>>;

/// Returns `value` (it's not an `is_other_int_v` type)
template <typename T, std::enable_if_t<!is_other_int_v<T>, int> = 0>
constexpr const T& int_arg(const T& value) noexcept {
	return value;
}

/// Converts `value` to `std::int32_t`, `std::uint32_t`, `std::int64_t` or `std::uint64_t` (same size and signedness)
template <typename T, std::enable_if_t<is_other_int_v<T>, int> = 0>
constexpr IntArgType<T> int_arg(T value) noexcept {
	return static_cast<IntArgType<T>>(value);
}

} // namespace iced_x86::internal
