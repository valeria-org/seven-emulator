// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Number formatter used by the gas/intel/masm/nasm formatters (Rust: formatter/num_fmt.rs)

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "iced_x86/formatter_options.hpp"

namespace iced_x86::internal {

class NumberFormatter {
public:
	NumberFormatter() {
		constexpr std::size_t CAP = 2 +		  // "0b"
									64 +	  // 64 bin digits
									(16 - 1); // # digit separators if group size == 4 and digit sep is one char
		sb_.reserve(CAP);
	}

	std::string_view format_i8(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::int8_t value) {
		std::uint32_t flags = get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal);
		auto uvalue = static_cast<std::uint8_t>(value);
		if (value < 0) {
			flags |= Flags::ADD_MINUS_SIGN;
			uvalue = static_cast<std::uint8_t>(0U - uvalue);
		}
		return format_unsigned_integer(formatter_options, options, uvalue, 8, flags);
	}

	std::string_view format_i16(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::int16_t value) {
		std::uint32_t flags = get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal);
		auto uvalue = static_cast<std::uint16_t>(value);
		if (value < 0) {
			flags |= Flags::ADD_MINUS_SIGN;
			uvalue = static_cast<std::uint16_t>(0U - uvalue);
		}
		return format_unsigned_integer(formatter_options, options, uvalue, 16, flags);
	}

	std::string_view format_i32(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::int32_t value) {
		std::uint32_t flags = get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal);
		auto uvalue = static_cast<std::uint32_t>(value);
		if (value < 0) {
			flags |= Flags::ADD_MINUS_SIGN;
			uvalue = 0U - uvalue;
		}
		return format_unsigned_integer(formatter_options, options, uvalue, 32, flags);
	}

	std::string_view format_i64(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::int64_t value) {
		std::uint32_t flags = get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal);
		auto uvalue = static_cast<std::uint64_t>(value);
		if (value < 0) {
			flags |= Flags::ADD_MINUS_SIGN;
			uvalue = 0ULL - uvalue;
		}
		return format_unsigned_integer(formatter_options, options, uvalue, 64, flags);
	}

	std::string_view format_u8(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint8_t value) {
		return format_unsigned_integer(formatter_options, options, value, 8, get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u16(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint16_t value) {
		return format_unsigned_integer(formatter_options, options, value, 16, get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u32(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint32_t value) {
		return format_unsigned_integer(formatter_options, options, value, 32, get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u64(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint64_t value) {
		return format_unsigned_integer(formatter_options, options, value, 64, get_flags(options.leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_displ_u8(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint8_t value) {
		return format_unsigned_integer(formatter_options, options, value, 8,
									   get_flags(options.displacement_leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_displ_u16(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint16_t value) {
		return format_unsigned_integer(formatter_options, options, value, 16,
									   get_flags(options.displacement_leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_displ_u32(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint32_t value) {
		return format_unsigned_integer(formatter_options, options, value, 32,
									   get_flags(options.displacement_leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_displ_u64(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint64_t value) {
		return format_unsigned_integer(formatter_options, options, value, 64,
									   get_flags(options.displacement_leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u16_zeros(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint16_t value,
									  bool leading_zeros) {
		return format_unsigned_integer(formatter_options, options, value, 16, get_flags(leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u32_zeros(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint32_t value,
									  bool leading_zeros) {
		return format_unsigned_integer(formatter_options, options, value, 32, get_flags(leading_zeros, options.small_hex_numbers_in_decimal));
	}

	std::string_view format_u64_zeros(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint64_t value,
									  bool leading_zeros) {
		return format_unsigned_integer(formatter_options, options, value, 64, get_flags(leading_zeros, options.small_hex_numbers_in_decimal));
	}

private:
	struct Flags {
		static constexpr std::uint32_t NONE = 0;
		static constexpr std::uint32_t ADD_MINUS_SIGN = 0x0000'0001;
		static constexpr std::uint32_t LEADING_ZEROS = 0x0000'0002;
		static constexpr std::uint32_t SMALL_HEX_NUMBERS_IN_DECIMAL = 0x0000'0004;
	};

	static constexpr std::uint32_t get_flags(bool leading_zeros, bool small_hex_numbers_in_decimal) noexcept {
		std::uint32_t flags = Flags::NONE;
		if (leading_zeros)
			flags |= Flags::LEADING_ZEROS;
		if (small_hex_numbers_in_decimal)
			flags |= Flags::SMALL_HEX_NUMBERS_IN_DECIMAL;
		return flags;
	}

	std::string_view format_unsigned_integer(const FormatterOptions& formatter_options, const NumberFormattingOptions& options, std::uint64_t value,
											 std::uint32_t value_size, std::uint32_t flags);

	std::string sb_;
};

} // namespace iced_x86::internal
