// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/formatter/num_fmt.hpp"

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

static constexpr std::uint64_t SMALL_POSITIVE_NUMBER = 9;

static constexpr std::string_view SMALL_DECIMAL_VALUES[SMALL_POSITIVE_NUMBER + 1] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

// clang-format off
static constexpr std::uint64_t DIVS[20] = {
	1ULL,
	10ULL,
	100ULL,
	1000ULL,
	10000ULL,
	100000ULL,
	1000000ULL,
	10000000ULL,
	100000000ULL,
	1000000000ULL,
	10000000000ULL,
	100000000000ULL,
	1000000000000ULL,
	10000000000000ULL,
	100000000000000ULL,
	1000000000000000ULL,
	10000000000000000ULL,
	100000000000000000ULL,
	1000000000000000000ULL,
	10000000000000000000ULL,
};
// clang-format on

// The length of the formatted number is calculated first, then `sb_` is resized and all chars are written to it
// (calling `push_back()`/`append()` for each char/string is much slower)

static inline std::uint32_t get_hexadecimal_digits(std::uint64_t value, std::uint32_t digits, bool leading_zero) noexcept {
	if (digits == 0) {
		digits = 1;
		std::uint64_t tmp = value;
		for (;;) {
			tmp >>= 4;
			if (tmp == 0)
				break;
			digits++;
		}
	}
	if (leading_zero && digits < 17 && ((value >> ((digits - 1) << 2)) & 0xF) > 9)
		digits++; // Another 0
	return digits;
}

static inline std::uint32_t get_decimal_digits(std::uint64_t value, std::uint32_t digits) noexcept {
	if (digits == 0) {
		digits = 1;
		std::uint64_t tmp = value;
		for (;;) {
			tmp /= 10;
			if (tmp == 0)
				break;
			digits++;
		}
	}
	return digits;
}

static inline std::uint32_t get_octal_digits(std::uint64_t value, std::uint32_t digits, std::string_view prefix) noexcept {
	if (digits == 0) {
		digits = 1;
		std::uint64_t tmp = value;
		for (;;) {
			tmp >>= 3;
			if (tmp == 0)
				break;
			digits++;
		}
	}
	// The prefix is part of the number so that a digit separator can be placed
	// between the "prefix" and the rest of the number, eg. "0" + "1234" with
	// digit separator "`" and group size = 2 is "0`12`34" and not "012`34".
	// Other prefixes, eg. "0o" prefix: 0o12`34 and never 0o`12`34.
	if (prefix == "0") {
		if (digits < 23 && ((value >> ((digits - 1) * 3)) & 7) != 0)
			digits++; // Another 0
	}
	return digits;
}

static inline std::uint32_t get_binary_digits(std::uint64_t value, std::uint32_t digits) noexcept {
	if (digits == 0) {
		digits = 1;
		std::uint64_t tmp = value;
		for (;;) {
			tmp >>= 1;
			if (tmp == 0)
				break;
			digits++;
		}
	}
	return digits;
}

// Number of chars needed to write `digits` digits (including the digit separators)
static inline std::size_t get_number_len(std::uint32_t digits, std::uint32_t digit_group_size, std::string_view digit_separator) noexcept {
	std::size_t len = digits;
	if (digit_group_size > 0 && !digit_separator.empty() && digits > 0)
		len += static_cast<std::size_t>((digits - 1) / digit_group_size) * digit_separator.size();
	return len;
}

static inline char* write_str(char* p, std::string_view s) noexcept {
	if (!s.empty()) {
		std::memcpy(p, s.data(), s.size());
		p += s.size();
	}
	return p;
}

static inline char* write_hexadecimal(char* p, std::uint64_t value, std::uint32_t digit_group_size, std::string_view digit_separator,
									  std::uint32_t digits, bool upper) noexcept {
	const std::uint32_t hex_high = upper ? static_cast<std::uint32_t>('A') - 10 : static_cast<std::uint32_t>('a') - 10;
	const bool use_digit_sep = digit_group_size > 0 && !digit_separator.empty();
	for (std::uint32_t i = 0; i < digits; i++) {
		const std::uint32_t index = digits - i - 1;
		const auto digit = index >= 16 ? 0U : static_cast<std::uint32_t>((value >> (index << 2)) & 0xF);
		if (digit > 9)
			*p++ = static_cast<char>(digit + hex_high);
		else
			*p++ = static_cast<char>(digit + '0');
		if (use_digit_sep && index > 0 && (index % digit_group_size) == 0)
			p = write_str(p, digit_separator);
	}
	return p;
}

static inline char* write_decimal(char* p, std::uint64_t value, std::uint32_t digit_group_size, std::string_view digit_separator,
								  std::uint32_t digits) noexcept {
	const bool use_digit_sep = digit_group_size > 0 && !digit_separator.empty();
	for (std::uint32_t i = 0; i < digits; i++) {
		const std::uint32_t index = digits - i - 1;
		if (index < sizeof(DIVS) / sizeof(DIVS[0])) {
			const auto digit = static_cast<std::uint32_t>(value / DIVS[index] % 10);
			*p++ = static_cast<char>(digit + '0');
		}
		else
			*p++ = '0';
		if (use_digit_sep && index > 0 && (index % digit_group_size) == 0)
			p = write_str(p, digit_separator);
	}
	return p;
}

