// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "iced_x86/formatter.hpp"
#include "iced_x86/formatter_options.hpp"
#include "iced_x86/formatter_output.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/op_access.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/symbol_resolver.hpp"

namespace iced_x86 {

namespace internal {
class FormatterString;
class NumberFormatter;
struct FormatterStringBuffer;
struct FormatterConstants;
struct FormatterArrayConstants;
namespace masm {
struct MemSizeInfo;
template <typename TOutput>
struct MasmFormatterImpl;
} // namespace masm
} // namespace internal

/// Masm formatter
///
/// # Examples
///
/// ```cpp
/// #include "iced_x86/decoder.hpp"
/// #include "iced_x86/masm_formatter.hpp"
///
/// const std::uint8_t bytes[] = {0x62, 0xF2, 0x4F, 0xDD, 0x72, 0x50, 0x01};
/// iced_x86::Decoder decoder(64, bytes, iced_x86::DecoderOptions::NONE);
/// const iced_x86::Instruction instr = decoder.decode();
///
/// std::string output;
/// iced_x86::MasmFormatter formatter;
/// formatter.options_mut().set_uppercase_mnemonics(true);
/// formatter.format(instr, output);
/// assert(output == "VCVTNE2PS2BF16 zmm2{k5}{z},zmm6,dword bcst [rax+4]");
/// ```
///
/// # Using a symbol resolver
///
/// ```cpp
/// #include <unordered_map>
/// #include "iced_x86/decoder.hpp"
/// #include "iced_x86/masm_formatter.hpp"
///
/// class MySymbolResolver final : public iced_x86::SymbolResolver {
/// public:
///     explicit MySymbolResolver(std::unordered_map<std::uint64_t, std::string> map) : map_(std::move(map)) {}
///     std::optional<iced_x86::SymbolResult> symbol(const iced_x86::Instruction& instruction, std::uint32_t operand,
///                                                  std::optional<std::uint32_t> instruction_operand, std::uint64_t address,
///                                                  std::uint32_t address_size) override {
///         auto it = map_.find(address);
///         if (it == map_.end())
///             return std::nullopt;
///         // The 'address' arg is the address of the symbol and doesn't have to be identical
///         // to the 'address' arg passed to symbol(). If it's different from the input
///         // address, the formatter will add +N or -N, eg. '[rax+symbol+123]'
///         return iced_x86::SymbolResult::with_str(address, it->second);
///     }
/// private:
///     std::unordered_map<std::uint64_t, std::string> map_;
/// };
///
/// const std::uint8_t bytes[] = {0x48, 0x8B, 0x8A, 0xA5, 0x5A, 0xA5, 0x5A};
/// iced_x86::Decoder decoder(64, bytes, iced_x86::DecoderOptions::NONE);
/// const iced_x86::Instruction instr = decoder.decode();
///
/// // Hard code the symbols, it's just an example!😄
/// std::unordered_map<std::uint64_t, std::string> sym_map;
/// sym_map.emplace(0x5AA55AA5, "my_data");
///
/// std::string output;
/// iced_x86::MasmFormatter formatter(std::make_unique<MySymbolResolver>(std::move(sym_map)), nullptr);
/// formatter.format(instr, output);
/// assert(output == "mov rcx,[rdx+my_data]");
/// ```
class MasmFormatter final : public Formatter {
	template <typename TOutput>
	friend struct internal::masm::MasmFormatterImpl;

public:
	ICED_X86_FORMATTER_USING_BASE_METHODS;

	/// Creates a masm formatter
	MasmFormatter();

	/// Creates a masm formatter
	///
	/// # Arguments
	///
	/// - `symbol_resolver`: Symbol resolver or `nullptr`
	/// - `options_provider`: Operand options provider or `nullptr`
	MasmFormatter(std::unique_ptr<SymbolResolver> symbol_resolver, std::unique_ptr<FormatterOptionsProvider> options_provider);

