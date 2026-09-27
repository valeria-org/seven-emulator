// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// VEX/XOP op code handlers (Rust: decoder/handlers/vex.rs)

#pragma once

#include "internal/decoder/handlers.hpp"

namespace iced_x86::internal {

struct OpCodeHandler_VectorLength_VEX : OpCodeHandler {
	const OpCodeHandler* handlers[4];
	constexpr OpCodeHandler_VectorLength_VEX(bool has_modrm_, const OpCodeHandler* handler128, const OpCodeHandler* handler256) noexcept
		: OpCodeHandler(&decode, has_modrm_), handlers{handler128, handler256, get_invalid_handler(), get_invalid_handler()} {
		static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L512) == 2, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::Unknown) == 3, "");
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler128));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler256));
		ICED_DEBUG_ASSERT(handlers[0]->has_modrm == has_modrm_);
		ICED_DEBUG_ASSERT(handlers[1]->has_modrm == has_modrm_);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Simple : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_Simple(Code code_) noexcept : OpCodeHandler(&decode, false), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHEv : OpCodeHandler {
	Register base_reg;
	Code code_w0;
	Code code_w1;
	constexpr OpCodeHandler_VEX_VHEv(Register base_reg_, Code code_w0_, Code code_w1_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHEvIb : OpCodeHandler {
	Register base_reg;
	Code code_w0;
	Code code_w1;
	constexpr OpCodeHandler_VEX_VHEvIb(Register base_reg_, Code code_w0_, Code code_w1_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VW : OpCodeHandler {
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_VEX_VW(Register base_reg1_, Register base_reg2_, Code code_) noexcept
		: OpCodeHandler(&decode, true), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VX_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_VX_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Ev_VX : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Ev_VX(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_WV : OpCodeHandler {
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_VEX_WV(Register reg, Code code_) noexcept : OpCodeHandler(&decode, true), code(code_), base_reg1(reg), base_reg2(reg) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VM : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VM(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_MV : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_MV(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_M : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_M(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_RdRq : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_RdRq(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_rDI_VX_RX : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_rDI_VX_RX(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VWIb : OpCodeHandler {
	Code code_w0;
	Code code_w1;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_VEX_VWIb(Register base_reg1_, Register base_reg2_, Code code_w0_, Code code_w1_) noexcept
		: OpCodeHandler(&decode, true), code_w0(code_w0_), code_w1(code_w1_), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_WVIb : OpCodeHandler {
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_VEX_WVIb(Register base_reg1_, Register base_reg2_, Code code_) noexcept
		: OpCodeHandler(&decode, true), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Ed_V_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Ed_V_Ib(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHW : OpCodeHandler {
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	Code code_r;
	Code code_m;
	constexpr OpCodeHandler_VEX_VHW(Register base_reg1_, Register base_reg2_, Register base_reg3_, Code code_r_, Code code_m_) noexcept
		: OpCodeHandler(&decode, true), base_reg1(base_reg1_), base_reg2(base_reg2_), base_reg3(base_reg3_), code_r(code_r_), code_m(code_m_) {}
	// Rust: new1()
	constexpr OpCodeHandler_VEX_VHW(Register base_reg1_, Register base_reg2_, Register base_reg3_, Code code) noexcept
		: OpCodeHandler(&decode, true), base_reg1(base_reg1_), base_reg2(base_reg2_), base_reg3(base_reg3_), code_r(code), code_m(code) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VWH : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VWH(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_WHV : OpCodeHandler {
	Register base_reg;
	Code code_r;
	constexpr OpCodeHandler_VEX_WHV(Register base_reg_, Code code) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code_r(code) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHM : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VHM(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_MHV : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_MHV(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHWIb : OpCodeHandler {
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	Code code;
	constexpr OpCodeHandler_VEX_VHWIb(Register base_reg1_, Register base_reg2_, Register base_reg3_, Code code_) noexcept
		: OpCodeHandler(&decode, true), base_reg1(base_reg1_), base_reg2(base_reg2_), base_reg3(base_reg3_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_HRIb : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_HRIb(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHWIs4 : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VHWIs4(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHIs4W : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VHIs4W(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHWIs5 : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VHWIs5(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VHIs5W : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_VEX_VHIs5W(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_HK_RK : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VK_HK_RK(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_RK : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VK_RK(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_RK_Ib : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VK_RK_Ib(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_WK : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VK_WK(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_M_VK : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_M_VK(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_R : OpCodeHandler {
	Register gpr;
	Code code;
	constexpr OpCodeHandler_VEX_VK_R(Code code_, Register gpr_) noexcept : OpCodeHandler(&decode, true), gpr(gpr_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_G_VK : OpCodeHandler {
	Register gpr;
	Code code;
	constexpr OpCodeHandler_VEX_G_VK(Code code_, Register gpr_) noexcept : OpCodeHandler(&decode, true), gpr(gpr_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_W : OpCodeHandler {
	Register base_reg;
	Code code_w0;
	Code code_w1;
	constexpr OpCodeHandler_VEX_Gv_W(Register base_reg_, Code code_w0_, Code code_w1_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code_w0(code_w0_), code_w1(code_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_RX : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_RX(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_GPR_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_GPR_Ib(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VX_VSIB_HX : OpCodeHandler {
	Register base_reg1;
	Register vsib_index;
	Register base_reg3;
	Code code;
	constexpr OpCodeHandler_VEX_VX_VSIB_HX(Register base_reg1_, Register vsib_index_, Register base_reg3_, Code code_) noexcept
		: OpCodeHandler(&decode, true), base_reg1(base_reg1_), vsib_index(vsib_index_), base_reg3(base_reg3_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_Gv_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_Gv_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_Ev_Gv : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_Ev_Gv(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Ev_Gv_Gv : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Ev_Gv_Gv(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Hv_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Hv_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Hv_Ed_Id : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Hv_Ed_Id(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_GvM_VX_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_GvM_VX_Ib(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_Ev_Ib : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_Ev_Ib(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_Ev_Id : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_Ev_Id(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VT_SIBMEM : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VT_SIBMEM(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_SIBMEM_VT : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_SIBMEM_VT(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VT : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VT(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VT_RT_HT : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_VT_RT_HT(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gq_HK_RK : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_Gq_HK_RK(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_VK_R_Ib : OpCodeHandler {
	Register gpr;
	Code code;
	constexpr OpCodeHandler_VEX_VK_R_Ib(Code code_, Register gpr_) noexcept : OpCodeHandler(&decode, true), gpr(gpr_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_K_Jb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_K_Jb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_K_Jz : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_VEX_K_Jz(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Gv_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Gv_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VEX_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	constexpr OpCodeHandler_VEX_Ev(Code code32_, Code code64_) noexcept : OpCodeHandler(&decode, true), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

} // namespace iced_x86::internal
