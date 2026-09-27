// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "iced_x86/cc_a.hpp"
#include "iced_x86/cc_ae.hpp"
#include "iced_x86/cc_b.hpp"
#include "iced_x86/cc_be.hpp"
#include "iced_x86/cc_e.hpp"
#include "iced_x86/cc_g.hpp"
#include "iced_x86/cc_ge.hpp"
#include "iced_x86/cc_l.hpp"
#include "iced_x86/cc_le.hpp"
#include "iced_x86/cc_ne.hpp"
#include "iced_x86/cc_np.hpp"
#include "iced_x86/cc_p.hpp"
#include "iced_x86/memory_size_options.hpp"
#include "iced_x86/number_base.hpp"

namespace iced_x86 {

class Instruction;

/// Formatter options
class FormatterOptions {
public:
	/// Creates default formatter options
	FormatterOptions() noexcept
		: hex_digit_group_size_(4)
		, decimal_digit_group_size_(3)
		, octal_digit_group_size_(4)
		, binary_digit_group_size_(4)
		, options1_(Flags1::UPPERCASE_HEX | Flags1::SMALL_HEX_NUMBERS_IN_DECIMAL | Flags1::ADD_LEADING_ZERO_TO_HEX_NUMBERS | Flags1::BRANCH_LEADING_ZEROS |
					Flags1::SIGNED_MEMORY_DISPLACEMENTS | Flags1::SHOW_BRANCH_SIZE | Flags1::USE_PSEUDO_OPS | Flags1::MASM_ADD_DS_PREFIX32 |
					Flags1::MASM_SYMBOL_DISPL_IN_BRACKETS | Flags1::MASM_DISPL_IN_BRACKETS)
		, options2_(0)
		, first_operand_char_index_(0)
		, tab_size_(0)
		, number_base_(NumberBase::Hexadecimal)
		, memory_size_options_(MemorySizeOptions::Default)
		, cc_b_(CC_b::b)
		, cc_ae_(CC_ae::ae)
		, cc_e_(CC_e::e)
		, cc_ne_(CC_ne::ne)
		, cc_be_(CC_be::be)
		, cc_a_(CC_a::a)
		, cc_p_(CC_p::p)
		, cc_np_(CC_np::np)
		, cc_l_(CC_l::l)
		, cc_ge_(CC_ge::ge)
		, cc_le_(CC_le::le)
		, cc_g_(CC_g::g) {}

	/// Creates default gas (AT&T) formatter options
	static FormatterOptions with_gas() {
		FormatterOptions options;
		options.set_hex_prefix("0x");
		options.set_octal_prefix("0");
		options.set_binary_prefix("0b");
		return options;
	}

	/// Creates default Intel (XED) formatter options
	static FormatterOptions with_intel() {
		FormatterOptions options;
		options.set_hex_suffix("h");
		options.set_octal_suffix("o");
		options.set_binary_suffix("b");
		return options;
	}

	/// Creates default masm formatter options
	static FormatterOptions with_masm() {
		FormatterOptions options;
		options.set_hex_suffix("h");
		options.set_octal_suffix("o");
		options.set_binary_suffix("b");
		return options;
	}

	/// Creates default nasm formatter options
	static FormatterOptions with_nasm() {
		FormatterOptions options;
		options.set_hex_suffix("h");
		options.set_octal_suffix("o");
		options.set_binary_suffix("b");
		return options;
	}

	/// Prefixes are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `REP stosd`
	/// 👍 | `false` | `rep stosd`
	bool uppercase_prefixes() const noexcept { return (options1_ & Flags1::UPPERCASE_PREFIXES) != 0; }

	/// Prefixes are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `REP stosd`
	/// 👍 | `false` | `rep stosd`
	void set_uppercase_prefixes(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_PREFIXES;
		} else {
			options1_ &= ~Flags1::UPPERCASE_PREFIXES;
		}
	}

	/// Mnemonics are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `MOV rcx,rax`
	/// 👍 | `false` | `mov rcx,rax`
	bool uppercase_mnemonics() const noexcept { return (options1_ & Flags1::UPPERCASE_MNEMONICS) != 0; }

