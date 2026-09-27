// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "iced_x86/format_mnemonic_options.hpp"
#include "iced_x86/formatter_options.hpp"
#include "iced_x86/formatter_output.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/op_access.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/symbol_resolver.hpp"

namespace iced_x86 {

class Instruction;

/// Formats instructions. This is the base class of the gas, Intel, masm and nasm formatters
/// (`GasFormatter`, `IntelFormatter`, `MasmFormatter`, `NasmFormatter`).
///
/// All methods that take a `std::string&` output append the text to the string.
///
/// The returned `std::string_view` of the `format_register()`/`format_i8()`/... methods is only valid until
/// the next call to any of these methods.
class Formatter {
public:
	virtual ~Formatter() = default;

	/// Formats the whole instruction: prefixes, mnemonic, operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	virtual void format(const Instruction& instruction, FormatterOutput& output) = 0;

	/// Formats the whole instruction: prefixes, mnemonic, operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The formatted instruction is appended to this string
	virtual void format(const Instruction& instruction, std::string& output) {
		StringFormatterOutput string_output(output);
		format(instruction, string_output);
	}

	/// Gets the formatter options (immutable)
	virtual const FormatterOptions& options() const noexcept = 0;

	/// Gets the formatter options (mutable)
	virtual FormatterOptions& options_mut() noexcept = 0;

	/// Formats the mnemonic and any prefixes
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	void format_mnemonic(const Instruction& instruction, FormatterOutput& output) {
		format_mnemonic_options(instruction, output, FormatMnemonicOptions::NONE);
	}

	/// Formats the mnemonic and any prefixes
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The text is appended to this string
	void format_mnemonic(const Instruction& instruction, std::string& output) {
		StringFormatterOutput string_output(output);
		format_mnemonic_options(instruction, string_output, FormatMnemonicOptions::NONE);
	}

	/// Formats the mnemonic and/or any prefixes
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	/// - `options`: Options, see `FormatMnemonicOptions`
	virtual void format_mnemonic_options(const Instruction& instruction, FormatterOutput& output, std::uint32_t options) = 0;

	/// Formats the mnemonic and/or any prefixes
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The text is appended to this string
	/// - `options`: Options, see `FormatMnemonicOptions`
	void format_mnemonic_options(const Instruction& instruction, std::string& output, std::uint32_t options) {
		StringFormatterOutput string_output(output);
		format_mnemonic_options(instruction, string_output, options);
	}

	/// Gets the number of operands that will be formatted. A formatter can add and remove operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	virtual std::uint32_t operand_count(const Instruction& instruction) = 0;

	/// Returns the operand access but only if it's an operand added by the formatter. If it's an
	/// operand that is part of `Instruction`, you should call eg. `InstructionInfoFactory::info()`.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand. See `operand_count()`
	///
	/// # Errors
	///
	/// This fails if `operand` is invalid.
	virtual Result<std::optional<OpAccess>> op_access(const Instruction& instruction, std::uint32_t operand) = 0;

	/// Converts a formatter operand index to an instruction operand index. Returns `std::nullopt` if it's an operand added by the formatter
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand. See `operand_count()`
	///
	/// # Errors
	///
	/// This fails if `operand` is invalid.
	virtual Result<std::optional<std::uint32_t>> get_instruction_operand(const Instruction& instruction, std::uint32_t operand) = 0;

	/// Converts an instruction operand index to a formatter operand index. Returns `std::nullopt` if the instruction operand isn't used by the formatter
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `instruction_operand`: Instruction operand
	///
	/// # Errors
	///
	/// This fails if `instruction_operand` is invalid.
	virtual Result<std::optional<std::uint32_t>> get_formatter_operand(const Instruction& instruction, std::uint32_t instruction_operand) = 0;

	/// Formats an operand. This is a formatter operand and not necessarily a real instruction operand.
	/// A formatter can add and remove operands.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand. See `operand_count()`
	///
	/// # Errors
	///
	/// This fails if `operand` is invalid.
	virtual Result<void> format_operand(const Instruction& instruction, FormatterOutput& output, std::uint32_t operand) = 0;

	/// Formats an operand. This is a formatter operand and not necessarily a real instruction operand.
	/// A formatter can add and remove operands.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The text is appended to this string
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand. See `operand_count()`
	///
	/// # Errors
	///
	/// This fails if `operand` is invalid.
	Result<void> format_operand(const Instruction& instruction, std::string& output, std::uint32_t operand) {
		StringFormatterOutput string_output(output);
		return format_operand(instruction, string_output, operand);
	}

	/// Formats an operand separator
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	virtual void format_operand_separator(const Instruction& instruction, FormatterOutput& output) = 0;

	/// Formats an operand separator
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The text is appended to this string
	void format_operand_separator(const Instruction& instruction, std::string& output) {
		StringFormatterOutput string_output(output);
		format_operand_separator(instruction, string_output);
	}

	/// Formats all operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	virtual void format_all_operands(const Instruction& instruction, FormatterOutput& output) = 0;

	/// Formats all operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The text is appended to this string
	void format_all_operands(const Instruction& instruction, std::string& output) {
		StringFormatterOutput string_output(output);
		format_all_operands(instruction, string_output);
	}

	/// Formats a register
	///
	/// # Arguments
	///
	/// - `register`: Register
	virtual std::string_view format_register(Register register_) = 0;

	/// Formats a `std::int8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_i8(std::int8_t value) = 0;

	/// Formats a `std::int16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_i16(std::int16_t value) = 0;

	/// Formats a `std::int32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_i32(std::int32_t value) = 0;

	/// Formats a `std::int64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_i64(std::int64_t value) = 0;

	/// Formats a `std::uint8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_u8(std::uint8_t value) = 0;

	/// Formats a `std::uint16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_u16(std::uint16_t value) = 0;

	/// Formats a `std::uint32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_u32(std::uint32_t value) = 0;

	/// Formats a `std::uint64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	virtual std::string_view format_u64(std::uint64_t value) = 0;

	/// Formats a `std::int8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_i8_options(std::int8_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::int16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_i16_options(std::int16_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::int32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_i32_options(std::int32_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::int64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_i64_options(std::int64_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::uint8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_u8_options(std::uint8_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::uint16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_u16_options(std::uint16_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::uint32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_u32_options(std::uint32_t value, const NumberFormattingOptions& number_options) = 0;

	/// Formats a `std::uint64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	virtual std::string_view format_u64_options(std::uint64_t value, const NumberFormattingOptions& number_options) = 0;

protected:
	Formatter() = default;
	Formatter(const Formatter&) = default;
	Formatter(Formatter&&) = default;
	Formatter& operator=(const Formatter&) = default;
	Formatter& operator=(Formatter&&) = default;
};

} // namespace iced_x86

/// Add this to a class derived from `iced_x86::Formatter` so the `std::string&` overloads of the base class
/// aren't hidden by the overridden `FormatterOutput&` methods.
#define ICED_X86_FORMATTER_USING_BASE_METHODS \
	using ::iced_x86::Formatter::format; \
	using ::iced_x86::Formatter::format_mnemonic; \
	using ::iced_x86::Formatter::format_mnemonic_options; \
	using ::iced_x86::Formatter::format_operand; \
	using ::iced_x86::Formatter::format_operand_separator; \
	using ::iced_x86::Formatter::format_all_operands
