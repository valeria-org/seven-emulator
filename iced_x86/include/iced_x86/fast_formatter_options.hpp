// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdint>

namespace iced_x86 {

/// Fast formatter options
class FastFormatterOptions {
public:
	/// Creates the default options
	constexpr FastFormatterOptions() noexcept : options1_(Flags1::USE_PSEUDO_OPS | Flags1::UPPERCASE_HEX) {}

	/// Add a space after the operand separator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov rax, rcx`
	/// 👍 | `false` | `mov rax,rcx`
	constexpr bool space_after_operand_separator() const noexcept { return (options1_ & Flags1::SPACE_AFTER_OPERAND_SEPARATOR) != 0; }

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

	/// Show `RIP+displ` or the virtual address
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rip+12345678h]`
	/// 👍 | `false` | `mov eax,[1029384756AFBECDh]`
	constexpr bool rip_relative_addresses() const noexcept { return (options1_ & Flags1::RIP_RELATIVE_ADDRESSES) != 0; }

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

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// _ | `false` | `vcmpsd xmm2,xmm6,xmm3,5h`
	constexpr bool use_pseudo_ops() const noexcept { return (options1_ & Flags1::USE_PSEUDO_OPS) != 0; }

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// _ | `false` | `vcmpsd xmm2,xmm6,xmm3,5h`
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
	constexpr bool show_symbol_address() const noexcept { return (options1_ & Flags1::SHOW_SYMBOL_ADDRESS) != 0; }

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

	/// Always show the effective segment register. If the option is `false`, only show the segment register if
	/// there's a segment override prefix.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ds:[ecx]`
	/// 👍 | `false` | `mov eax,[ecx]`
	constexpr bool always_show_segment_register() const noexcept { return (options1_ & Flags1::ALWAYS_SHOW_SEGMENT_REGISTER) != 0; }

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

	/// Always show the size of memory operands
	///
	/// Default | Value | Example | Example
	/// --------|-------|---------|--------
	/// _ | `true` | `mov eax,dword ptr [ebx]` | `add byte ptr [eax],0x12`
	/// 👍 | `false` | `mov eax,[ebx]` | `add byte ptr [eax],0x12`
	constexpr bool always_show_memory_size() const noexcept { return (options1_ & Flags1::ALWAYS_SHOW_MEMORY_SIZE) != 0; }

	/// Always show the size of memory operands
	///
	/// Default | Value | Example | Example
	/// --------|-------|---------|--------
	/// _ | `true` | `mov eax,dword ptr [ebx]` | `add byte ptr [eax],0x12`
	/// 👍 | `false` | `mov eax,[ebx]` | `add byte ptr [eax],0x12`
	void set_always_show_memory_size(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::ALWAYS_SHOW_MEMORY_SIZE;
		} else {
			options1_ &= ~Flags1::ALWAYS_SHOW_MEMORY_SIZE;
		}
	}

	/// Use uppercase hex digits
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0xFF`
	/// _ | `false` | `0xff`
	constexpr bool uppercase_hex() const noexcept { return (options1_ & Flags1::UPPERCASE_HEX) != 0; }

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

	/// Use a hex prefix (`0x`) or a hex suffix (`h`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `0x5A`
	/// 👍 | `false` | `5Ah`
	constexpr bool use_hex_prefix() const noexcept { return (options1_ & Flags1::USE_HEX_PREFIX) != 0; }

	/// Use a hex prefix (`0x`) or a hex suffix (`h`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `0x5A`
	/// 👍 | `false` | `5Ah`
	void set_use_hex_prefix(bool value) noexcept {
		if (value) {
			options1_ |= Flags1::USE_HEX_PREFIX;
		} else {
			options1_ &= ~Flags1::USE_HEX_PREFIX;
		}
	}

	/// Compares the options
	constexpr bool operator==(const FastFormatterOptions& other) const noexcept { return options1_ == other.options1_; }
	/// Compares the options
	constexpr bool operator!=(const FastFormatterOptions& other) const noexcept { return options1_ != other.options1_; }

private:
	struct Flags1 {
		static constexpr std::uint32_t SPACE_AFTER_OPERAND_SEPARATOR = 0x0000'0001;
		static constexpr std::uint32_t RIP_RELATIVE_ADDRESSES = 0x0000'0002;
		static constexpr std::uint32_t USE_PSEUDO_OPS = 0x0000'0004;
		static constexpr std::uint32_t SHOW_SYMBOL_ADDRESS = 0x0000'0008;
		static constexpr std::uint32_t ALWAYS_SHOW_SEGMENT_REGISTER = 0x0000'0010;
		static constexpr std::uint32_t ALWAYS_SHOW_MEMORY_SIZE = 0x0000'0020;
		static constexpr std::uint32_t UPPERCASE_HEX = 0x0000'0040;
		static constexpr std::uint32_t USE_HEX_PREFIX = 0x0000'0080;
	};

	std::uint32_t options1_;
};

} // namespace iced_x86
