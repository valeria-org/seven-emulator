// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/condition_code.hpp"
#include "iced_x86/cpuid_feature.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/flow_control.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/mnemonic.hpp"
#include "iced_x86/slice.hpp"

namespace iced_x86 {

class OpCodeInfo;

namespace internal {
extern const Mnemonic TO_MNEMONIC[IcedConstants::CODE_ENUM_COUNT];

constexpr bool code_in_range(Code code, Code first, Code last) noexcept {
	return static_cast<std::uint32_t>(code) - static_cast<std::uint32_t>(first) <= static_cast<std::uint32_t>(last) - static_cast<std::uint32_t>(first);
}
} // namespace internal

/// `Code` helper methods (Rust: `impl Code`)
///
/// Functions that are implemented by other components (they're only linked if they're used):
/// - `op_code()`: encoder (op code info)
/// - `encoding()`, `cpuid_features()`, `flow_control()`, `is_privileged()`, `is_stack_instruction()`, `is_save_restore_instruction()`:
///   instruction info
namespace code_ext {

/// Gets the mnemonic
inline Mnemonic mnemonic(Code code) noexcept { return internal::TO_MNEMONIC[static_cast<std::size_t>(code)]; }

/// Gets a `OpCodeInfo`
const OpCodeInfo& op_code(Code code) noexcept;

/// Gets the encoding, eg. Legacy, 3DNow!, VEX, EVEX, XOP
EncodingKind encoding(Code code) noexcept;

/// Gets the CPU or CPUID feature flags
Slice<CpuidFeature> cpuid_features(Code code) noexcept;

/// Gets control flow info
FlowControl flow_control(Code code) noexcept;

/// Checks if it's a privileged instruction (all CPL=0 instructions (except `VMCALL`) and IOPL instructions `IN`, `INS`, `OUT`, `OUTS`, `CLI`, `STI`)
bool is_privileged(Code code) noexcept;

/// Checks if this is an instruction that implicitly uses the stack pointer (`SP`/`ESP`/`RSP`), eg. `CALL`, `PUSH`, `POP`, `RET`, etc.
/// See also `Instruction::stack_pointer_increment()`
bool is_stack_instruction(Code code) noexcept;

/// Checks if it's an instruction that saves or restores too many registers (eg. `FXRSTOR`, `XSAVE`, etc).
bool is_save_restore_instruction(Code code) noexcept;

/// Checks if it's a `Jcc NEAR` instruction
constexpr bool is_jcc_near(Code code) noexcept { return internal::code_in_range(code, Code::Jo_rel16, Code::Jg_rel32_64); }

/// Checks if it's a `Jcc SHORT` instruction
constexpr bool is_jcc_short(Code code) noexcept { return internal::code_in_range(code, Code::Jo_rel8_16, Code::Jg_rel8_64); }

/// Checks if it's a `JMP SHORT` instruction
constexpr bool is_jmp_short(Code code) noexcept { return internal::code_in_range(code, Code::Jmp_rel8_16, Code::Jmp_rel8_64); }

/// Checks if it's a `JMP NEAR` instruction
constexpr bool is_jmp_near(Code code) noexcept { return internal::code_in_range(code, Code::Jmp_rel16, Code::Jmp_rel32_64); }

/// Checks if it's a `JMP SHORT` or a `JMP NEAR` instruction
constexpr bool is_jmp_short_or_near(Code code) noexcept {
	return internal::code_in_range(code, Code::Jmp_rel8_16, Code::Jmp_rel8_64) || internal::code_in_range(code, Code::Jmp_rel16, Code::Jmp_rel32_64);
}

/// Checks if it's a `JMP FAR` instruction
constexpr bool is_jmp_far(Code code) noexcept { return internal::code_in_range(code, Code::Jmp_ptr1616, Code::Jmp_ptr1632); }

/// Checks if it's a `CALL NEAR` instruction
constexpr bool is_call_near(Code code) noexcept { return internal::code_in_range(code, Code::Call_rel16, Code::Call_rel32_64); }

/// Checks if it's a `CALL FAR` instruction
constexpr bool is_call_far(Code code) noexcept { return internal::code_in_range(code, Code::Call_ptr1616, Code::Call_ptr1632); }

/// Checks if it's a `JMP NEAR reg/[mem]` instruction
constexpr bool is_jmp_near_indirect(Code code) noexcept { return internal::code_in_range(code, Code::Jmp_rm16, Code::Jmp_rm64); }

/// Checks if it's a `JMP FAR [mem]` instruction
constexpr bool is_jmp_far_indirect(Code code) noexcept { return internal::code_in_range(code, Code::Jmp_m1616, Code::Jmp_m1664); }

/// Checks if it's a `CALL NEAR reg/[mem]` instruction
constexpr bool is_call_near_indirect(Code code) noexcept { return internal::code_in_range(code, Code::Call_rm16, Code::Call_rm64); }

/// Checks if it's a `CALL FAR [mem]` instruction
constexpr bool is_call_far_indirect(Code code) noexcept { return internal::code_in_range(code, Code::Call_m1616, Code::Call_m1664); }

/// Checks if it's a `JKccD SHORT` or `JKccD NEAR` instruction
constexpr bool is_jkcc_short_or_near(Code code) noexcept {
	return code == Code::VEX_KNC_Jkzd_kr_rel8_64 || code == Code::VEX_KNC_Jknzd_kr_rel8_64 || code == Code::VEX_KNC_Jkzd_kr_rel32_64 ||
		code == Code::VEX_KNC_Jknzd_kr_rel32_64;
}

/// Checks if it's a `JKccD NEAR` instruction
constexpr bool is_jkcc_near(Code code) noexcept { return code == Code::VEX_KNC_Jkzd_kr_rel32_64 || code == Code::VEX_KNC_Jknzd_kr_rel32_64; }

/// Checks if it's a `JKccD SHORT` instruction
constexpr bool is_jkcc_short(Code code) noexcept { return code == Code::VEX_KNC_Jkzd_kr_rel8_64 || code == Code::VEX_KNC_Jknzd_kr_rel8_64; }

/// Gets the condition code if it's `Jcc`, `SETcc`, `CMOVcc`, `CMPccXADD`, `LOOPcc` else `ConditionCode::None` is returned
ConditionCode condition_code(Code code) noexcept;

/// `true` if this `Code` corresponds to a "string" operation, such as `MOVS`, `LODS`,
/// `STOS`, etc.
constexpr bool is_string_instruction(Code code) noexcept {
	switch (code) {
	// GENERATOR-BEGIN: IsStringOpTable
	// ⚠️This was generated by GENERATOR!🦹‍♂️
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
	// GENERATOR-END: IsStringOpTable
		return true;
	default:
		return false;
	}
}

/// Checks if it's a `JCXZ SHORT`, `JECXZ SHORT` or `JRCXZ SHORT` instruction
constexpr bool is_jcx_short(Code code) noexcept { return internal::code_in_range(code, Code::Jcxz_rel8_16, Code::Jrcxz_rel8_64); }

/// Checks if it's a `LOOPcc SHORT` instruction
constexpr bool is_loopcc(Code code) noexcept { return internal::code_in_range(code, Code::Loopne_rel8_16_CX, Code::Loope_rel8_64_RCX); }

/// Checks if it's a `LOOP SHORT` instruction
constexpr bool is_loop(Code code) noexcept { return internal::code_in_range(code, Code::Loop_rel8_16_CX, Code::Loop_rel8_64_RCX); }

/// Checks if it's a `Jcc SHORT` or `Jcc NEAR` instruction
constexpr bool is_jcc_short_or_near(Code code) noexcept {
	return internal::code_in_range(code, Code::Jo_rel8_16, Code::Jg_rel8_64) || internal::code_in_range(code, Code::Jo_rel16, Code::Jg_rel32_64);
}

/// Negates the condition code, eg. `JE` -> `JNE`. Can be used if it's `Jcc`, `SETcc`, `CMOVcc`, `CMPccXADD`, `LOOPcc`
/// and returns the original value if it's none of those instructions.
Code negate_condition_code(Code code) noexcept;

/// Converts `Jcc/JMP NEAR` to `Jcc/JMP SHORT`. Returns the input if it's not a `Jcc/JMP NEAR` instruction.
Code as_short_branch(Code code) noexcept;

/// Converts `Jcc/JMP SHORT` to `Jcc/JMP NEAR`. Returns the input if it's not a `Jcc/JMP SHORT` instruction.
Code as_near_branch(Code code) noexcept;

} // namespace code_ext

} // namespace iced_x86
