// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Internal helpers shared by the gas/intel/masm/nasm formatters (Rust: the private items in formatter/mod.rs)

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "iced_x86/formatter_options.hpp"
#include "iced_x86/formatter_output.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/symbol_resolver.hpp"
#include "internal/formatter/formatter_string.hpp"
#include "internal/formatter/num_fmt.hpp"

namespace iced_x86::internal {

/// Special register value used by the gas/intel/masm formatters for `st` (it's `Register::DontUse0`)
constexpr Register REGISTER_ST = static_cast<Register>(249);

/// Converts a 32/64-bit GPR to the 16-bit GPR (eg. `EAX` -> `AX`). Other registers are returned unchanged.
constexpr Register r_to_r16(Register reg) noexcept {
	if (Register::EAX <= reg && reg <= Register::R15)
		return static_cast<Register>(((static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::AX)) & 0xF) +
									 static_cast<std::uint32_t>(Register::AX));
	return reg;
}

/// Converts a 64-bit GPR to the 32-bit GPR (eg. `RAX` -> `EAX`). Other registers are returned unchanged.
constexpr Register r64_to_r32(Register reg) noexcept {
	if (Register::RAX <= reg && reg <= Register::R15)
		return static_cast<Register>(static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::RAX) +
									 static_cast<std::uint32_t>(Register::EAX));
	return reg;
}

/// Writes symbols (Rust: `FormatterOutputMethods`)
struct FormatterOutputMethods {
	static void write1(FormatterOutput& output, const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
					   const FormatterOptions& options, NumberFormatter& number_formatter, const NumberFormattingOptions& number_options,
					   std::uint64_t address, const SymbolResult& symbol, bool show_symbol_address) {
		write2(output, instruction, operand, instruction_operand, options, number_formatter, number_options, address, symbol, show_symbol_address, true,
			   false);
	}

	static void write2(FormatterOutput& output, const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
					   const FormatterOptions& options, NumberFormatter& number_formatter, const NumberFormattingOptions& number_options,
					   std::uint64_t address, const SymbolResult& symbol, bool show_symbol_address, bool write_minus_if_signed,
					   bool spaces_between_op);
};

/// Creates an owned copy of a symbol result (the text parts are copied to `vec` which must outlive the result).
/// Rust needs this to satisfy the borrow checker. In C++ it's needed if the symbol resolver can be called again before
/// the symbol result has been used (the new call could invalidate borrowed strings in the previous result).
inline std::optional<SymbolResult> to_owned(const std::optional<SymbolResult>& sym_res, std::vector<SymResTextPart>& vec) {
	if (!sym_res)
		return std::nullopt;
	return sym_res->to_owned(vec);
}

/// Gets the number of mnemonic variants of a condition code (`cc_index` = 0-15, eg. 3 if it's 2: `b`, `c`, `nae`)
constexpr std::size_t get_cc_mnemonics_count(std::uint32_t cc_index) noexcept {
	constexpr std::uint8_t COUNTS[16] = {1, 1, 3, 3, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2};
	return COUNTS[cc_index & 0xF];
}

/// Gets the index of the mnemonic of a `Jcc`/`SETcc`/`CMOVcc`/... instruction. `cc_index` is the condition code (0-15) and
/// `mnemonics_size` is the number of mnemonic variants (eg. 3: `jb`, `jc`, `jnae`), see the `CC_*` options.
std::size_t get_mnemonic_cc_index(const FormatterOptions& options, std::uint32_t cc_index, std::size_t mnemonics_size) noexcept;

/// Gets the mnemonic of a `Jcc`/`SETcc`/`CMOVcc`/... instruction. `cc_index` is the condition code (0-15) and `mnemonics`
/// contains all mnemonic variants (eg. `jb`, `jc`, `jnae`), see the `CC_*` options.
template <std::size_t N>
inline const FormatterString& get_mnemonic_cc(const FormatterOptions& options, std::uint32_t cc_index,
											  const std::array<FormatterString, N>& mnemonics) noexcept {
	return mnemonics[get_mnemonic_cc_index(options, cc_index, N)];
}

} // namespace iced_x86::internal
