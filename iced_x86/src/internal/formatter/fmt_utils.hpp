// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Helpers used by the gas/intel/masm/nasm formatters (Rust: formatter/fmt_utils.rs)

#pragma once

#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/formatter_options.hpp"
#include "iced_x86/formatter_output.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/internal/fmt_utils_all.hpp"
#include "iced_x86/prefix_kind.hpp"
#include "iced_x86/register.hpp"
#include "internal/code_internal.hpp"
#include "internal/formatter/formatter_flow_control.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

/// Writes spaces and/or tabs so the next char is written at column `first_operand_char_index`
void add_tabs(FormatterOutput& output, std::uint32_t column, std::uint32_t first_operand_char_index, std::uint32_t tab_size);

inline bool is_call(FormatterFlowControl kind) noexcept { return kind == FormatterFlowControl::NearCall || kind == FormatterFlowControl::FarCall; }

/// Gets the flow control kind of a branch instruction (generated, see `fmt_flow_control.cpp`)
FormatterFlowControl get_flow_control(const Instruction& instruction) noexcept;

inline bool show_rep_or_repe_prefix(Code code, const FormatterOptions& options) noexcept {
	return show_rep_or_repe_prefix_bool(code, options.show_useless_prefixes());
}

inline bool show_repne_prefix(Code code, const FormatterOptions& options) noexcept {
	return show_repne_prefix_bool(code, options.show_useless_prefixes());
}

inline PrefixKind get_segment_register_prefix_kind(Register register_) noexcept {
	ICED_DEBUG_ASSERT(register_ == Register::ES || register_ == Register::CS || register_ == Register::SS || register_ == Register::DS ||
					  register_ == Register::FS || register_ == Register::GS);
	static_assert(static_cast<std::uint32_t>(PrefixKind::ES) + 1 == static_cast<std::uint32_t>(PrefixKind::CS), "");
	static_assert(static_cast<std::uint32_t>(PrefixKind::ES) + 2 == static_cast<std::uint32_t>(PrefixKind::SS), "");
	static_assert(static_cast<std::uint32_t>(PrefixKind::ES) + 3 == static_cast<std::uint32_t>(PrefixKind::DS), "");
	static_assert(static_cast<std::uint32_t>(PrefixKind::ES) + 4 == static_cast<std::uint32_t>(PrefixKind::FS), "");
	static_assert(static_cast<std::uint32_t>(PrefixKind::ES) + 5 == static_cast<std::uint32_t>(PrefixKind::GS), "");
	return static_cast<PrefixKind>((static_cast<std::uint32_t>(register_) - static_cast<std::uint32_t>(Register::ES)) +
								   static_cast<std::uint32_t>(PrefixKind::ES));
}

inline bool show_index_scale(const Instruction& instruction, const FormatterOptions& options) noexcept {
	return options.show_useless_prefixes() || !code_ignores_index(instruction.code());
}

inline bool show_segment_prefix(Register default_seg_reg, const Instruction& instruction, const FormatterOptions& options) noexcept {
	return show_segment_prefix_bool(default_seg_reg, instruction, options.show_useless_prefixes());
}

inline bool can_show_rounding_control(const Instruction& instruction, const FormatterOptions& options) noexcept {
	const Code code = instruction.code();
	if (code == Code::EVEX_Vcvtsi2sd_xmm_xmm_rm32_er || code == Code::EVEX_Vcvtusi2sd_xmm_xmm_rm32_er ||
		code == Code::EVEX_Vcvtdq2pd_zmm_k1z_ymmm256b32_er || code == Code::EVEX_Vcvtudq2pd_zmm_k1z_ymmm256b32_er)
		return options.show_useless_prefixes();
	return true;
}

} // namespace iced_x86::internal