	~MasmFormatter() override;
	MasmFormatter(MasmFormatter&& other) noexcept;
	MasmFormatter& operator=(MasmFormatter&& other) noexcept;
	MasmFormatter(const MasmFormatter&) = delete;
	MasmFormatter& operator=(const MasmFormatter&) = delete;

	/// Formats the whole instruction: prefixes, mnemonic, operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	void format(const Instruction& instruction, FormatterOutput& output) override;

	/// Formats the whole instruction: prefixes, mnemonic, operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The formatted instruction is appended to this string
	void format(const Instruction& instruction, std::string& output) override;

	/// Gets the formatter options (immutable)
	const FormatterOptions& options() const noexcept override { return options_; }

	/// Gets the formatter options (mutable)
	FormatterOptions& options_mut() noexcept override { return options_; }

	/// Formats the mnemonic and/or any prefixes
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	/// - `options`: Options, see `FormatMnemonicOptions`
	void format_mnemonic_options(const Instruction& instruction, FormatterOutput& output, std::uint32_t options) override;

	/// Gets the number of operands that will be formatted. A formatter can add and remove operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	std::uint32_t operand_count(const Instruction& instruction) override;

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
	Result<std::optional<OpAccess>> op_access(const Instruction& instruction, std::uint32_t operand) override;

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
	Result<std::optional<std::uint32_t>> get_instruction_operand(const Instruction& instruction, std::uint32_t operand) override;

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
	Result<std::optional<std::uint32_t>> get_formatter_operand(const Instruction& instruction, std::uint32_t instruction_operand) override;

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
	Result<void> format_operand(const Instruction& instruction, FormatterOutput& output, std::uint32_t operand) override;

	/// Formats an operand separator
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	void format_operand_separator(const Instruction& instruction, FormatterOutput& output) override;

	/// Formats all operands
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output, eg. a `StringFormatterOutput`
	void format_all_operands(const Instruction& instruction, FormatterOutput& output) override;

	/// Formats a register
	///
	/// # Arguments
	///
	/// - `register_`: Register
	std::string_view format_register(Register register_) override;

	/// Formats a `std::int8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_i8(std::int8_t value) override;

	/// Formats a `std::int16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_i16(std::int16_t value) override;

	/// Formats a `std::int32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_i32(std::int32_t value) override;

	/// Formats a `std::int64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_i64(std::int64_t value) override;

	/// Formats a `std::uint8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_u8(std::uint8_t value) override;

	/// Formats a `std::uint16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_u16(std::uint16_t value) override;

	/// Formats a `std::uint32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_u32(std::uint32_t value) override;

	/// Formats a `std::uint64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	std::string_view format_u64(std::uint64_t value) override;

	/// Formats a `std::int8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_i8_options(std::int8_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::int16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_i16_options(std::int16_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::int32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_i32_options(std::int32_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::int64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_i64_options(std::int64_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::uint8_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_u8_options(std::uint8_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::uint16_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_u16_options(std::uint16_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::uint32_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_u32_options(std::uint32_t value, const NumberFormattingOptions& number_options) override;

	/// Formats a `std::uint64_t`
	///
	/// # Arguments
	///
	/// - `value`: Value
	/// - `number_options`: Options
	std::string_view format_u64_options(std::uint64_t value, const NumberFormattingOptions& number_options) override;

private:
	FormatterOptions options_;
	// Read-only tables (constant data)
	const internal::FormatterString* all_registers_;
	const internal::masm::MemSizeInfo* all_memory_sizes_;
	const internal::FormatterConstants* str_;
	const internal::FormatterArrayConstants* vec_;
	std::unique_ptr<internal::NumberFormatter> number_formatter_;
	// Used by `format(const Instruction&, std::string&)` (allocated the first time it's called)
	std::unique_ptr<internal::FormatterStringBuffer> string_buffer_;
	std::unique_ptr<SymbolResolver> symbol_resolver_;
	std::unique_ptr<FormatterOptionsProvider> options_provider_;
};

} // namespace iced_x86
