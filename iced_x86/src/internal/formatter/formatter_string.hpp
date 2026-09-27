// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iced_x86::internal {

/// Converts an ASCII char to uppercase
constexpr char formatter_string_to_upper(char c) noexcept { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; }

/// The chars of a `FormatterString` created at compile time from a lowercase string literal (see `FormatterString`):
/// `constexpr FormatterStringData<sizeof("abc")> ABC("abc");` and `FormatterString(ABC)`. It must have static storage duration.
template <std::size_t N>
struct FormatterStringData {
	static_assert(N >= 1 && N - 1 <= 0xFF, "");
	char data[1 + 2 * (N - 1) + 1];

	/// `lower` must be a lowercase ASCII string literal
	constexpr explicit FormatterStringData(const char (&lower)[N]) noexcept : data{} {
		data[0] = static_cast<char>(N - 1);
		for (std::size_t i = 0; i + 1 < N; i++) {
			data[1 + i] = lower[i];
			data[N + i] = formatter_string_to_upper(lower[i]);
		}
	}
};

/// A lowercase string and its uppercase version (Rust: `formatter::FormatterString`).
///
/// It's a pointer to constant data (it never allocates and it's trivially copyable): a length byte followed by the
/// lowercase chars followed by the uppercase chars (no NUL terminator), eg. `"\x03" "add" "ADD"`. All tables of the
/// gas/intel/masm/nasm formatters are generated constant data that contain the strings in this format.
class FormatterString {
public:
	/// The empty string
	constexpr FormatterString() noexcept : data_("") {}

	/// Creates a new instance. `data` must point to a length byte followed by the lowercase chars and the uppercase chars
	/// and it must have static storage duration
	constexpr explicit FormatterString(const char* data) noexcept : data_(data) {}

	/// Creates a new instance from compile time data
	template <std::size_t N>
	constexpr explicit FormatterString(const FormatterStringData<N>& data) noexcept : data_(data.data) {}

	/// Length of the string
	constexpr std::size_t len() const noexcept { return static_cast<std::uint8_t>(data_[0]); }

	/// `true` if it's the empty string
	constexpr bool is_default() const noexcept { return data_[0] == 0; }

	/// Gets the lowercase (`upper == false`) or the uppercase (`upper == true`) string
	constexpr std::string_view get(bool upper) const noexcept {
		const std::size_t len_ = len();
		return std::string_view(data_ + 1 + (upper ? len_ : 0), len_);
	}

	/// Gets the lowercase string
	constexpr std::string_view lower() const noexcept { return get(false); }

	/// Gets the uppercase string
	constexpr std::string_view upper() const noexcept { return get(true); }

private:
	const char* data_;
};

/// A read-only slice of `const FormatterString*` (Rust: `&'static [&'static FormatterString]`)
class FormatterStringSlice {
public:
	constexpr FormatterStringSlice() noexcept : data_(nullptr), size_(0) {}
	constexpr FormatterStringSlice(const FormatterString* const* data, std::size_t size) noexcept : data_(data), size_(size) {}
	template <std::size_t N>
	constexpr FormatterStringSlice(const std::array<const FormatterString*, N>& array) noexcept : data_(array.data()), size_(N) {}

	constexpr const FormatterString* const* begin() const noexcept { return data_; }
	constexpr const FormatterString* const* end() const noexcept { return data_ + size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_ == 0; }
	constexpr const FormatterString* operator[](std::size_t index) const noexcept { return data_[index]; }

private:
	const FormatterString* const* data_;
	std::size_t size_;
};

} // namespace iced_x86::internal
