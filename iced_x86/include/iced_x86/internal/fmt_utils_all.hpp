// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// NOT PART OF THE PUBLIC API. Helpers shared by all formatters (Rust: formatter/fmt_utils_all.rs).
// It's a public header only because the `SpecializedFormatter<TraitOptions>` template uses it.

#pragma once

#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/register.hpp"

namespace iced_x86::internal {

constexpr bool is_rep_repe_repne_instruction(Code code) noexcept {
	switch (code) {
	case Code::Insb_m8_DX:
	case Code::Insw_m16_DX:
	case Code::Insd_m32_DX:
	case Code::Outsb_DX_m8:
	case Code::Outsw_DX_m16:
	case Code::Outsd_DX_m32:
	case Code::Movsb_m8_m8:
	case Code::Movsw_m16_m16:
	case Code::Movsd_m32_m32:
	case Code::Movsq_m64_m64:
	case Code::Cmpsb_m8_m8:
	case Code::Cmpsw_m16_m16:
	case Code::Cmpsd_m32_m32:
	case Code::Cmpsq_m64_m64:
	case Code::Stosb_m8_AL:
	case Code::Stosw_m16_AX:
	case Code::Stosd_m32_EAX:
	case Code::Stosq_m64_RAX:
	case Code::Lodsb_AL_m8:
	case Code::Lodsw_AX_m16:
	case Code::Lodsd_EAX_m32:
	case Code::Lodsq_RAX_m64:
	case Code::Scasb_AL_m8:
	case Code::Scasw_AX_m16:
	case Code::Scasd_EAX_m32:
	case Code::Scasq_RAX_m64:
	case Code::Montmul_16:
	case Code::Montmul_32:
	case Code::Montmul_64:
	case Code::Xsha1_16:
	case Code::Xsha1_32:
	case Code::Xsha1_64:
	case Code::Xsha256_16:
	case Code::Xsha256_32:
	case Code::Xsha256_64:
	case Code::Xstore_16:
	case Code::Xstore_32:
	case Code::Xstore_64:
	case Code::Xcryptecb_16:
	case Code::Xcryptecb_32:
	case Code::Xcryptecb_64:
	case Code::Xcryptcbc_16:
	case Code::Xcryptcbc_32:
	case Code::Xcryptcbc_64:
	case Code::Xcryptctr_16:
	case Code::Xcryptctr_32:
	case Code::Xcryptctr_64:
	case Code::Xcryptcfb_16:
	case Code::Xcryptcfb_32:
	case Code::Xcryptcfb_64:
	case Code::Xcryptofb_16:
	case Code::Xcryptofb_32:
	case Code::Xcryptofb_64:
	case Code::Ccs_hash_16:
	case Code::Ccs_hash_32:
	case Code::Ccs_hash_64:
	case Code::Ccs_encrypt_16:
	case Code::Ccs_encrypt_32:
	case Code::Ccs_encrypt_64:
	case Code::Via_undoc_F30FA6F0_16:
	case Code::Via_undoc_F30FA6F0_32:
	case Code::Via_undoc_F30FA6F0_64:
	case Code::Via_undoc_F30FA6F8_16:
	case Code::Via_undoc_F30FA6F8_32:
	case Code::Via_undoc_F30FA6F8_64:
	case Code::Xsha512_16:
	case Code::Xsha512_32:
	case Code::Xsha512_64:
	case Code::Xstore_alt_16:
	case Code::Xstore_alt_32:
	case Code::Xstore_alt_64:
	case Code::Xsha512_alt_16:
	case Code::Xsha512_alt_32:
	case Code::Xsha512_alt_64:
		return true;
	default:
		return false;
	}
}

constexpr bool show_rep_or_repe_prefix_bool(Code code, bool show_useless_prefixes) noexcept {
	if (show_useless_prefixes || is_rep_repe_repne_instruction(code))
		return true;
	// We allow 'rep ret' too since some old code use it to work around an old AMD bug
	switch (code) {
	case Code::Retnw:
	case Code::Retnd:
	case Code::Retnq:
		return true;
	default:
		return show_useless_prefixes;
	}
}

constexpr bool show_repne_prefix_bool(Code code, bool show_useless_prefixes) noexcept {
	// If it's a 'rep/repne' instruction, always show the prefix
	if (show_useless_prefixes || is_rep_repe_repne_instruction(code))
		return true;
	return show_useless_prefixes;
}

constexpr bool is_code64(CodeSize code_size) noexcept { return code_size == CodeSize::Code64 || code_size == CodeSize::Unknown; }

inline Register get_default_segment_register(const Instruction& instruction) noexcept {
	const Register base_reg = instruction.memory_base();
	if (base_reg == Register::BP || base_reg == Register::EBP || base_reg == Register::ESP || base_reg == Register::RBP || base_reg == Register::RSP)
		return Register::SS;
	return Register::DS;
}

/// Checks if the segment prefix should be shown. The instruction must have a segment prefix.
bool show_segment_prefix_bool(Register default_seg_reg, const Instruction& instruction, bool show_useless_prefixes) noexcept;

constexpr bool is_repe_or_repne_instruction(Code code) noexcept {
	switch (code) {
	case Code::Cmpsb_m8_m8:
	case Code::Cmpsw_m16_m16:
	case Code::Cmpsd_m32_m32:
	case Code::Cmpsq_m64_m64:
	case Code::Scasb_AL_m8:
	case Code::Scasw_AX_m16:
	case Code::Scasd_EAX_m32:
	case Code::Scasq_RAX_m64:
		return true;
	default:
		return false;
	}
}

constexpr bool is_notrack_prefix_branch(Code code) noexcept {
	static_assert(static_cast<std::uint32_t>(Code::Jmp_rm16) + 1 == static_cast<std::uint32_t>(Code::Jmp_rm32), "");
	static_assert(static_cast<std::uint32_t>(Code::Jmp_rm16) + 2 == static_cast<std::uint32_t>(Code::Jmp_rm64), "");
	static_assert(static_cast<std::uint32_t>(Code::Call_rm16) + 1 == static_cast<std::uint32_t>(Code::Call_rm32), "");
	static_assert(static_cast<std::uint32_t>(Code::Call_rm16) + 2 == static_cast<std::uint32_t>(Code::Call_rm64), "");
	return (static_cast<std::uint32_t>(code) - static_cast<std::uint32_t>(Code::Jmp_rm16)) <= 2 ||
		   (static_cast<std::uint32_t>(code) - static_cast<std::uint32_t>(Code::Call_rm16)) <= 2;
}

} // namespace iced_x86::internal
