// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Legacy op code handlers (Rust: decoder/handlers/legacy.rs)

#pragma once

#include "iced_x86/code.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "internal/decoder/handlers.hpp"
#include "internal/decoder/legacy_handler_flags.hpp"

#include <cstdint>

namespace iced_x86::internal {

struct OpCodeHandler_VEX2 : OpCodeHandler {
	const OpCodeHandler* handler_mem;
	explicit constexpr OpCodeHandler_VEX2(const OpCodeHandler* handler_mem_) noexcept : OpCodeHandler(&decode, true), handler_mem(handler_mem_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_mem_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX3 : OpCodeHandler {
	const OpCodeHandler* handler_mem;
	explicit constexpr OpCodeHandler_VEX3(const OpCodeHandler* handler_mem_) noexcept : OpCodeHandler(&decode, true), handler_mem(handler_mem_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_mem_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_XOP : OpCodeHandler {
	const OpCodeHandler* handler_reg0;
	explicit constexpr OpCodeHandler_XOP(const OpCodeHandler* handler_reg0_) noexcept : OpCodeHandler(&decode, true), handler_reg0(handler_reg0_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_reg0_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX : OpCodeHandler {
	const OpCodeHandler* handler_mem;
	explicit constexpr OpCodeHandler_EVEX(const OpCodeHandler* handler_mem_) noexcept : OpCodeHandler(&decode, true), handler_mem(handler_mem_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_mem_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixEsCsSsDs : OpCodeHandler {
	Register seg;
	explicit constexpr OpCodeHandler_PrefixEsCsSsDs(Register seg_) noexcept : OpCodeHandler(&decode, false), seg(seg_) {
		ICED_DEBUG_ASSERT(seg_ == Register::ES || seg_ == Register::CS || seg_ == Register::SS || seg_ == Register::DS);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixFsGs : OpCodeHandler {
	Register seg;
	explicit constexpr OpCodeHandler_PrefixFsGs(Register seg_) noexcept : OpCodeHandler(&decode, false), seg(seg_) {
		ICED_DEBUG_ASSERT(seg_ == Register::FS || seg_ == Register::GS);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Prefix66 : OpCodeHandler {
	constexpr OpCodeHandler_Prefix66() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Prefix67 : OpCodeHandler {
	constexpr OpCodeHandler_Prefix67() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixF0 : OpCodeHandler {
	constexpr OpCodeHandler_PrefixF0() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixF2 : OpCodeHandler {
	constexpr OpCodeHandler_PrefixF2() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixF3 : OpCodeHandler {
	constexpr OpCodeHandler_PrefixF3() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PrefixREX : OpCodeHandler {
	std::uint32_t rex;
	const OpCodeHandler* handler;
	constexpr OpCodeHandler_PrefixREX(const OpCodeHandler* handler_, std::uint32_t rex_) noexcept
		: OpCodeHandler(&decode, false), rex(rex_), handler(handler_) {
		ICED_DEBUG_ASSERT(rex_ <= 0x0F);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Reg(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RegIb : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_RegIb(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_IbReg : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_IbReg(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_AL_DX : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_AL_DX(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_DX_AL : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_DX_AL(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ib : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Ib(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ib3 : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Ib3(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MandatoryPrefix : OpCodeHandler {
	const OpCodeHandler* handlers[4];
	constexpr OpCodeHandler_MandatoryPrefix(bool has_modrm_, const OpCodeHandler* handler, const OpCodeHandler* handler_66, const OpCodeHandler* handler_f3,
		const OpCodeHandler* handler_f2) noexcept
		: OpCodeHandler(&decode, has_modrm_), handlers{handler, handler_66, handler_f3, handler_f2} {
		static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
		static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::P66) == 1, "");
		static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF3) == 2, "");
		static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF2) == 3, "");
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_66));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_f3));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_f2));
		ICED_DEBUG_ASSERT(handlers[0]->has_modrm == has_modrm_);
		ICED_DEBUG_ASSERT(handlers[1]->has_modrm == has_modrm_);
		ICED_DEBUG_ASSERT(handlers[2]->has_modrm == has_modrm_);
		ICED_DEBUG_ASSERT(handlers[3]->has_modrm == has_modrm_);
	}
	ICED_DECODE_FN_DECL;
};

// Rust: `(OpCodeHandlerDecodeFn, &'static OpCodeHandler, bool)`
struct MandatoryPrefix3Info {
	const OpCodeHandler* handler;
	bool mandatory_prefix;
};

struct OpCodeHandler_MandatoryPrefix3 : OpCodeHandler {
	MandatoryPrefix3Info handlers_reg[4];
	MandatoryPrefix3Info handlers_mem[4];
	constexpr OpCodeHandler_MandatoryPrefix3(const OpCodeHandler* handler_reg, const OpCodeHandler* handler_mem, const OpCodeHandler* handler66_reg,
		const OpCodeHandler* handler66_mem, const OpCodeHandler* handlerf3_reg, const OpCodeHandler* handlerf3_mem,
		const OpCodeHandler* handlerf2_reg, const OpCodeHandler* handlerf2_mem, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true),
		  handlers_reg{
			  MandatoryPrefix3Info{handler_reg, (flags & LegacyHandlerFlags::HANDLER_REG) == 0},
			  MandatoryPrefix3Info{handler66_reg, (flags & LegacyHandlerFlags::HANDLER_66_REG) == 0},
			  MandatoryPrefix3Info{handlerf3_reg, (flags & LegacyHandlerFlags::HANDLER_F3_REG) == 0},
			  MandatoryPrefix3Info{handlerf2_reg, (flags & LegacyHandlerFlags::HANDLER_F2_REG) == 0},
		  },
		  handlers_mem{
			  MandatoryPrefix3Info{handler_mem, (flags & LegacyHandlerFlags::HANDLER_MEM) == 0},
			  MandatoryPrefix3Info{handler66_mem, (flags & LegacyHandlerFlags::HANDLER_66_MEM) == 0},
			  MandatoryPrefix3Info{handlerf3_mem, (flags & LegacyHandlerFlags::HANDLER_F3_MEM) == 0},
			  MandatoryPrefix3Info{handlerf2_mem, (flags & LegacyHandlerFlags::HANDLER_F2_MEM) == 0},
		  } {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_reg));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_mem));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler66_reg));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler66_mem));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handlerf3_reg));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handlerf3_mem));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handlerf2_reg));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handlerf2_mem));
		ICED_DEBUG_ASSERT(handlers_reg[0].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_reg[1].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_reg[2].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_reg[3].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_mem[0].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_mem[1].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_mem[2].handler->has_modrm);
		ICED_DEBUG_ASSERT(handlers_mem[3].handler->has_modrm);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MandatoryPrefix4 : OpCodeHandler {
	std::uint32_t flags;
	const OpCodeHandler* handler_np;
	const OpCodeHandler* handler_66;
	const OpCodeHandler* handler_f3;
	const OpCodeHandler* handler_f2;
	constexpr OpCodeHandler_MandatoryPrefix4(const OpCodeHandler* handler_np_, const OpCodeHandler* handler_66_, const OpCodeHandler* handler_f3_,
		const OpCodeHandler* handler_f2_, std::uint32_t flags_) noexcept
		: OpCodeHandler(&decode, false), flags(flags_), handler_np(handler_np_), handler_66(handler_66_), handler_f3(handler_f3_),
		  handler_f2(handler_f2_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_np_));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_66_));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_f3_));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_f2_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_NIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_NIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reservednop : OpCodeHandler {
	const OpCodeHandler* reserved_nop_handler;
	const OpCodeHandler* other_handler;
	constexpr OpCodeHandler_Reservednop(const OpCodeHandler* reserved_nop_handler_, const OpCodeHandler* other_handler_) noexcept
		: OpCodeHandler(&decode, true), reserved_nop_handler(reserved_nop_handler_), other_handler(other_handler_) {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(reserved_nop_handler_));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(other_handler_));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Iz : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Ev_Iz(Code code16, Code code32, Code code64, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64}, reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)},
		  state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Ib : OpCodeHandler {
	OpKind op_kinds[3];
	std::uint32_t reg_base[3];
	std::uint32_t state_flags_or_value;
	Code code[3];
	constexpr OpCodeHandler_Ev_Ib(Code code16, Code code32, Code code64, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), op_kinds{OpKind::Immediate8to16, OpKind::Immediate8to32, OpKind::Immediate8to64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)},
		  state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)), code{code16, code32, code64} {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Ib2 : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Ev_Ib2(Code code16, Code code32, Code code64, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64}, reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)},
		  state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_1 : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_1(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_CL : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_CL(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Ev(Code code16, Code code32, Code code64, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64}, reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)},
		  state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Rv : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Rv(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Rv_32_64 : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Rv_32_64(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Rq : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Rq(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_REXW : OpCodeHandler {
	std::uint32_t flags;
	std::uint32_t disallow_reg;
	std::uint32_t disallow_mem;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ev_REXW(Code code32_, Code code64_, std::uint32_t flags_) noexcept
		: OpCodeHandler(&decode, true), flags(flags_), disallow_reg((flags_ & 1) != 0 ? 0 : UINT32_MAX),
		  disallow_mem((flags_ & 2) != 0 ? 0 : UINT32_MAX), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Evj : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Evj(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), code16(code16_), code32(code32_), code64(code64_),
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ep : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ep(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Evw : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Evw(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ew : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ew(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ms : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ms(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ev(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gd_Rd : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Gd_Rd(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_M_as : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_M_as(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gdq_Ev : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gdq_Ev(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev3 : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ev3(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev2 : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ev2(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_R_C : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_R_C(Code code32_, Code code64_, Register base_reg_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_C_R : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_C_R(Code code32_, Code code64_, Register base_reg_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Jb : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Jb(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Jx : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Jx(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Jz : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Jz(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Jb2 : OpCodeHandler {
	Code code16_16;
	Code code16_32;
	Code code16_64;
	Code code32_16;
	Code code32_32;
	Code code64_32;
	Code code64_64;
	constexpr OpCodeHandler_Jb2(Code code16_16_, Code code16_32_, Code code16_64_, Code code32_16_, Code code32_32_, Code code64_32_, Code code64_64_) noexcept
		: OpCodeHandler(&decode, false), code16_16(code16_16_), code16_32(code16_32_), code16_64(code16_64_), code32_16(code32_16_),
		  code32_32(code32_32_), code64_32(code64_32_), code64_64(code64_64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Jdisp : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Jdisp(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushOpSizeReg : OpCodeHandler {
	Register reg;
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_PushOpSizeReg(Code code16_, Code code32_, Code code64_, Register reg_) noexcept
		: OpCodeHandler(&decode, false), reg(reg_), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushEv : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_PushEv(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_Gv(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv_flags : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Ev_Gv_flags(Code code16, Code code32, Code code64, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64}, reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)},
		  state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		ICED_DEBUG_ASSERT((flags & (HandlerFlags::XACQUIRE | HandlerFlags::XRELEASE)) != 0);
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv_32_64 : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ev_Gv_32_64(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv_Ib : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_Gv_Ib(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv_CL : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_Gv_CL(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Mp : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_Mp(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Eb : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Eb(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ew : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ew(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushSimple2 : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_PushSimple2(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple2 : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Simple2(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple2Iw : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Simple2Iw(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple3 : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Simple3(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple5 : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Simple5(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple5_a32 : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Simple5_a32(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple5_ModRM_as : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Simple5_ModRM_as(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Simple4 : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Simple4(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, false), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushSimpleReg : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	std::uint32_t index;
	constexpr OpCodeHandler_PushSimpleReg(std::uint32_t index_, Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_), index(index_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_SimpleReg : OpCodeHandler {
	Code code;
	std::uint32_t index;
	constexpr OpCodeHandler_SimpleReg(Code code_, std::uint32_t index_) noexcept : OpCodeHandler(&decode, false), code(code_), index(index_) {
		static_assert(static_cast<std::uint32_t>(OpSize::Size16) == 0, "");
		static_assert(static_cast<std::uint32_t>(OpSize::Size32) == 1, "");
		static_assert(static_cast<std::uint32_t>(OpSize::Size64) == 2, "");
		ICED_DEBUG_ASSERT(static_cast<std::uint32_t>(code_) + 2 < static_cast<std::uint32_t>(IcedConstants::CODE_ENUM_COUNT));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Xchg_Reg_rAX : OpCodeHandler {
	std::uint32_t index;
	explicit constexpr OpCodeHandler_Xchg_Reg_rAX(std::uint32_t index_) noexcept : OpCodeHandler(&decode, false), index(index_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Iz : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Reg_Iz(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RegIb3 : OpCodeHandler {
	std::uint32_t index;
	explicit constexpr OpCodeHandler_RegIb3(std::uint32_t index_) noexcept : OpCodeHandler(&decode, false), index(index_) {
		ICED_DEBUG_ASSERT(index_ <= 7);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RegIz2 : OpCodeHandler {
	std::uint32_t index;
	explicit constexpr OpCodeHandler_RegIz2(std::uint32_t index_) noexcept : OpCodeHandler(&decode, false), index(index_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushIb2 : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_PushIb2(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_PushIz : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_PushIz(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ma : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Gv_Ma(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, true), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RvMw_Gw : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_RvMw_Gw(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, true), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev_Ib : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ev_Ib(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev_Ib_REX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_Ev_Ib_REX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev_32_64 : OpCodeHandler {
	std::uint32_t disallow_reg;
	std::uint32_t disallow_mem;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_Ev_32_64(Code code32_, Code code64_, bool allow_reg, bool allow_mem) noexcept
		: OpCodeHandler(&decode, true), disallow_reg(allow_reg ? 0 : UINT32_MAX), disallow_mem(allow_mem ? 0 : UINT32_MAX), code32(code32_),
		  code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev_Iz : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Ev_Iz(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Yb_Reg : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Yb_Reg(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Yv_Reg : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Yv_Reg(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Yv_Reg2 : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Yv_Reg2(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Xb : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Reg_Xb(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Xv : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Reg_Xv(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Xv2 : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Reg_Xv2(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Yb : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Reg_Yb(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Yv : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Reg_Yv(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Yb_Xb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Yb_Xb(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Yv_Xv : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Yv_Xv(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Xb_Yb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Xb_Yb(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Xv_Yv : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Xv_Yv(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Sw : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Ev_Sw(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_M_Sw : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_M_Sw(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_M : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_M(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Sw_Ev : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Sw_Ev(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Sw_M : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Sw_M(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ap : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Ap(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Ob : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Reg_Ob(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ob_Reg : OpCodeHandler {
	Register reg;
	Code code;
	constexpr OpCodeHandler_Ob_Reg(Code code_, Register reg_) noexcept : OpCodeHandler(&decode, false), reg(reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Ov : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Reg_Ov(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ov_Reg : OpCodeHandler {
	Code code[3];
	constexpr OpCodeHandler_Ov_Reg(Code code16, Code code32, Code code64) noexcept : OpCodeHandler(&decode, false), code{code16, code32, code64} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_BranchIw : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_BranchIw(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_BranchSimple : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_BranchSimple(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Iw_Ib : OpCodeHandler {
	Code code16;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Iw_Ib(Code code16_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, false), code16(code16_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Reg_Ib2 : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_Reg_Ib2(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_IbReg2 : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_IbReg2(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_eAX_DX : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_eAX_DX(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_DX_eAX : OpCodeHandler {
	Code code16;
	Code code32;
	constexpr OpCodeHandler_DX_eAX(Code code16_, Code code32_) noexcept : OpCodeHandler(&decode, false), code16(code16_), code32(code32_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Eb_Ib : OpCodeHandler {
	Code code;
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Eb_Ib(Code code_, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code(code_), state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Eb_1 : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Eb_1(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Eb_CL : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Eb_CL(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Eb : OpCodeHandler {
	Code code;
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Eb(Code code_, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code(code_), state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Eb_Gb : OpCodeHandler {
	Code code;
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_Eb_Gb(Code code_, std::uint32_t flags) noexcept
		: OpCodeHandler(&decode, true), code(code_), state_flags_or_value((flags & HandlerFlags::LOCK) << (13 - 3)) {
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gb_Eb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Gb_Eb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_M : OpCodeHandler {
	Code code_w0;
	Code code_w1;
	// Rust: new(code)
	explicit constexpr OpCodeHandler_M(Code code) noexcept : OpCodeHandler(&decode, true), code_w0(code), code_w1(code) {}
	// Rust: new1(code_w0, code_w1)
	constexpr OpCodeHandler_M(Code code_w0_, Code code_w1_) noexcept : OpCodeHandler(&decode, true), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_M_REXW : OpCodeHandler {
	std::uint32_t flags32;
	std::uint32_t flags64;
	Code code32;
	Code code64;
	std::uint32_t state_flags_or_value;
	constexpr OpCodeHandler_M_REXW(Code code32_, Code code64_, std::uint32_t flags32_, std::uint32_t flags64_) noexcept
		: OpCodeHandler(&decode, true), flags32(flags32_), flags64(flags64_), code32(code32_), code64(code64_),
		  state_flags_or_value((flags32_ & HandlerFlags::LOCK) << (13 - 3)) {
		ICED_DEBUG_ASSERT((flags32_ & HandlerFlags::LOCK) == (flags64_ & HandlerFlags::LOCK));
		static_assert(HandlerFlags::LOCK == 1 << 3, "");
		static_assert(StateFlags::ALLOW_LOCK == 1 << 13, "");
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MemBx : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MemBx(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VW : OpCodeHandler {
	Code code_r;
	Code code_m;
	// Rust: new(code)
	explicit constexpr OpCodeHandler_VW(Code code) noexcept : OpCodeHandler(&decode, true), code_r(code), code_m(code) {}
	// Rust: new1(code_r, code_m)
	constexpr OpCodeHandler_VW(Code code_r_, Code code_m_) noexcept : OpCodeHandler(&decode, true), code_r(code_r_), code_m(code_m_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_WV : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_WV(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_rDI_VX_RX : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_rDI_VX_RX(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_rDI_P_N : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_rDI_P_N(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VM : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VM(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MV : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MV(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VQ : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VQ(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_Q : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_P_Q(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Q_P : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_Q_P(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MP : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MP(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_Q_Ib : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_P_Q_Ib(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_W : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_P_W(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_R : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_P_R(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_P_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_P_Ev_Ib : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_P_Ev_Ib(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_P : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ev_P(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_W : OpCodeHandler {
	Code code_w0;
	Code code_w1;
	constexpr OpCodeHandler_Gv_W(Code code_w0_, Code code_w1_) noexcept : OpCodeHandler(&decode, true), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_V_Ev : OpCodeHandler {
	Code code_w0;
	Code code_w1;
	constexpr OpCodeHandler_V_Ev(Code code_w0_, Code code_w1_) noexcept : OpCodeHandler(&decode, true), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VWIb : OpCodeHandler {
	Code code_w0;
	Code code_w1;
	// Rust: new(code)
	explicit constexpr OpCodeHandler_VWIb(Code code) noexcept : OpCodeHandler(&decode, true), code_w0(code), code_w1(code) {}
	// Rust: new1(code_w0, code_w1)
	constexpr OpCodeHandler_VWIb(Code code_w0_, Code code_w1_) noexcept : OpCodeHandler(&decode, true), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VRIbIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VRIbIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RIbIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_RIbIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_RIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_RIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ed_V_Ib : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ed_V_Ib(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VX_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VX_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_VX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ev_VX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VX_E_Ib : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VX_E_Ib(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_RX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_RX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_B_MIB : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_B_MIB(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MIB_B : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MIB_B(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_B_BM : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_B_BM(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_BM_B : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_BM_B(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_B_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	std::uint32_t rip_rel_mask;
	constexpr OpCodeHandler_B_Ev(Code code32_, Code code64_, bool supports_rip_rel) noexcept
		: OpCodeHandler(&decode, true), code32(code32_), code64(code64_), rip_rel_mask(supports_rip_rel ? 0 : UINT32_MAX) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Mv_Gv_REXW : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Mv_Gv_REXW(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_N_Ib_REX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_N_Ib_REX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_N : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_N(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VN : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VN(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Mv : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Gv_Mv(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Mv_Gv : OpCodeHandler {
	Code code[3];
	std::uint32_t reg_base[3];
	constexpr OpCodeHandler_Mv_Gv(Code code16, Code code32, Code code64) noexcept
		: OpCodeHandler(&decode, true), code{code16, code32, code64},
		  reg_base{reg_u32(Register::AX), reg_u32(Register::EAX), reg_u32(Register::RAX)} {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Eb_REX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_Eb_REX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Gv_Ev_REX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Gv_Ev_REX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Ev_Gv_REX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_Ev_Gv_REX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_GvM_VX_Ib : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_GvM_VX_Ib(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_Wbinvd : OpCodeHandler {
	constexpr OpCodeHandler_Wbinvd() noexcept : OpCodeHandler(&decode, false) {}
	ICED_DECODE_FN_DECL;
};
} // namespace iced_x86::internal