static inline char* write_octal(char* p, std::uint64_t value, std::uint32_t digit_group_size, std::string_view digit_separator,
								std::uint32_t digits) noexcept {
	const bool use_digit_sep = digit_group_size > 0 && !digit_separator.empty();
	for (std::uint32_t i = 0; i < digits; i++) {
		const std::uint32_t index = digits - i - 1;
		const auto digit = index >= 22 ? 0U : static_cast<std::uint32_t>((value >> (index * 3)) & 7);
		*p++ = static_cast<char>(digit + '0');
		if (use_digit_sep && index > 0 && (index % digit_group_size) == 0)
			p = write_str(p, digit_separator);
	}
	return p;
}

static inline char* write_binary(char* p, std::uint64_t value, std::uint32_t digit_group_size, std::string_view digit_separator,
								 std::uint32_t digits) noexcept {
	const bool use_digit_sep = digit_group_size > 0 && !digit_separator.empty();
	for (std::uint32_t i = 0; i < digits; i++) {
		const std::uint32_t index = digits - i - 1;
		const auto digit = index >= 64 ? 0U : static_cast<std::uint32_t>((value >> index) & 1);
		*p++ = static_cast<char>(digit + '0');
		if (use_digit_sep && index > 0 && (index % digit_group_size) == 0)
			p = write_str(p, digit_separator);
	}
	return p;
}

std::string_view NumberFormatter::format_unsigned_integer(const FormatterOptions& formatter_options, const NumberFormattingOptions& options,
														  std::uint64_t value, std::uint32_t value_size, std::uint32_t flags) {
	const bool add_minus_sign = (flags & Flags::ADD_MINUS_SIGN) != 0;
	std::string_view prefix;
	std::string_view suffix;
	std::uint32_t digits;
	bool is_small_decimal = false;
	switch (options.number_base) {
	case NumberBase::Hexadecimal:
		if ((flags & Flags::SMALL_HEX_NUMBERS_IN_DECIMAL) != 0 && value <= SMALL_POSITIVE_NUMBER) {
			is_small_decimal = true;
			prefix = formatter_options.decimal_prefix();
			digits = 1;
			suffix = formatter_options.decimal_suffix();
		}
		else {
			prefix = options.prefix;
			digits = get_hexadecimal_digits(value, (flags & Flags::LEADING_ZEROS) != 0 ? (value_size + 3) >> 2 : 0,
											options.add_leading_zero_to_hex_numbers && options.prefix.empty());
			suffix = options.suffix;
		}
		break;

	case NumberBase::Decimal:
		prefix = options.prefix;
		digits = get_decimal_digits(value, 0);
		suffix = options.suffix;
		break;

	case NumberBase::Octal:
		// If the prefix is "0", it's part of the number (see get_octal_digits())
		prefix = options.prefix == "0" ? std::string_view() : options.prefix;
		digits = get_octal_digits(value, (flags & Flags::LEADING_ZEROS) != 0 ? (value_size + 2) / 3 : 0, options.prefix);
		suffix = options.suffix;
		break;

	case NumberBase::Binary:
	default:
		prefix = options.prefix;
		digits = get_binary_digits(value, (flags & Flags::LEADING_ZEROS) != 0 ? value_size : 0);
		suffix = options.suffix;
		break;
	}

	const std::size_t number_len = is_small_decimal ? 1 : get_number_len(digits, options.digit_group_size, options.digit_separator);
	const std::size_t len = (add_minus_sign ? 1 : 0) + prefix.size() + number_len + suffix.size();
	// `sb_` is only used as a buffer (its size is >= the length of the returned string) so it only needs to be resized
	// if it's too small, eg. the first time it's called.
	if (sb_.size() < len)
		sb_.resize(len);
	char* p = &sb_[0];
	if (add_minus_sign)
		*p++ = '-';
	p = write_str(p, prefix);
	if (is_small_decimal)
		*p++ = SMALL_DECIMAL_VALUES[value][0];
	else {
		switch (options.number_base) {
		case NumberBase::Hexadecimal:
			p = write_hexadecimal(p, value, options.digit_group_size, options.digit_separator, digits, options.uppercase_hex);
			break;
		case NumberBase::Decimal:
			p = write_decimal(p, value, options.digit_group_size, options.digit_separator, digits);
			break;
		case NumberBase::Octal:
			p = write_octal(p, value, options.digit_group_size, options.digit_separator, digits);
			break;
		case NumberBase::Binary:
		default:
			p = write_binary(p, value, options.digit_group_size, options.digit_separator, digits);
			break;
		}
	}
	p = write_str(p, suffix);
	ICED_DEBUG_ASSERT(p == sb_.data() + len);
	static_cast<void>(p);
	return std::string_view(sb_.data(), len);
}

} // namespace iced_x86::internal