	/// Mnemonics are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `MOV rcx,rax`
	/// 👍 | `false` | `mov rcx,rax`
	void set_uppercase_mnemonics(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_MNEMONICS;
		} else {
			options1_ &= ~Flags1::UPPERCASE_MNEMONICS;
		}
	}

	/// Registers are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov RCX,[RAX+RDX*8]`
	/// 👍 | `false` | `mov rcx,[rax+rdx*8]`
	bool uppercase_registers() const noexcept { return (options1_ & Flags1::UPPERCASE_REGISTERS) != 0; }

	/// Registers are uppercased
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov RCX,[RAX+RDX*8]`
	/// 👍 | `false` | `mov rcx,[rax+rdx*8]`
	void set_uppercase_registers(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_REGISTERS;
		} else {
			options1_ &= ~Flags1::UPPERCASE_REGISTERS;
		}
	}

	/// Keywords are uppercased (eg. `BYTE PTR`, `SHORT`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov BYTE PTR [rcx],12h`
	/// 👍 | `false` | `mov byte ptr [rcx],12h`
	bool uppercase_keywords() const noexcept { return (options1_ & Flags1::UPPERCASE_KEYWORDS) != 0; }

	/// Keywords are uppercased (eg. `BYTE PTR`, `SHORT`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov BYTE PTR [rcx],12h`
	/// 👍 | `false` | `mov byte ptr [rcx],12h`
	void set_uppercase_keywords(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_KEYWORDS;
		} else {
			options1_ &= ~Flags1::UPPERCASE_KEYWORDS;
		}
	}

	/// Uppercase decorators, eg. `{z}`, `{sae}`, `{rd-sae}` (but not opmask registers: `{k1}`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `vunpcklps xmm2{k5}{Z},xmm6,dword bcst [rax+4]`
	/// 👍 | `false` | `vunpcklps xmm2{k5}{z},xmm6,dword bcst [rax+4]`
	bool uppercase_decorators() const noexcept { return (options1_ & Flags1::UPPERCASE_DECORATORS) != 0; }

	/// Uppercase decorators, eg. `{z}`, `{sae}`, `{rd-sae}` (but not opmask registers: `{k1}`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `vunpcklps xmm2{k5}{Z},xmm6,dword bcst [rax+4]`
	/// 👍 | `false` | `vunpcklps xmm2{k5}{z},xmm6,dword bcst [rax+4]`
	void set_uppercase_decorators(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_DECORATORS;
		} else {
			options1_ &= ~Flags1::UPPERCASE_DECORATORS;
		}
	}

	/// Everything is uppercased, except numbers and their prefixes/suffixes
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `MOV EAX,GS:[RCX*4+0ffh]`
	/// 👍 | `false` | `mov eax,gs:[rcx*4+0ffh]`
	bool uppercase_all() const noexcept { return (options1_ & Flags1::UPPERCASE_ALL) != 0; }

	/// Everything is uppercased, except numbers and their prefixes/suffixes
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `MOV EAX,GS:[RCX*4+0ffh]`
	/// 👍 | `false` | `mov eax,gs:[rcx*4+0ffh]`
	void set_uppercase_all(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_ALL;
		} else {
			options1_ &= ~Flags1::UPPERCASE_ALL;
		}
	}

	/// Character index (0-based) where the first operand is formatted. Can be set to 0 to format it immediately after the mnemonic.
	/// At least one space or tab is always added between the mnemonic and the first operand.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `0` | `mov•rcx,rbp`
	/// _ | `8` | `mov•••••rcx,rbp`
	std::uint32_t first_operand_char_index() const noexcept { return first_operand_char_index_; }

	/// Character index (0-based) where the first operand is formatted. Can be set to 0 to format it immediately after the mnemonic.
	/// At least one space or tab is always added between the mnemonic and the first operand.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `0` | `mov•rcx,rbp`
	/// _ | `8` | `mov•••••rcx,rbp`
	void set_first_operand_char_index(std::uint32_t value) noexcept { first_operand_char_index_ = value; }

	/// Size of a tab character or 0 to use spaces
	///
	/// - Default: `0`
	std::uint32_t tab_size() const noexcept { return tab_size_; }

	/// Size of a tab character or 0 to use spaces
	///
	/// - Default: `0`
	void set_tab_size(std::uint32_t value) noexcept { tab_size_ = value; }

	/// Add a space after the operand separator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov rax, rcx`
	/// 👍 | `false` | `mov rax,rcx`
	bool space_after_operand_separator() const noexcept { return (options1_ & Flags1::SPACE_AFTER_OPERAND_SEPARATOR) != 0; }

	/// Add a space after the operand separator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov rax, rcx`
	/// 👍 | `false` | `mov rax,rcx`
	void set_space_after_operand_separator(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SPACE_AFTER_OPERAND_SEPARATOR;
		} else {
			options1_ &= ~Flags1::SPACE_AFTER_OPERAND_SEPARATOR;
		}
	}

	/// Add a space between the memory expression and the brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[ rcx+rdx ]`
	/// 👍 | `false` | `mov eax,[rcx+rdx]`
	bool space_after_memory_bracket() const noexcept { return (options1_ & Flags1::SPACE_AFTER_MEMORY_BRACKET) != 0; }

	/// Add a space between the memory expression and the brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[ rcx+rdx ]`
	/// 👍 | `false` | `mov eax,[rcx+rdx]`
	void set_space_after_memory_bracket(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SPACE_AFTER_MEMORY_BRACKET;
		} else {
			options1_ &= ~Flags1::SPACE_AFTER_MEMORY_BRACKET;
		}
	}

	/// Add spaces between memory operand `+` and `-` operators
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx + rdx*8 - 80h]`
	/// 👍 | `false` | `mov eax,[rcx+rdx*8-80h]`
	bool space_between_memory_add_operators() const noexcept { return (options1_ & Flags1::SPACE_BETWEEN_MEMORY_ADD_OPERATORS) != 0; }

	/// Add spaces between memory operand `+` and `-` operators
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx + rdx*8 - 80h]`
	/// 👍 | `false` | `mov eax,[rcx+rdx*8-80h]`
	void set_space_between_memory_add_operators(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SPACE_BETWEEN_MEMORY_ADD_OPERATORS;
		} else {
			options1_ &= ~Flags1::SPACE_BETWEEN_MEMORY_ADD_OPERATORS;
		}
	}

	/// Add spaces between memory operand `*` operator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx+rdx * 8-80h]`
	/// 👍 | `false` | `mov eax,[rcx+rdx*8-80h]`
	bool space_between_memory_mul_operators() const noexcept { return (options1_ & Flags1::SPACE_BETWEEN_MEMORY_MUL_OPERATORS) != 0; }

	/// Add spaces between memory operand `*` operator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx+rdx * 8-80h]`
	/// 👍 | `false` | `mov eax,[rcx+rdx*8-80h]`
	void set_space_between_memory_mul_operators(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SPACE_BETWEEN_MEMORY_MUL_OPERATORS;
		} else {
			options1_ &= ~Flags1::SPACE_BETWEEN_MEMORY_MUL_OPERATORS;
		}
	}

	/// Show memory operand scale value before the index register
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[8*rdx]`
	/// 👍 | `false` | `mov eax,[rdx*8]`
	bool scale_before_index() const noexcept { return (options1_ & Flags1::SCALE_BEFORE_INDEX) != 0; }

	/// Show memory operand scale value before the index register
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[8*rdx]`
	/// 👍 | `false` | `mov eax,[rdx*8]`
	void set_scale_before_index(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SCALE_BEFORE_INDEX;
		} else {
			options1_ &= ~Flags1::SCALE_BEFORE_INDEX;
		}
	}

	/// Always show the scale value even if it's `*1`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rbx+rcx*1]`
	/// 👍 | `false` | `mov eax,[rbx+rcx]`
	bool always_show_scale() const noexcept { return (options1_ & Flags1::ALWAYS_SHOW_SCALE) != 0; }

	/// Always show the scale value even if it's `*1`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rbx+rcx*1]`
	/// 👍 | `false` | `mov eax,[rbx+rcx]`
	void set_always_show_scale(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::ALWAYS_SHOW_SCALE;
		} else {
			options1_ &= ~Flags1::ALWAYS_SHOW_SCALE;
		}
	}

	/// Always show the effective segment register. If the option is `false`, only show the segment register if
	/// there's a segment override prefix.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ds:[ecx]`
	/// 👍 | `false` | `mov eax,[ecx]`
	bool always_show_segment_register() const noexcept { return (options1_ & Flags1::ALWAYS_SHOW_SEGMENT_REGISTER) != 0; }

	/// Always show the effective segment register. If the option is `false`, only show the segment register if
	/// there's a segment override prefix.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ds:[ecx]`
	/// 👍 | `false` | `mov eax,[ecx]`
	void set_always_show_segment_register(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::ALWAYS_SHOW_SEGMENT_REGISTER;
		} else {
			options1_ &= ~Flags1::ALWAYS_SHOW_SEGMENT_REGISTER;
		}
	}

	/// Show zero displacements
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx*2+0]`
	/// 👍 | `false` | `mov eax,[rcx*2]`
	bool show_zero_displacements() const noexcept { return (options1_ & Flags1::SHOW_ZERO_DISPLACEMENTS) != 0; }

	/// Show zero displacements
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rcx*2+0]`
	/// 👍 | `false` | `mov eax,[rcx*2]`
	void set_show_zero_displacements(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SHOW_ZERO_DISPLACEMENTS;
		} else {
			options1_ &= ~Flags1::SHOW_ZERO_DISPLACEMENTS;
		}
	}

	/// Hex number prefix or an empty string, eg. `"0x"`
	///
	/// - Default: `""` (masm/nasm/intel), `"0x"` (gas)
	std::string_view hex_prefix() const noexcept { return hex_prefix_; }

	/// Hex number prefix or an empty string, eg. `"0x"`
	///
	/// - Default: `""` (masm/nasm/intel), `"0x"` (gas)
	void set_hex_prefix(std::string_view value) { hex_prefix_ = value; }

	/// Hex number prefix or an empty string, eg. `"0x"`
	///
	/// - Default: `""` (masm/nasm/intel), `"0x"` (gas)
	void set_hex_prefix_string(std::string value) { hex_prefix_ = std::move(value); }

	/// Hex number suffix or an empty string, eg. `"h"`
	///
	/// - Default: `"h"` (masm/nasm/intel), `""` (gas)
	std::string_view hex_suffix() const noexcept { return hex_suffix_; }

	/// Hex number suffix or an empty string, eg. `"h"`
	///
	/// - Default: `"h"` (masm/nasm/intel), `""` (gas)
	void set_hex_suffix(std::string_view value) { hex_suffix_ = value; }

	/// Hex number suffix or an empty string, eg. `"h"`
	///
	/// - Default: `"h"` (masm/nasm/intel), `""` (gas)
	void set_hex_suffix_string(std::string value) { hex_suffix_ = std::move(value); }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `0x12345678`
	/// 👍 | `4` | `0x1234_5678`
	std::uint32_t hex_digit_group_size() const noexcept { return hex_digit_group_size_; }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `0x12345678`
	/// 👍 | `4` | `0x1234_5678`
	void set_hex_digit_group_size(std::uint32_t value) noexcept { hex_digit_group_size_ = value; }

	/// Decimal number prefix or an empty string
	///
	/// - Default: `""`
	std::string_view decimal_prefix() const noexcept { return decimal_prefix_; }

	/// Decimal number prefix or an empty string
	///
	/// - Default: `""`
	void set_decimal_prefix(std::string_view value) { decimal_prefix_ = value; }

	/// Decimal number prefix or an empty string
	///
	/// - Default: `""`
	void set_decimal_prefix_string(std::string value) { decimal_prefix_ = std::move(value); }

	/// Decimal number suffix or an empty string
	///
	/// - Default: `""`
	std::string_view decimal_suffix() const noexcept { return decimal_suffix_; }

	/// Decimal number suffix or an empty string
	///
	/// - Default: `""`
	void set_decimal_suffix(std::string_view value) { decimal_suffix_ = value; }

	/// Decimal number suffix or an empty string
	///
	/// - Default: `""`
	void set_decimal_suffix_string(std::string value) { decimal_suffix_ = std::move(value); }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `12345678`
	/// 👍 | `3` | `12_345_678`
	std::uint32_t decimal_digit_group_size() const noexcept { return decimal_digit_group_size_; }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `12345678`
	/// 👍 | `3` | `12_345_678`
	void set_decimal_digit_group_size(std::uint32_t value) noexcept { decimal_digit_group_size_ = value; }

	/// Octal number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0"` (gas)
	std::string_view octal_prefix() const noexcept { return octal_prefix_; }

	/// Octal number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0"` (gas)
	void set_octal_prefix(std::string_view value) { octal_prefix_ = value; }

	/// Octal number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0"` (gas)
	void set_octal_prefix_string(std::string value) { octal_prefix_ = std::move(value); }

	/// Octal number suffix or an empty string
	///
	/// - Default: `"o"` (masm/nasm/intel), `""` (gas)
	std::string_view octal_suffix() const noexcept { return octal_suffix_; }

	/// Octal number suffix or an empty string
	///
	/// - Default: `"o"` (masm/nasm/intel), `""` (gas)
	void set_octal_suffix(std::string_view value) { octal_suffix_ = value; }

	/// Octal number suffix or an empty string
	///
	/// - Default: `"o"` (masm/nasm/intel), `""` (gas)
	void set_octal_suffix_string(std::string value) { octal_suffix_ = std::move(value); }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `12345670`
	/// 👍 | `4` | `1234_5670`
	std::uint32_t octal_digit_group_size() const noexcept { return octal_digit_group_size_; }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `12345670`
	/// 👍 | `4` | `1234_5670`
	void set_octal_digit_group_size(std::uint32_t value) noexcept { octal_digit_group_size_ = value; }

	/// Binary number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0b"` (gas)
	std::string_view binary_prefix() const noexcept { return binary_prefix_; }

	/// Binary number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0b"` (gas)
	void set_binary_prefix(std::string_view value) { binary_prefix_ = value; }

	/// Binary number prefix or an empty string
	///
	/// - Default: `""` (masm/nasm/intel), `"0b"` (gas)
	void set_binary_prefix_string(std::string value) { binary_prefix_ = std::move(value); }

	/// Binary number suffix or an empty string
	///
	/// - Default: `"b"` (masm/nasm/intel), `""` (gas)
	std::string_view binary_suffix() const noexcept { return binary_suffix_; }

	/// Binary number suffix or an empty string
	///
	/// - Default: `"b"` (masm/nasm/intel), `""` (gas)
	void set_binary_suffix(std::string_view value) { binary_suffix_ = value; }

	/// Binary number suffix or an empty string
	///
	/// - Default: `"b"` (masm/nasm/intel), `""` (gas)
	void set_binary_suffix_string(std::string value) { binary_suffix_ = std::move(value); }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `11010111`
	/// 👍 | `4` | `1101_0111`
	std::uint32_t binary_digit_group_size() const noexcept { return binary_digit_group_size_; }

	/// Size of a digit group, see also `digit_separator()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `0` | `11010111`
	/// 👍 | `4` | `1101_0111`
	void set_binary_digit_group_size(std::uint32_t value) noexcept { binary_digit_group_size_ = value; }

	/// Digit separator or an empty string. See also eg. `hex_digit_group_size()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `""` | `0x12345678`
	/// _ | `"_"` | `0x1234_5678`
	std::string_view digit_separator() const noexcept { return digit_separator_; }

	/// Digit separator or an empty string. See also eg. `hex_digit_group_size()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `""` | `0x12345678`
	/// _ | `"_"` | `0x1234_5678`
	void set_digit_separator(std::string_view value) { digit_separator_ = value; }

	/// Digit separator or an empty string. See also eg. `hex_digit_group_size()`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `""` | `0x12345678`
	/// _ | `"_"` | `0x1234_5678`
	void set_digit_separator_string(std::string value) { digit_separator_ = std::move(value); }

	/// Add leading zeros to hexadecimal/octal/binary numbers.
	/// This option has no effect on branch targets and displacements, use `branch_leading_zeros`
	/// and `displacement_leading_zeros`.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `0x0000000A`/`0000000Ah`
	/// 👍 | `false` | `0xA`/`0Ah`
	bool leading_zeros() const noexcept { return (options1_ & Flags1::LEADING_ZEROS) != 0; }

	/// Add leading zeros to hexadecimal/octal/binary numbers.
	/// This option has no effect on branch targets and displacements, use `branch_leading_zeros`
	/// and `displacement_leading_zeros`.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `0x0000000A`/`0000000Ah`
	/// 👍 | `false` | `0xA`/`0Ah`
	void set_leading_zeros(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::LEADING_ZEROS;
		} else {
			options1_ &= ~Flags1::LEADING_ZEROS;
		}
	}

	/// Same as `leading_zeros()`
	bool leading_zeroes() const noexcept { return leading_zeros(); }

	/// Same as `set_leading_zeros()`
	void set_leading_zeroes(bool value) noexcept { set_leading_zeros(value); }

	/// Use uppercase hex digits
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0xFF`
	/// _ | `false` | `0xff`
	bool uppercase_hex() const noexcept { return (options1_ & Flags1::UPPERCASE_HEX) != 0; }

	/// Use uppercase hex digits
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0xFF`
	/// _ | `false` | `0xff`
	void set_uppercase_hex(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::UPPERCASE_HEX;
		} else {
			options1_ &= ~Flags1::UPPERCASE_HEX;
		}
	}

	/// Small hex numbers (-9 .. 9) are shown in decimal
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `9`
	/// _ | `false` | `0x9`
	bool small_hex_numbers_in_decimal() const noexcept { return (options1_ & Flags1::SMALL_HEX_NUMBERS_IN_DECIMAL) != 0; }

	/// Small hex numbers (-9 .. 9) are shown in decimal
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `9`
	/// _ | `false` | `0x9`
	void set_small_hex_numbers_in_decimal(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SMALL_HEX_NUMBERS_IN_DECIMAL;
		} else {
			options1_ &= ~Flags1::SMALL_HEX_NUMBERS_IN_DECIMAL;
		}
	}

	/// Add a leading zero to hex numbers if there's no prefix and the number starts with hex digits `A-F`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0FFh`
	/// _ | `false` | `FFh`
	bool add_leading_zero_to_hex_numbers() const noexcept { return (options1_ & Flags1::ADD_LEADING_ZERO_TO_HEX_NUMBERS) != 0; }

	/// Add a leading zero to hex numbers if there's no prefix and the number starts with hex digits `A-F`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0FFh`
	/// _ | `false` | `FFh`
	void set_add_leading_zero_to_hex_numbers(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::ADD_LEADING_ZERO_TO_HEX_NUMBERS;
		} else {
			options1_ &= ~Flags1::ADD_LEADING_ZERO_TO_HEX_NUMBERS;
		}
	}

	/// Number base
	///
	/// - Default: `Hexadecimal`
	NumberBase number_base() const noexcept { return number_base_; }

	/// Number base
	///
	/// - Default: `Hexadecimal`
	void set_number_base(NumberBase value) noexcept { number_base_ = value; }

	/// Add leading zeros to branch offsets. Used by `CALL NEAR`, `CALL FAR`, `JMP NEAR`, `JMP FAR`, `Jcc`, `LOOP`, `LOOPcc`, `XBEGIN`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `je 00000123h`
	/// _ | `false` | `je 123h`
	bool branch_leading_zeros() const noexcept { return (options1_ & Flags1::BRANCH_LEADING_ZEROS) != 0; }

	/// Add leading zeros to branch offsets. Used by `CALL NEAR`, `CALL FAR`, `JMP NEAR`, `JMP FAR`, `Jcc`, `LOOP`, `LOOPcc`, `XBEGIN`
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `je 00000123h`
	/// _ | `false` | `je 123h`
	void set_branch_leading_zeros(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::BRANCH_LEADING_ZEROS;
		} else {
			options1_ &= ~Flags1::BRANCH_LEADING_ZEROS;
		}
	}

	/// Same as `branch_leading_zeros()`
	bool branch_leading_zeroes() const noexcept { return branch_leading_zeros(); }

	/// Same as `set_branch_leading_zeros()`
	void set_branch_leading_zeroes(bool value) noexcept { set_branch_leading_zeros(value); }

	/// Show immediate operands as signed numbers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,-1`
	/// 👍 | `false` | `mov eax,FFFFFFFF`
	bool signed_immediate_operands() const noexcept { return (options1_ & Flags1::SIGNED_IMMEDIATE_OPERANDS) != 0; }

	/// Show immediate operands as signed numbers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,-1`
	/// 👍 | `false` | `mov eax,FFFFFFFF`
	void set_signed_immediate_operands(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SIGNED_IMMEDIATE_OPERANDS;
		} else {
			options1_ &= ~Flags1::SIGNED_IMMEDIATE_OPERANDS;
		}
	}

	/// Displacements are signed numbers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `mov al,[eax-2000h]`
	/// _ | `false` | `mov al,[eax+0FFFFE000h]`
	bool signed_memory_displacements() const noexcept { return (options1_ & Flags1::SIGNED_MEMORY_DISPLACEMENTS) != 0; }

	/// Displacements are signed numbers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `mov al,[eax-2000h]`
	/// _ | `false` | `mov al,[eax+0FFFFE000h]`
	void set_signed_memory_displacements(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SIGNED_MEMORY_DISPLACEMENTS;
		} else {
			options1_ &= ~Flags1::SIGNED_MEMORY_DISPLACEMENTS;
		}
	}

	/// Add leading zeros to displacements
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov al,[eax+00000012h]`
	/// 👍 | `false` | `mov al,[eax+12h]`
	bool displacement_leading_zeros() const noexcept { return (options1_ & Flags1::DISPLACEMENT_LEADING_ZEROS) != 0; }

	/// Add leading zeros to displacements
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov al,[eax+00000012h]`
	/// 👍 | `false` | `mov al,[eax+12h]`
	void set_displacement_leading_zeros(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::DISPLACEMENT_LEADING_ZEROS;
		} else {
			options1_ &= ~Flags1::DISPLACEMENT_LEADING_ZEROS;
		}
	}

	/// Same as `displacement_leading_zeros()`
	bool displacement_leading_zeroes() const noexcept { return displacement_leading_zeros(); }

	/// Same as `set_displacement_leading_zeros()`
	void set_displacement_leading_zeroes(bool value) noexcept { set_displacement_leading_zeros(value); }

	/// Options that control if the memory size (eg. `DWORD PTR`) is shown or not.
	/// This is ignored by the gas (AT&T) formatter.
	///
	/// - Default: `Default`
	MemorySizeOptions memory_size_options() const noexcept { return memory_size_options_; }

	/// Options that control if the memory size (eg. `DWORD PTR`) is shown or not.
	/// This is ignored by the gas (AT&T) formatter.
	///
	/// - Default: `Default`
	void set_memory_size_options(MemorySizeOptions value) noexcept { memory_size_options_ = value; }

	/// Show `RIP+displ` or the virtual address
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rip+12345678h]`
	/// 👍 | `false` | `mov eax,[1029384756AFBECDh]`
	bool rip_relative_addresses() const noexcept { return (options1_ & Flags1::RIP_RELATIVE_ADDRESSES) != 0; }

	/// Show `RIP+displ` or the virtual address
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rip+12345678h]`
	/// 👍 | `false` | `mov eax,[1029384756AFBECDh]`
	void set_rip_relative_addresses(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::RIP_RELATIVE_ADDRESSES;
		} else {
			options1_ &= ~Flags1::RIP_RELATIVE_ADDRESSES;
		}
	}

	/// Show `NEAR`, `SHORT`, etc if it's a branch instruction
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `je short 1234h`
	/// _ | `false` | `je 1234h`
	bool show_branch_size() const noexcept { return (options1_ & Flags1::SHOW_BRANCH_SIZE) != 0; }

	/// Show `NEAR`, `SHORT`, etc if it's a branch instruction
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `je short 1234h`
	/// _ | `false` | `je 1234h`
	void set_show_branch_size(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SHOW_BRANCH_SIZE;
		} else {
			options1_ &= ~Flags1::SHOW_BRANCH_SIZE;
		}
	}

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// _ | `false` | `vcmpsd xmm2,xmm6,xmm3,5`
	bool use_pseudo_ops() const noexcept { return (options1_ & Flags1::USE_PSEUDO_OPS) != 0; }

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// _ | `false` | `vcmpsd xmm2,xmm6,xmm3,5`
	void set_use_pseudo_ops(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::USE_PSEUDO_OPS;
		} else {
			options1_ &= ~Flags1::USE_PSEUDO_OPS;
		}
	}

	/// Show the original value after the symbol name
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[myfield (12345678)]`
	/// 👍 | `false` | `mov eax,[myfield]`
	bool show_symbol_address() const noexcept { return (options1_ & Flags1::SHOW_SYMBOL_ADDRESS) != 0; }

	/// Show the original value after the symbol name
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[myfield (12345678)]`
	/// 👍 | `false` | `mov eax,[myfield]`
	void set_show_symbol_address(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::SHOW_SYMBOL_ADDRESS;
		} else {
			options1_ &= ~Flags1::SHOW_SYMBOL_ADDRESS;
		}
	}

	/// (gas only): If `true`, the formatter doesn't add `%` to registers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ecx`
	/// 👍 | `false` | `mov %eax,%ecx`
	bool gas_naked_registers() const noexcept { return (options1_ & Flags1::GAS_NAKED_REGISTERS) != 0; }

	/// (gas only): If `true`, the formatter doesn't add `%` to registers
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ecx`
	/// 👍 | `false` | `mov %eax,%ecx`
	void set_gas_naked_registers(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::GAS_NAKED_REGISTERS;
		} else {
			options1_ &= ~Flags1::GAS_NAKED_REGISTERS;
		}
	}

	/// (gas only): Shows the mnemonic size suffix even when not needed
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `movl %eax,%ecx`
	/// 👍 | `false` | `mov %eax,%ecx`
	bool gas_show_mnemonic_size_suffix() const noexcept { return (options1_ & Flags1::GAS_SHOW_MNEMONIC_SIZE_SUFFIX) != 0; }

	/// (gas only): Shows the mnemonic size suffix even when not needed
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `movl %eax,%ecx`
	/// 👍 | `false` | `mov %eax,%ecx`
	void set_gas_show_mnemonic_size_suffix(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::GAS_SHOW_MNEMONIC_SIZE_SUFFIX;
		} else {
			options1_ &= ~Flags1::GAS_SHOW_MNEMONIC_SIZE_SUFFIX;
		}
	}

	/// (gas only): Add a space after the comma if it's a memory operand
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `(%eax, %ecx, 2)`
	/// 👍 | `false` | `(%eax,%ecx,2)`
	bool gas_space_after_memory_operand_comma() const noexcept {
		return (options1_ & Flags1::GAS_SPACE_AFTER_MEMORY_OPERAND_COMMA) != 0;
	}

	/// (gas only): Add a space after the comma if it's a memory operand
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `(%eax, %ecx, 2)`
	/// 👍 | `false` | `(%eax,%ecx,2)`
	void set_gas_space_after_memory_operand_comma(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::GAS_SPACE_AFTER_MEMORY_OPERAND_COMMA;
		} else {
			options1_ &= ~Flags1::GAS_SPACE_AFTER_MEMORY_OPERAND_COMMA;
		}
	}

	/// (masm only): Add a `DS` segment override even if it's not present. Used if it's 16/32-bit code and mem op is a displ
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `mov eax,ds:[12345678]`
	/// _ | `false` | `mov eax,[12345678]`
	bool masm_add_ds_prefix32() const noexcept { return (options1_ & Flags1::MASM_ADD_DS_PREFIX32) != 0; }

	/// (masm only): Add a `DS` segment override even if it's not present. Used if it's 16/32-bit code and mem op is a displ
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `mov eax,ds:[12345678]`
	/// _ | `false` | `mov eax,[12345678]`
	void set_masm_add_ds_prefix32(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::MASM_ADD_DS_PREFIX32;
		} else {
			options1_ &= ~Flags1::MASM_ADD_DS_PREFIX32;
		}
	}

	/// (masm only): Show symbols in brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `[ecx+symbol]` / `[symbol]`
	/// _ | `false` | `symbol[ecx]` / `symbol`
	bool masm_symbol_displ_in_brackets() const noexcept { return (options1_ & Flags1::MASM_SYMBOL_DISPL_IN_BRACKETS) != 0; }

	/// (masm only): Show symbols in brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `[ecx+symbol]` / `[symbol]`
	/// _ | `false` | `symbol[ecx]` / `symbol`
	void set_masm_symbol_displ_in_brackets(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::MASM_SYMBOL_DISPL_IN_BRACKETS;
		} else {
			options1_ &= ~Flags1::MASM_SYMBOL_DISPL_IN_BRACKETS;
		}
	}

	/// (masm only): Show displacements in brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `[ecx+1234h]`
	/// _ | `false` | `1234h[ecx]`
	bool masm_displ_in_brackets() const noexcept { return (options1_ & Flags1::MASM_DISPL_IN_BRACKETS) != 0; }

	/// (masm only): Show displacements in brackets
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `[ecx+1234h]`
	/// _ | `false` | `1234h[ecx]`
	void set_masm_displ_in_brackets(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::MASM_DISPL_IN_BRACKETS;
		} else {
			options1_ &= ~Flags1::MASM_DISPL_IN_BRACKETS;
		}
	}

	/// (nasm only): Shows `BYTE`, `WORD`, `DWORD` or `QWORD` if it's a sign extended immediate operand value
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `or rcx,byte -1`
	/// 👍 | `false` | `or rcx,-1`
	bool nasm_show_sign_extended_immediate_size() const noexcept {
		return (options2_ & Flags2::NASM_SHOW_SIGN_EXTENDED_IMMEDIATE_SIZE) != 0;
	}

	/// (nasm only): Shows `BYTE`, `WORD`, `DWORD` or `QWORD` if it's a sign extended immediate operand value
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `or rcx,byte -1`
	/// 👍 | `false` | `or rcx,-1`
	void set_nasm_show_sign_extended_immediate_size(bool value) noexcept {
		if (value) {
			options2_ |= Flags2::NASM_SHOW_SIGN_EXTENDED_IMMEDIATE_SIZE;
		} else {
			options2_ &= ~Flags2::NASM_SHOW_SIGN_EXTENDED_IMMEDIATE_SIZE;
		}
	}

	/// Use `st(0)` instead of `st` if `st` can be used. Ignored by the nasm formatter.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `fadd st(0),st(3)`
	/// 👍 | `false` | `fadd st,st(3)`
	bool prefer_st0() const noexcept { return (options2_ & Flags2::PREFER_ST0) != 0; }

	/// Use `st(0)` instead of `st` if `st` can be used. Ignored by the nasm formatter.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `fadd st(0),st(3)`
	/// 👍 | `false` | `fadd st,st(3)`
	void set_prefer_st0(bool value) noexcept {
		if (value) {
			options2_ |= Flags2::PREFER_ST0;
		} else {
			options2_ &= ~Flags2::PREFER_ST0;
		}
	}

	/// Show useless prefixes. If it has useless prefixes, it could be data and not code.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `es rep add eax,ecx`
	/// 👍 | `false` | `add eax,ecx`
	bool show_useless_prefixes() const noexcept { return (options2_ & Flags2::SHOW_USELESS_PREFIXES) != 0; }

	/// Show useless prefixes. If it has useless prefixes, it could be data and not code.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `es rep add eax,ecx`
	/// 👍 | `false` | `add eax,ecx`
	void set_show_useless_prefixes(bool value) noexcept {
		if (value) {
			options2_ |= Flags2::SHOW_USELESS_PREFIXES;
		} else {
			options2_ &= ~Flags2::SHOW_USELESS_PREFIXES;
		}
	}

	/// Mnemonic condition code selector (eg. `JB` / `JC` / `JNAE`)
	///
	/// Default: `JB`, `CMOVB`, `SETB`
	CC_b cc_b() const noexcept { return cc_b_; }

	/// Mnemonic condition code selector (eg. `JB` / `JC` / `JNAE`)
	///
	/// Default: `JB`, `CMOVB`, `SETB`
	void set_cc_b(CC_b value) noexcept { cc_b_ = value; }

	/// Mnemonic condition code selector (eg. `JAE` / `JNB` / `JNC`)
	///
	/// Default: `JAE`, `CMOVAE`, `SETAE`
	CC_ae cc_ae() const noexcept { return cc_ae_; }

	/// Mnemonic condition code selector (eg. `JAE` / `JNB` / `JNC`)
	///
	/// Default: `JAE`, `CMOVAE`, `SETAE`
	void set_cc_ae(CC_ae value) noexcept { cc_ae_ = value; }

	/// Mnemonic condition code selector (eg. `JE` / `JZ`)
	///
	/// Default: `JE`, `CMOVE`, `SETE`, `LOOPE`, `REPE`
	CC_e cc_e() const noexcept { return cc_e_; }

	/// Mnemonic condition code selector (eg. `JE` / `JZ`)
	///
	/// Default: `JE`, `CMOVE`, `SETE`, `LOOPE`, `REPE`
	void set_cc_e(CC_e value) noexcept { cc_e_ = value; }

	/// Mnemonic condition code selector (eg. `JNE` / `JNZ`)
	///
	/// Default: `JNE`, `CMOVNE`, `SETNE`, `LOOPNE`, `REPNE`
	CC_ne cc_ne() const noexcept { return cc_ne_; }

	/// Mnemonic condition code selector (eg. `JNE` / `JNZ`)
	///
	/// Default: `JNE`, `CMOVNE`, `SETNE`, `LOOPNE`, `REPNE`
	void set_cc_ne(CC_ne value) noexcept { cc_ne_ = value; }

	/// Mnemonic condition code selector (eg. `JBE` / `JNA`)
	///
	/// Default: `JBE`, `CMOVBE`, `SETBE`
	CC_be cc_be() const noexcept { return cc_be_; }

	/// Mnemonic condition code selector (eg. `JBE` / `JNA`)
	///
	/// Default: `JBE`, `CMOVBE`, `SETBE`
	void set_cc_be(CC_be value) noexcept { cc_be_ = value; }

	/// Mnemonic condition code selector (eg. `JA` / `JNBE`)
	///
	/// Default: `JA`, `CMOVA`, `SETA`
	CC_a cc_a() const noexcept { return cc_a_; }

	/// Mnemonic condition code selector (eg. `JA` / `JNBE`)
	///
	/// Default: `JA`, `CMOVA`, `SETA`
	void set_cc_a(CC_a value) noexcept { cc_a_ = value; }

	/// Mnemonic condition code selector (eg. `JP` / `JPE`)
	///
	/// Default: `JP`, `CMOVP`, `SETP`
	CC_p cc_p() const noexcept { return cc_p_; }

	/// Mnemonic condition code selector (eg. `JP` / `JPE`)
	///
	/// Default: `JP`, `CMOVP`, `SETP`
	void set_cc_p(CC_p value) noexcept { cc_p_ = value; }

	/// Mnemonic condition code selector (eg. `JNP` / `JPO`)
	///
	/// Default: `JNP`, `CMOVNP`, `SETNP`
	CC_np cc_np() const noexcept { return cc_np_; }

	/// Mnemonic condition code selector (eg. `JNP` / `JPO`)
	///
	/// Default: `JNP`, `CMOVNP`, `SETNP`
	void set_cc_np(CC_np value) noexcept { cc_np_ = value; }

	/// Mnemonic condition code selector (eg. `JL` / `JNGE`)
	///
	/// Default: `JL`, `CMOVL`, `SETL`
	CC_l cc_l() const noexcept { return cc_l_; }

	/// Mnemonic condition code selector (eg. `JL` / `JNGE`)
	///
	/// Default: `JL`, `CMOVL`, `SETL`
	void set_cc_l(CC_l value) noexcept { cc_l_ = value; }

	/// Mnemonic condition code selector (eg. `JGE` / `JNL`)
	///
	/// Default: `JGE`, `CMOVGE`, `SETGE`
	CC_ge cc_ge() const noexcept { return cc_ge_; }

	/// Mnemonic condition code selector (eg. `JGE` / `JNL`)
	///
	/// Default: `JGE`, `CMOVGE`, `SETGE`
	void set_cc_ge(CC_ge value) noexcept { cc_ge_ = value; }

	/// Mnemonic condition code selector (eg. `JLE` / `JNG`)
	///
	/// Default: `JLE`, `CMOVLE`, `SETLE`
	CC_le cc_le() const noexcept { return cc_le_; }

	/// Mnemonic condition code selector (eg. `JLE` / `JNG`)
	///
	/// Default: `JLE`, `CMOVLE`, `SETLE`
	void set_cc_le(CC_le value) noexcept { cc_le_ = value; }

	/// Mnemonic condition code selector (eg. `JG` / `JNLE`)
	///
	/// Default: `JG`, `CMOVG`, `SETG`
	CC_g cc_g() const noexcept { return cc_g_; }

	/// Mnemonic condition code selector (eg. `JG` / `JNLE`)
	///
	/// Default: `JG`, `CMOVG`, `SETG`
	void set_cc_g(CC_g value) noexcept { cc_g_ = value; }

	/// Compares all options
	bool operator==(const FormatterOptions& other) const noexcept {
		return hex_prefix_ == other.hex_prefix_ && hex_suffix_ == other.hex_suffix_ && decimal_prefix_ == other.decimal_prefix_ &&
			   decimal_suffix_ == other.decimal_suffix_ && octal_prefix_ == other.octal_prefix_ && octal_suffix_ == other.octal_suffix_ &&
			   binary_prefix_ == other.binary_prefix_ && binary_suffix_ == other.binary_suffix_ && digit_separator_ == other.digit_separator_ &&
			   hex_digit_group_size_ == other.hex_digit_group_size_ && decimal_digit_group_size_ == other.decimal_digit_group_size_ &&
			   octal_digit_group_size_ == other.octal_digit_group_size_ && binary_digit_group_size_ == other.binary_digit_group_size_ &&
			   options1_ == other.options1_ && options2_ == other.options2_ && first_operand_char_index_ == other.first_operand_char_index_ &&
			   tab_size_ == other.tab_size_ && number_base_ == other.number_base_ && memory_size_options_ == other.memory_size_options_ &&
			   cc_b_ == other.cc_b_ && cc_ae_ == other.cc_ae_ && cc_e_ == other.cc_e_ && cc_ne_ == other.cc_ne_ && cc_be_ == other.cc_be_ &&
			   cc_a_ == other.cc_a_ && cc_p_ == other.cc_p_ && cc_np_ == other.cc_np_ && cc_l_ == other.cc_l_ && cc_ge_ == other.cc_ge_ &&
			   cc_le_ == other.cc_le_ && cc_g_ == other.cc_g_;
	}
	/// Compares all options
	bool operator!=(const FormatterOptions& other) const noexcept { return !(*this == other); }

private:
	struct Flags1 {
		static constexpr std::uint32_t UPPERCASE_PREFIXES = 0x0000'0001;
		static constexpr std::uint32_t UPPERCASE_MNEMONICS = 0x0000'0002;
		static constexpr std::uint32_t UPPERCASE_REGISTERS = 0x0000'0004;
		static constexpr std::uint32_t UPPERCASE_KEYWORDS = 0x0000'0008;
		static constexpr std::uint32_t UPPERCASE_DECORATORS = 0x0000'0010;
		static constexpr std::uint32_t UPPERCASE_ALL = 0x0000'0020;
		static constexpr std::uint32_t SPACE_AFTER_OPERAND_SEPARATOR = 0x0000'0040;
		static constexpr std::uint32_t SPACE_AFTER_MEMORY_BRACKET = 0x0000'0080;
		static constexpr std::uint32_t SPACE_BETWEEN_MEMORY_ADD_OPERATORS = 0x0000'0100;
		static constexpr std::uint32_t SPACE_BETWEEN_MEMORY_MUL_OPERATORS = 0x0000'0200;
		static constexpr std::uint32_t SCALE_BEFORE_INDEX = 0x0000'0400;
		static constexpr std::uint32_t ALWAYS_SHOW_SCALE = 0x0000'0800;
		static constexpr std::uint32_t ALWAYS_SHOW_SEGMENT_REGISTER = 0x0000'1000;
		static constexpr std::uint32_t SHOW_ZERO_DISPLACEMENTS = 0x0000'2000;
		static constexpr std::uint32_t LEADING_ZEROS = 0x0000'4000;
		static constexpr std::uint32_t UPPERCASE_HEX = 0x0000'8000;
		static constexpr std::uint32_t SMALL_HEX_NUMBERS_IN_DECIMAL = 0x0001'0000;
		static constexpr std::uint32_t ADD_LEADING_ZERO_TO_HEX_NUMBERS = 0x0002'0000;
		static constexpr std::uint32_t BRANCH_LEADING_ZEROS = 0x0004'0000;
		static constexpr std::uint32_t SIGNED_IMMEDIATE_OPERANDS = 0x0008'0000;
		static constexpr std::uint32_t SIGNED_MEMORY_DISPLACEMENTS = 0x0010'0000;
		static constexpr std::uint32_t DISPLACEMENT_LEADING_ZEROS = 0x0020'0000;
		static constexpr std::uint32_t RIP_RELATIVE_ADDRESSES = 0x0040'0000;
		static constexpr std::uint32_t SHOW_BRANCH_SIZE = 0x0080'0000;
		static constexpr std::uint32_t USE_PSEUDO_OPS = 0x0100'0000;
		static constexpr std::uint32_t SHOW_SYMBOL_ADDRESS = 0x0200'0000;
		static constexpr std::uint32_t GAS_NAKED_REGISTERS = 0x0400'0000;
		static constexpr std::uint32_t GAS_SHOW_MNEMONIC_SIZE_SUFFIX = 0x0800'0000;
		static constexpr std::uint32_t GAS_SPACE_AFTER_MEMORY_OPERAND_COMMA = 0x1000'0000;
		static constexpr std::uint32_t MASM_ADD_DS_PREFIX32 = 0x2000'0000;
		static constexpr std::uint32_t MASM_SYMBOL_DISPL_IN_BRACKETS = 0x4000'0000;
		static constexpr std::uint32_t MASM_DISPL_IN_BRACKETS = 0x8000'0000;
	};

	struct Flags2 {
		static constexpr std::uint32_t NASM_SHOW_SIGN_EXTENDED_IMMEDIATE_SIZE = 0x0000'0001;
		static constexpr std::uint32_t PREFER_ST0 = 0x0000'0002;
		static constexpr std::uint32_t SHOW_USELESS_PREFIXES = 0x0000'0004;
	};

	std::string hex_prefix_;
	std::string hex_suffix_;
	std::string decimal_prefix_;
	std::string decimal_suffix_;
	std::string octal_prefix_;
	std::string octal_suffix_;
	std::string binary_prefix_;
	std::string binary_suffix_;
	std::string digit_separator_;
	std::uint32_t hex_digit_group_size_;
	std::uint32_t decimal_digit_group_size_;
	std::uint32_t octal_digit_group_size_;
	std::uint32_t binary_digit_group_size_;
	std::uint32_t options1_;
	std::uint32_t options2_;
	std::uint32_t first_operand_char_index_;
	std::uint32_t tab_size_;
	NumberBase number_base_;
	MemorySizeOptions memory_size_options_;
	CC_b cc_b_;
	CC_ae cc_ae_;
	CC_e cc_e_;
	CC_ne cc_ne_;
	CC_be cc_be_;
	CC_a cc_a_;
	CC_p cc_p_;
	CC_np cc_np_;
	CC_l cc_l_;
	CC_ge cc_ge_;
	CC_le cc_le_;
	CC_g cc_g_;
};

namespace internal {
struct FormatterOperandOptionsFlags {
	static constexpr std::uint32_t NONE = 0x0000'0000;
	static constexpr std::uint32_t NO_BRANCH_SIZE = 0x0000'0001;
	static constexpr std::uint32_t RIP_RELATIVE_ADDRESSES = 0x0000'0002;
	static constexpr std::uint32_t MEMORY_SIZE_SHIFT = 30;
	static constexpr std::uint32_t MEMORY_SIZE_MASK = 3U << MEMORY_SIZE_SHIFT;
};
} // namespace internal

/// Operand options
class FormatterOperandOptions {
public:
	/// Creates default operand options
	constexpr FormatterOperandOptions() noexcept : flags_(0) {}

	/// Creates operand options (used by the formatters). `flags` is a combination of `internal::FormatterOperandOptionsFlags` values
	explicit constexpr FormatterOperandOptions(std::uint32_t flags) noexcept : flags_(flags) {}

	/// Creates operand options (used by the formatters)
	static constexpr FormatterOperandOptions with_memory_size_options(MemorySizeOptions options) noexcept {
		return FormatterOperandOptions(static_cast<std::uint32_t>(options) << internal::FormatterOperandOptionsFlags::MEMORY_SIZE_SHIFT);
	}

	/// Show branch size (eg. `SHORT`, `NEAR PTR`)
	constexpr bool branch_size() const noexcept { return (flags_ & internal::FormatterOperandOptionsFlags::NO_BRANCH_SIZE) == 0; }

	/// Show branch size (eg. `SHORT`, `NEAR PTR`)
	void set_branch_size(bool value) noexcept {
		if (value)
			flags_ &= ~internal::FormatterOperandOptionsFlags::NO_BRANCH_SIZE;
		else
			flags_ |= internal::FormatterOperandOptionsFlags::NO_BRANCH_SIZE;
	}

	/// If `true`, show `RIP` relative addresses as `[rip+12345678h]`, else show the linear address eg. `[1029384756AFBECDh]`
	constexpr bool rip_relative_addresses() const noexcept { return (flags_ & internal::FormatterOperandOptionsFlags::RIP_RELATIVE_ADDRESSES) != 0; }

	/// If `true`, show `RIP` relative addresses as `[rip+12345678h]`, else show the linear address eg. `[1029384756AFBECDh]`
	void set_rip_relative_addresses(bool value) noexcept {
		if (value)
			flags_ |= internal::FormatterOperandOptionsFlags::RIP_RELATIVE_ADDRESSES;
		else
			flags_ &= ~internal::FormatterOperandOptionsFlags::RIP_RELATIVE_ADDRESSES;
	}

	/// Memory size options
	constexpr MemorySizeOptions memory_size_options() const noexcept {
		return static_cast<MemorySizeOptions>(flags_ >> internal::FormatterOperandOptionsFlags::MEMORY_SIZE_SHIFT);
	}

	/// Memory size options
	void set_memory_size_options(MemorySizeOptions value) noexcept {
		flags_ = (flags_ & ~internal::FormatterOperandOptionsFlags::MEMORY_SIZE_MASK) |
				 (static_cast<std::uint32_t>(value) << internal::FormatterOperandOptionsFlags::MEMORY_SIZE_SHIFT);
	}

	/// Compares the options
	constexpr bool operator==(const FormatterOperandOptions& other) const noexcept { return flags_ == other.flags_; }
	/// Compares the options
	constexpr bool operator!=(const FormatterOperandOptions& other) const noexcept { return flags_ != other.flags_; }

private:
	std::uint32_t flags_;
};

/// Gets initialized with the default options and can be overridden by a `FormatterOptionsProvider`
///
/// The string members point to strings owned by the `FormatterOptions` (or any other string that outlives this value).
struct NumberFormattingOptions {
	/// Number prefix or an empty string
	std::string_view prefix;
	/// Number suffix or an empty string
	std::string_view suffix;
	/// Digit separator or an empty string to not use a digit separator
	std::string_view digit_separator;
	/// Size of a digit group or 0 to not use a digit separator
	std::uint8_t digit_group_size = 0;
	/// Number base
	NumberBase number_base = NumberBase::Hexadecimal;
	/// Use uppercase hex digits
	bool uppercase_hex = false;
	/// Small hex numbers (-9 .. 9) are shown in decimal
	bool small_hex_numbers_in_decimal = false;
	/// Add a leading zero to hex numbers if there's no prefix and the number starts with hex digits `A-F`
	bool add_leading_zero_to_hex_numbers = false;
	/// If `true`, add leading zeros to numbers, eg. `1h` vs `00000001h`
	bool leading_zeros = false;
	/// If `true`, the number is signed, and if `false` it's an unsigned number
	bool signed_number = false;
	/// Add leading zeros to displacements
	bool displacement_leading_zeros = false;

	/// Creates default options (all strings are empty, hexadecimal, all other options are `false`/`0`)
	NumberFormattingOptions() noexcept = default;

	/// Constructor
	///
	/// # Arguments
	///
	/// * `options`: Formatter options to use
	/// * `leading_zeros`: Add leading zeros to numbers, eg. `1h` vs `00000001h`
	/// * `signed_number`: Signed numbers if `true`, and unsigned numbers if `false`
	/// * `displacement_leading_zeros`: Add leading zeros to displacements
	NumberFormattingOptions(const FormatterOptions& options, bool leading_zeros_, bool signed_number_, bool displacement_leading_zeros_) noexcept
		: digit_separator(options.digit_separator())
		, number_base(options.number_base())
		, uppercase_hex(options.uppercase_hex())
		, small_hex_numbers_in_decimal(options.small_hex_numbers_in_decimal())
		, add_leading_zero_to_hex_numbers(options.add_leading_zero_to_hex_numbers())
		, leading_zeros(leading_zeros_)
		, signed_number(signed_number_)
		, displacement_leading_zeros(displacement_leading_zeros_) {
		std::uint32_t group_size;
		switch (options.number_base()) {
		case NumberBase::Hexadecimal:
			group_size = options.hex_digit_group_size();
			prefix = options.hex_prefix();
			suffix = options.hex_suffix();
			break;
		case NumberBase::Decimal:
			group_size = options.decimal_digit_group_size();
			prefix = options.decimal_prefix();
			suffix = options.decimal_suffix();
			break;
		case NumberBase::Octal:
			group_size = options.octal_digit_group_size();
			prefix = options.octal_prefix();
			suffix = options.octal_suffix();
			break;
		case NumberBase::Binary:
		default:
			group_size = options.binary_digit_group_size();
			prefix = options.binary_prefix();
			suffix = options.binary_suffix();
			break;
		}
		digit_group_size = static_cast<std::uint8_t>(std::min<std::uint32_t>(0xFF, group_size));
	}

	/// Creates options used when formatting immediate values
	///
	/// # Arguments
	///
	/// * `options`: Formatter options to use
	static NumberFormattingOptions with_immediate(const FormatterOptions& options) noexcept {
		return NumberFormattingOptions(options, options.leading_zeros(), options.signed_immediate_operands(), false);
	}

	/// Creates options used when formatting displacements
	///
	/// # Arguments
	///
	/// * `options`: Formatter options to use
	static NumberFormattingOptions with_displacement(const FormatterOptions& options) noexcept {
		return NumberFormattingOptions(options, options.leading_zeros(), options.signed_memory_displacements(), options.displacement_leading_zeros());
	}

	/// Creates options used when formatting branch operands
	///
	/// # Arguments
	///
	/// * `options`: Formatter options to use
	static NumberFormattingOptions with_branch(const FormatterOptions& options) noexcept {
		return NumberFormattingOptions(options, options.branch_leading_zeros(), false, false);
	}
};

/// Can override options used by a `Formatter`
class FormatterOptionsProvider {
public:
	virtual ~FormatterOptionsProvider() = default;

	/// Called by the formatter. The method can override any options before the formatter uses them.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `options`: Options. Only those options that will be used by the formatter are initialized.
	/// - `number_options`: Number formatting options
	virtual void operand_options(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
								 FormatterOperandOptions& options, NumberFormattingOptions& number_options) = 0;
};

} // namespace iced_x86
