// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "iced_x86/decorator_kind.hpp"
#include "iced_x86/formatter_text_kind.hpp"
#include "iced_x86/number_kind.hpp"
#include "iced_x86/prefix_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/symbol_resolver.hpp"

namespace iced_x86 {

class Instruction;

/// Used by a `Formatter` to write all text. `StringFormatterOutput` writes to a `std::string`.
///
/// The only method that must be implemented is `write()`, all other methods call it if they're not overridden.
class FormatterOutput {
public:
	virtual ~FormatterOutput() = default;

	/// Writes text and text kind
	///
	/// # Arguments
	///
	/// - `text`: Text
	/// - `kind`: Text kind
	virtual void write(std::string_view text, FormatterTextKind kind) = 0;

	/// Writes a prefix
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `text`: Prefix text
	/// - `prefix`: Prefix
	virtual void write_prefix(const Instruction& instruction, std::string_view text, PrefixKind prefix) {
		static_cast<void>(instruction);
		static_cast<void>(prefix);
		write(text, FormatterTextKind::Prefix);
	}

	/// Writes a mnemonic (see `Instruction::mnemonic()`)
	///
	/// - `instruction`: Instruction
	/// - `text`: Mnemonic text
	virtual void write_mnemonic(const Instruction& instruction, std::string_view text) {
		static_cast<void>(instruction);
		write(text, FormatterTextKind::Mnemonic);
	}

	/// Writes a number
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `text`: Number text
	/// - `value`: Value
	/// - `number_kind`: Number kind
	/// - `kind`: Text kind
	virtual void write_number(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
							  std::string_view text, std::uint64_t value, NumberKind number_kind, FormatterTextKind kind) {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(value);
		static_cast<void>(number_kind);
		write(text, kind);
	}

	/// Writes a decorator
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `text`: Decorator text
	/// - `decorator`: Decorator
	virtual void write_decorator(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
								 std::string_view text, DecoratorKind decorator) {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(decorator);
		write(text, FormatterTextKind::Decorator);
	}

	/// Writes a register
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `text`: Register text
	/// - `register`: Register
	virtual void write_register(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
								std::string_view text, Register register_) {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(register_);
		write(text, FormatterTextKind::Register);
	}

	/// Writes a symbol
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `address`: Address
	/// - `symbol`: Symbol
	virtual void write_symbol(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
							  std::uint64_t address, const SymbolResult& symbol) {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(address);
		for (const auto& part : symbol.text)
			write(part.text.as_str(), part.color);
	}
};

/// A `FormatterOutput` that appends all text to a `std::string` (Rust: `impl FormatterOutput for String`)
class StringFormatterOutput final : public FormatterOutput {
public:
	/// Creates an instance that appends text to `output` (which must outlive this instance)
	explicit StringFormatterOutput(std::string& output) noexcept : output_(&output) {}

	/// Appends `text` to the string
	void write(std::string_view text, FormatterTextKind kind) override {
		static_cast<void>(kind);
		append(text);
	}

	// The other methods are overridden so they append the text directly (no second virtual call to `write()`),
	// same as the Rust `impl FormatterOutput for String` (the default trait methods are monomorphized).

	/// Appends `text` to the string
	void write_prefix(const Instruction& instruction, std::string_view text, PrefixKind prefix) override {
		static_cast<void>(instruction);
		static_cast<void>(prefix);
		append(text);
	}

	/// Appends `text` to the string
	void write_mnemonic(const Instruction& instruction, std::string_view text) override {
		static_cast<void>(instruction);
		append(text);
	}

	/// Appends `text` to the string
	void write_number(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
					  std::uint64_t value, NumberKind number_kind, FormatterTextKind kind) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(value);
		static_cast<void>(number_kind);
		static_cast<void>(kind);
		append(text);
	}

	/// Appends `text` to the string
	void write_decorator(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
						 DecoratorKind decorator) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(decorator);
		append(text);
	}

	/// Appends `text` to the string
	void write_register(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
						Register register_) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(register_);
		append(text);
	}

	/// Gets the string
	std::string& output() noexcept { return *output_; }

private:
	void append(std::string_view text) {
		// Most strings are 1 char long (eg. `,`, `[`, `]`, ` `) and `push_back()` is faster than `append()` (which isn't inlined)
		if (text.size() == 1)
			output_->push_back(text[0]);
		else
			output_->append(text.data(), text.size());
	}

	std::string* output_;
};

} // namespace iced_x86
