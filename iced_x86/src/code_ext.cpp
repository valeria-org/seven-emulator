// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/code_ext.hpp"

namespace iced_x86::code_ext {

namespace {
constexpr std::uint32_t u32(Code code) noexcept { return static_cast<std::uint32_t>(code); }
constexpr Code to_code(std::uint32_t value) noexcept { return static_cast<Code>(value); }
constexpr ConditionCode to_cc(std::uint32_t value) noexcept { return static_cast<ConditionCode>(value); }
} // namespace

ConditionCode condition_code(Code code) noexcept {
	std::uint32_t t;
	const std::uint32_t cc_o = static_cast<std::uint32_t>(ConditionCode::o);

	t = u32(code) - u32(Code::Jo_rel16);
	if (t <= u32(Code::Jg_rel32_64) - u32(Code::Jo_rel16))
		return to_cc((t / 3) + cc_o);

	t = u32(code) - u32(Code::Jo_rel8_16);
	if (t <= u32(Code::Jg_rel8_64) - u32(Code::Jo_rel8_16))
		return to_cc((t / 3) + cc_o);

	t = u32(code) - u32(Code::Cmovo_r16_rm16);
	if (t <= u32(Code::Cmovg_r64_rm64) - u32(Code::Cmovo_r16_rm16))
		return to_cc((t / 3) + cc_o);

	t = u32(code) - u32(Code::Seto_rm8);
	if (t <= u32(Code::Setg_rm8) - u32(Code::Seto_rm8))
		return to_cc(t + cc_o);

	t = u32(code) - u32(Code::Loopne_rel8_16_CX);
	if (t <= u32(Code::Loopne_rel8_64_RCX) - u32(Code::Loopne_rel8_16_CX))
		return ConditionCode::ne;

	t = u32(code) - u32(Code::Loope_rel8_16_CX);
	if (t <= u32(Code::Loope_rel8_64_RCX) - u32(Code::Loope_rel8_16_CX))
		return ConditionCode::e;

	t = u32(code) - u32(Code::VEX_Cmpoxadd_m32_r32_r32);
	if (t <= u32(Code::VEX_Cmpnlexadd_m64_r64_r64) - u32(Code::VEX_Cmpoxadd_m32_r32_r32))
		return to_cc((t / 2) + cc_o);

	switch (code) {
	case Code::VEX_KNC_Jkzd_kr_rel8_64:
	case Code::VEX_KNC_Jkzd_kr_rel32_64:
		return ConditionCode::e;
	case Code::VEX_KNC_Jknzd_kr_rel8_64:
	case Code::VEX_KNC_Jknzd_kr_rel32_64:
		return ConditionCode::ne;
	default:
		break;
	}

	return ConditionCode::None;
}

Code negate_condition_code(Code code) noexcept {
	std::uint32_t t;

	t = u32(code) - u32(Code::Jo_rel16);
	if (t <= u32(Code::Jg_rel32_64) - u32(Code::Jo_rel16)) {
		// They're ordered, eg. je_16, je_32, je_64, jne_16, jne_32, jne_64
		// if low 3, add 3, else if high 3, subtract 3.
		if (((t / 3) & 1) != 0)
			return to_code(u32(code) - 3);
		return to_code(u32(code) + 3);
	}

	t = u32(code) - u32(Code::Jo_rel8_16);
	if (t <= u32(Code::Jg_rel8_64) - u32(Code::Jo_rel8_16)) {
		if (((t / 3) & 1) != 0)
			return to_code(u32(code) - 3);
		return to_code(u32(code) + 3);
	}

	t = u32(code) - u32(Code::Cmovo_r16_rm16);
	if (t <= u32(Code::Cmovg_r64_rm64) - u32(Code::Cmovo_r16_rm16)) {
		if (((t / 3) & 1) != 0)
			return to_code(u32(code) - 3);
		return to_code(u32(code) + 3);
	}

	t = u32(code) - u32(Code::Seto_rm8);
	if (t <= u32(Code::Setg_rm8) - u32(Code::Seto_rm8))
		return to_code((t ^ 1) + u32(Code::Seto_rm8));

	static_assert(static_cast<std::uint32_t>(Code::Loopne_rel8_16_CX) + 7 == static_cast<std::uint32_t>(Code::Loope_rel8_16_CX), "");
	t = u32(code) - u32(Code::Loopne_rel8_16_CX);
	if (t <= u32(Code::Loope_rel8_64_RCX) - u32(Code::Loopne_rel8_16_CX))
		return to_code(u32(Code::Loopne_rel8_16_CX) + (t + 7) % 14);

	t = u32(code) - u32(Code::VEX_Cmpoxadd_m32_r32_r32);
	if (t <= u32(Code::VEX_Cmpnlexadd_m64_r64_r64) - u32(Code::VEX_Cmpoxadd_m32_r32_r32)) {
		if ((t & 2) != 0)
			return to_code(u32(code) - 2);
		return to_code(u32(code) + 2);
	}

	switch (code) {
	case Code::VEX_KNC_Jkzd_kr_rel8_64:
		return Code::VEX_KNC_Jknzd_kr_rel8_64;
	case Code::VEX_KNC_Jknzd_kr_rel8_64:
		return Code::VEX_KNC_Jkzd_kr_rel8_64;
	case Code::VEX_KNC_Jkzd_kr_rel32_64:
		return Code::VEX_KNC_Jknzd_kr_rel32_64;
	case Code::VEX_KNC_Jknzd_kr_rel32_64:
		return Code::VEX_KNC_Jkzd_kr_rel32_64;
	default:
		break;
	}

	return code;
}

Code as_short_branch(Code code) noexcept {
	std::uint32_t t;

	t = u32(code) - u32(Code::Jo_rel16);
	if (t <= u32(Code::Jg_rel32_64) - u32(Code::Jo_rel16))
		return to_code(t + u32(Code::Jo_rel8_16));

	t = u32(code) - u32(Code::Jmp_rel16);
	if (t <= u32(Code::Jmp_rel32_64) - u32(Code::Jmp_rel16))
		return to_code(t + u32(Code::Jmp_rel8_16));

	switch (code) {
	case Code::VEX_KNC_Jkzd_kr_rel32_64:
		return Code::VEX_KNC_Jkzd_kr_rel8_64;
	case Code::VEX_KNC_Jknzd_kr_rel32_64:
		return Code::VEX_KNC_Jknzd_kr_rel8_64;
	default:
		break;
	}

	return code;
}

Code as_near_branch(Code code) noexcept {
	std::uint32_t t;

	t = u32(code) - u32(Code::Jo_rel8_16);
	if (t <= u32(Code::Jg_rel8_64) - u32(Code::Jo_rel8_16))
		return to_code(t + u32(Code::Jo_rel16));

	t = u32(code) - u32(Code::Jmp_rel8_16);
	if (t <= u32(Code::Jmp_rel8_64) - u32(Code::Jmp_rel8_16))
		return to_code(t + u32(Code::Jmp_rel16));

	switch (code) {
	case Code::VEX_KNC_Jkzd_kr_rel8_64:
		return Code::VEX_KNC_Jkzd_kr_rel32_64;
	case Code::VEX_KNC_Jknzd_kr_rel8_64:
		return Code::VEX_KNC_Jknzd_kr_rel32_64;
	default:
		break;
	}

	return code;
}

} // namespace iced_x86::code_ext
