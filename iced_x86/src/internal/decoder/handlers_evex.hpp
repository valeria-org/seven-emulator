// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// EVEX op code handlers (Rust: decoder/handlers/evex.rs)

#pragma once

#include "iced_x86/code.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/tuple_type.hpp"
#include "internal/decoder/handlers.hpp"
#include "internal/vector_length.hpp"

#include <cstdint>

namespace iced_x86::internal {

struct OpCodeHandler_VectorLength_EVEX : OpCodeHandler {
	const OpCodeHandler* handlers[4];
	constexpr OpCodeHandler_VectorLength_EVEX(const OpCodeHandler* handler128, const OpCodeHandler* handler256, const OpCodeHandler* handler512) noexcept
		: OpCodeHandler(&decode, true), handlers{handler128, handler256, handler512, get_invalid_handler()} {
		static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L512) == 2, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::Unknown) == 3, "");
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler128));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler256));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler512));
		ICED_DEBUG_ASSERT(handlers[0]->has_modrm);
		ICED_DEBUG_ASSERT(handlers[1]->has_modrm);
		ICED_DEBUG_ASSERT(handlers[2]->has_modrm);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_VectorLength_EVEX_er : OpCodeHandler {
	const OpCodeHandler* handlers[4];
	constexpr OpCodeHandler_VectorLength_EVEX_er(const OpCodeHandler* handler128, const OpCodeHandler* handler256, const OpCodeHandler* handler512) noexcept
		: OpCodeHandler(&decode, true), handlers{handler128, handler256, handler512, get_invalid_handler()} {
		static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::L512) == 2, "");
		static_assert(static_cast<std::uint32_t>(VectorLength::Unknown) == 3, "");
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler128));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler256));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler512));
		ICED_DEBUG_ASSERT(handlers[0]->has_modrm);
		ICED_DEBUG_ASSERT(handlers[1]->has_modrm);
		ICED_DEBUG_ASSERT(handlers[2]->has_modrm);
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_V_H_Ev_er : OpCodeHandler {
	Register base_reg;
	Code code_w0;
	Code code_w1;
	TupleType tuple_type_w0;
	TupleType tuple_type_w1;
	constexpr OpCodeHandler_EVEX_V_H_Ev_er(Register base_reg_, Code code_w0_, Code code_w1_, TupleType tuple_type_w0_, TupleType tuple_type_w1_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code_w0(code_w0_), code_w1(code_w1_), tuple_type_w0(tuple_type_w0_),
		  tuple_type_w1(tuple_type_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_V_H_Ev_Ib : OpCodeHandler {
	Register base_reg;
	Code code_w0;
	Code code_w1;
	TupleType tuple_type_w0;
	TupleType tuple_type_w1;
	constexpr OpCodeHandler_EVEX_V_H_Ev_Ib(Register base_reg_, Code code_w0_, Code code_w1_, TupleType tuple_type_w0_, TupleType tuple_type_w1_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code_w0(code_w0_), code_w1(code_w1_), tuple_type_w0(tuple_type_w0_),
		  tuple_type_w1(tuple_type_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_Ed_V_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	TupleType tuple_type32;
	TupleType tuple_type64;
	constexpr OpCodeHandler_EVEX_Ed_V_Ib(Register base_reg_, Code code32_, Code code64_, TupleType tuple_type32_, TupleType tuple_type64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_), tuple_type32(tuple_type32_),
		  tuple_type64(tuple_type64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHW_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool only_sae;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_VkHW_er(Register base_reg_, Code code_, TupleType tuple_type_, bool only_sae_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), only_sae(only_sae_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHW_er_ur : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_VkHW_er_ur(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkW_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	bool only_sae;
	bool can_broadcast;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_VkW_er(Register base_reg_, Code code_, TupleType tuple_type_, bool only_sae_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), only_sae(only_sae_),
		  can_broadcast(true) {}
	// Rust: new1()
	constexpr OpCodeHandler_EVEX_VkW_er(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_, bool only_sae_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_), only_sae(only_sae_),
		  can_broadcast(true) {}
	// Rust: new2()
	constexpr OpCodeHandler_EVEX_VkW_er(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_, bool only_sae_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_), only_sae(only_sae_),
		  can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkWIb_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_VkWIb_er(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkW : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	bool can_broadcast;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_VkW(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), can_broadcast(can_broadcast_) {}
	// Rust: new1()
	constexpr OpCodeHandler_EVEX_VkW(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_),
		  can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_WkV : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	std::uint32_t disallow_zeroing_masking;
	Register base_reg1;
	Register base_reg2;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_WkV(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), disallow_zeroing_masking(0), base_reg1(base_reg_), base_reg2(base_reg_) {}
	// Rust: new1()
	constexpr OpCodeHandler_EVEX_WkV(Register base_reg_, Code code_, TupleType tuple_type_, bool allow_zeroing_masking) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), disallow_zeroing_masking(allow_zeroing_masking ? 0 : UINT32_MAX),
		  base_reg1(base_reg_), base_reg2(base_reg_) {}
	// Rust: new2()
	constexpr OpCodeHandler_EVEX_WkV(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), disallow_zeroing_masking(0), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkM : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_VkM(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_VkWIb(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_WkVIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_WkVIb(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_HkWIb : OpCodeHandler {
	bool can_broadcast;
	Code code;
	Register base_reg1;
	Register base_reg2;
	TupleType tuple_type;
	constexpr OpCodeHandler_EVEX_HkWIb(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), can_broadcast(can_broadcast_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), tuple_type(tuple_type_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_HWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_HWIb(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_WkVIb_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_WkVIb_er(Register base_reg1_, Register base_reg2_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VW_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_VW_er(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VW : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_VW(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_WV : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_WV(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VM : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_VM(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VK : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_EVEX_VK(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KR : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_EVEX_KR(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KkHWIb_sae : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_KkHWIb_sae(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHW : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	bool can_broadcast;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_VkHW(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), base_reg3(base_reg_),
		  can_broadcast(can_broadcast_) {}
	// Rust: new1()
	constexpr OpCodeHandler_EVEX_VkHW(Register base_reg1_, Register base_reg2_, Register base_reg3_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_), base_reg3(base_reg3_),
		  can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHM : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	constexpr OpCodeHandler_EVEX_VkHM(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	bool can_broadcast;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_VkHWIb(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), base_reg3(base_reg_),
		  can_broadcast(can_broadcast_) {}
	// Rust: new1()
	constexpr OpCodeHandler_EVEX_VkHWIb(Register base_reg1_, Register base_reg2_, Register base_reg3_, Code code_, TupleType tuple_type_,
										bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg1_), base_reg2(base_reg2_), base_reg3(base_reg3_),
		  can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkHWIb_er : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_VkHWIb_er(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg1(base_reg_), base_reg2(base_reg_), base_reg3(base_reg_),
		  can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KkHW : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_KkHW(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KP1HW : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_KP1HW(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KkHWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_KkHWIb(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_WkHV : OpCodeHandler {
	Register base_reg;
	Code code;
	constexpr OpCodeHandler_EVEX_WkHV(Register base_reg_, Code code_) noexcept : OpCodeHandler(&decode, true), base_reg(base_reg_), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VHWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_VHWIb(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VHW : OpCodeHandler {
	TupleType tuple_type;
	Code code_r;
	Code code_m;
	Register base_reg1;
	Register base_reg2;
	Register base_reg3;
	// Rust: new()
	constexpr OpCodeHandler_EVEX_VHW(Register base_reg_, Code code_r_, Code code_m_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code_r(code_r_), code_m(code_m_), base_reg1(base_reg_), base_reg2(base_reg_),
		  base_reg3(base_reg_) {}
	// Rust: new2()
	constexpr OpCodeHandler_EVEX_VHW(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code_r(code_), code_m(code_), base_reg1(base_reg_), base_reg2(base_reg_),
		  base_reg3(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VHM : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_VHM(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_Gv_W_er : OpCodeHandler {
	TupleType tuple_type;
	Code code_w0;
	Code code_w1;
	Register base_reg;
	bool only_sae;
	constexpr OpCodeHandler_EVEX_Gv_W_er(Register base_reg_, Code code_w0_, Code code_w1_, TupleType tuple_type_, bool only_sae_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code_w0(code_w0_), code_w1(code_w1_), base_reg(base_reg_), only_sae(only_sae_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VX_Ev : OpCodeHandler {
	Code code32;
	Code code64;
	TupleType tuple_type_w0;
	TupleType tuple_type_w1;
	constexpr OpCodeHandler_EVEX_VX_Ev(Code code32_, Code code64_, TupleType tuple_type_w0_, TupleType tuple_type_w1_) noexcept
		: OpCodeHandler(&decode, true), code32(code32_), code64(code64_), tuple_type_w0(tuple_type_w0_), tuple_type_w1(tuple_type_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_Ev_VX : OpCodeHandler {
	Code code32;
	Code code64;
	TupleType tuple_type_w0;
	TupleType tuple_type_w1;
	constexpr OpCodeHandler_EVEX_Ev_VX(Code code32_, Code code64_, TupleType tuple_type_w0_, TupleType tuple_type_w1_) noexcept
		: OpCodeHandler(&decode, true), code32(code32_), code64(code64_), tuple_type_w0(tuple_type_w0_), tuple_type_w1(tuple_type_w1_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_Ev_VX_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_EVEX_Ev_VX_Ib(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_MV : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_MV(Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VkEv_REXW : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	constexpr OpCodeHandler_EVEX_VkEv_REXW(Register base_reg_, Code code32_, Code code64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_Vk_VSIB : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	Register vsib_base;
	constexpr OpCodeHandler_EVEX_Vk_VSIB(Register base_reg_, Register vsib_base_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), vsib_base(vsib_base_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VSIB_k1_VX : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register vsib_index;
	Register base_reg;
	constexpr OpCodeHandler_EVEX_VSIB_k1_VX(Register vsib_index_, Register base_reg_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), vsib_index(vsib_index_), base_reg(base_reg_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_VSIB_k1 : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register vsib_index;
	constexpr OpCodeHandler_EVEX_VSIB_k1(Register vsib_index_, Code code_, TupleType tuple_type_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), vsib_index(vsib_index_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_GvM_VX_Ib : OpCodeHandler {
	Register base_reg;
	Code code32;
	Code code64;
	TupleType tuple_type32;
	TupleType tuple_type64;
	constexpr OpCodeHandler_EVEX_GvM_VX_Ib(Register base_reg_, Code code32_, Code code64_, TupleType tuple_type32_, TupleType tuple_type64_) noexcept
		: OpCodeHandler(&decode, true), base_reg(base_reg_), code32(code32_), code64(code64_), tuple_type32(tuple_type32_),
		  tuple_type64(tuple_type64_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_EVEX_KkWIb : OpCodeHandler {
	TupleType tuple_type;
	Code code;
	Register base_reg;
	bool can_broadcast;
	constexpr OpCodeHandler_EVEX_KkWIb(Register base_reg_, Code code_, TupleType tuple_type_, bool can_broadcast_) noexcept
		: OpCodeHandler(&decode, true), tuple_type(tuple_type_), code(code_), base_reg(base_reg_), can_broadcast(can_broadcast_) {}
	ICED_DECODE_FN_DECL;
};

} // namespace iced_x86::internal
