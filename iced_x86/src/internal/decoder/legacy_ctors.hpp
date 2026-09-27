// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Legacy handler factory fns (Rust: decoder/table_de/legacy_reader.rs)
//
// One `constexpr` fn per handler kind (the generator's `LegacyOpCodeHandlerKind`), same name as the kind. They're only
// called by the generated tables (src/decoder/data_legacy.cpp, see `CppDecoderTableWriter`) to create the `constexpr`
// handlers. The args are the args of the generator's handler definition (same order, all `Code` values are passed):
// enum values, bools, ints, pointers to other handlers and braced lists of handler pointers (too many elements don't
// compile, a missing element is null which fails the ctor's constant evaluation).

#pragma once

#include "internal/decoder/handlers.hpp"
#include "internal/decoder/handlers_d3now.hpp"
#include "internal/decoder/handlers_fpu.hpp"
#include "internal/decoder/handlers_legacy.hpp"

#include <cstddef>
#include <cstdint>

namespace iced_x86::internal::legacy_ctors {

constexpr OpCodeHandler_Bitness Bitness(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_Bitness{handler1, handler2};
}
constexpr OpCodeHandler_Bitness_DontReadModRM Bitness_DontReadModRM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_Bitness_DontReadModRM{handler1, handler2};
}
constexpr OpCodeHandler_RM RM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_RM{handler1, handler2}; }
constexpr OpCodeHandler_Options1632 Options1632_1(const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options) noexcept {
	return OpCodeHandler_Options1632{handler1, handler2, options};
}
constexpr OpCodeHandler_Options1632 Options1632_2(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options1, const OpCodeHandler* handler3,
		std::uint32_t options2) noexcept {
	return OpCodeHandler_Options1632{handler1, handler2, options1, handler3, options2};
}
constexpr OpCodeHandler_Options Options3(const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options) noexcept {
	return OpCodeHandler_Options{handler1, handler2, options};
}
constexpr OpCodeHandler_Options Options5(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options1, const OpCodeHandler* handler3,
		std::uint32_t options2) noexcept {
	return OpCodeHandler_Options{handler1, handler2, options1, handler3, options2};
}
constexpr OpCodeHandler_Options_DontReadModRM Options_DontReadModRM(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options) noexcept {
	return OpCodeHandler_Options_DontReadModRM{handler1, handler2, options};
}
constexpr OpCodeHandler_AnotherTable AnotherTable(const OpCodeHandler* const (&handlers)[0x100]) noexcept {
	return OpCodeHandler_AnotherTable{handlers};
}
constexpr OpCodeHandler_Group Group(const OpCodeHandler* const (&handlers)[8]) noexcept { return OpCodeHandler_Group{handlers}; }
constexpr OpCodeHandler_Group8x64 Group8x64(const OpCodeHandler* const (&handlers1)[8], const OpCodeHandler* const (&handlers2)[0x40]) noexcept {
	return OpCodeHandler_Group8x64{handlers1, handlers2};
}
constexpr OpCodeHandler_Group8x8 Group8x8(const OpCodeHandler* const (&handlers1)[8], const OpCodeHandler* const (&handlers2)[8]) noexcept {
	return OpCodeHandler_Group8x8{handlers1, handlers2};
}
constexpr OpCodeHandler_MandatoryPrefix MandatoryPrefix(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix{true, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_MandatoryPrefix4 MandatoryPrefix4(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4,
		std::uint32_t value) noexcept {
	return OpCodeHandler_MandatoryPrefix4{handler1, handler2, handler3, handler4, value};
}
constexpr OpCodeHandler_MandatoryPrefix MandatoryPrefix_NoModRM(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix{false, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_MandatoryPrefix3 MandatoryPrefix3(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4,
		const OpCodeHandler* handler5, const OpCodeHandler* handler6, const OpCodeHandler* handler7, const OpCodeHandler* handler8,
		std::uint32_t flags) noexcept {
	return OpCodeHandler_MandatoryPrefix3{handler1, handler2, handler3, handler4, handler5, handler6, handler7, handler8, flags};
}
constexpr OpCodeHandler_D3NOW D3NOW() noexcept { return OpCodeHandler_D3NOW{}; }
constexpr OpCodeHandler_EVEX EVEX(const OpCodeHandler* handler) noexcept { return OpCodeHandler_EVEX{handler}; }
constexpr OpCodeHandler_VEX2 VEX2(const OpCodeHandler* handler) noexcept { return OpCodeHandler_VEX2{handler}; }
constexpr OpCodeHandler_VEX3 VEX3(const OpCodeHandler* handler) noexcept { return OpCodeHandler_VEX3{handler}; }
constexpr OpCodeHandler_XOP XOP(const OpCodeHandler* handler) noexcept { return OpCodeHandler_XOP{handler}; }
constexpr OpCodeHandler_AL_DX AL_DX(Code code) noexcept { return OpCodeHandler_AL_DX{code}; }
constexpr OpCodeHandler_Ap Ap(Code code1, Code code2) noexcept { return OpCodeHandler_Ap{code1, code2}; }
constexpr OpCodeHandler_B_BM B_BM(Code code1, Code code2) noexcept { return OpCodeHandler_B_BM{code1, code2}; }
constexpr OpCodeHandler_B_Ev B_Ev(Code code1, Code code2, bool b_) noexcept { return OpCodeHandler_B_Ev{code1, code2, b_}; }
constexpr OpCodeHandler_B_MIB B_MIB(Code code) noexcept { return OpCodeHandler_B_MIB{code}; }
constexpr OpCodeHandler_BM_B BM_B(Code code1, Code code2) noexcept { return OpCodeHandler_BM_B{code1, code2}; }
constexpr OpCodeHandler_BranchIw BranchIw(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_BranchIw{code1, code2, code3}; }
constexpr OpCodeHandler_BranchSimple BranchSimple(Code code1, Code code2, Code code3) noexcept {
	return OpCodeHandler_BranchSimple{code1, code2, code3};
}
constexpr OpCodeHandler_C_R C_R_3a(Code code1, Code code2, Register reg) noexcept { return OpCodeHandler_C_R{code1, code2, reg}; }
constexpr OpCodeHandler_C_R C_R_3b(Code code, Register reg) noexcept { return OpCodeHandler_C_R{code, Code::INVALID, reg}; }
constexpr OpCodeHandler_DX_AL DX_AL(Code code) noexcept { return OpCodeHandler_DX_AL{code}; }
constexpr OpCodeHandler_DX_eAX DX_eAX(Code code1, Code code2) noexcept { return OpCodeHandler_DX_eAX{code1, code2}; }
constexpr OpCodeHandler_eAX_DX eAX_DX(Code code1, Code code2) noexcept { return OpCodeHandler_eAX_DX{code1, code2}; }
constexpr OpCodeHandler_Eb Eb_1(Code code) noexcept { return OpCodeHandler_Eb{code, 0}; }
constexpr OpCodeHandler_Eb Eb_2(Code code, std::uint32_t flags) noexcept { return OpCodeHandler_Eb{code, flags}; }
constexpr OpCodeHandler_Eb_CL Eb_CL(Code code) noexcept { return OpCodeHandler_Eb_CL{code}; }
constexpr OpCodeHandler_Eb_Gb Eb_Gb_1(Code code) noexcept { return OpCodeHandler_Eb_Gb{code, 0}; }
constexpr OpCodeHandler_Eb_Gb Eb_Gb_2(Code code, std::uint32_t flags) noexcept { return OpCodeHandler_Eb_Gb{code, flags}; }
constexpr OpCodeHandler_Eb_Ib Eb_Ib_1(Code code) noexcept { return OpCodeHandler_Eb_Ib{code, 0}; }
constexpr OpCodeHandler_Eb_Ib Eb_Ib_2(Code code, std::uint32_t flags) noexcept { return OpCodeHandler_Eb_Ib{code, flags}; }
constexpr OpCodeHandler_Eb_1 Eb1(Code code) noexcept { return OpCodeHandler_Eb_1{code}; }
constexpr OpCodeHandler_Ed_V_Ib Ed_V_Ib(Code code1, Code code2) noexcept { return OpCodeHandler_Ed_V_Ib{code1, code2}; }
constexpr OpCodeHandler_Ep Ep(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ep{code1, code2, code3}; }
constexpr OpCodeHandler_Ev Ev_3a(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev{code1, code2, code3, 0}; }
constexpr OpCodeHandler_Ev Ev_3b(Code code1, Code code2) noexcept { return OpCodeHandler_Ev{code1, code2, Code::INVALID, 0}; }
constexpr OpCodeHandler_Ev Ev_4(Code code1, Code code2, Code code3, std::uint32_t flags) noexcept {
	return OpCodeHandler_Ev{code1, code2, code3, flags};
}
constexpr OpCodeHandler_Ev_CL Ev_CL(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_CL{code1, code2, code3}; }
constexpr OpCodeHandler_Ev_Gv_32_64 Ev_Gv_32_64(Code code1, Code code2) noexcept { return OpCodeHandler_Ev_Gv_32_64{code1, code2}; }
constexpr OpCodeHandler_Ev_Gv Ev_Gv_3a(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Gv{code1, code2, code3}; }
constexpr OpCodeHandler_Ev_Gv Ev_Gv_3b(Code code1, Code code2) noexcept { return OpCodeHandler_Ev_Gv{code1, code2, Code::INVALID}; }
constexpr OpCodeHandler_Ev_Gv_flags Ev_Gv_4(Code code1, Code code2, Code code3, std::uint32_t flags) noexcept {
	return OpCodeHandler_Ev_Gv_flags{code1, code2, code3, flags};
}
constexpr OpCodeHandler_Ev_Gv_CL Ev_Gv_CL(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Gv_CL{code1, code2, code3}; }
constexpr OpCodeHandler_Ev_Gv_Ib Ev_Gv_Ib(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Gv_Ib{code1, code2, code3}; }
constexpr OpCodeHandler_Ev_Gv_REX Ev_Gv_REX(Code code1, Code code2) noexcept { return OpCodeHandler_Ev_Gv_REX{code1, code2}; }
constexpr OpCodeHandler_Ev_Ib Ev_Ib_3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Ib{code1, code2, code3, 0}; }
constexpr OpCodeHandler_Ev_Ib Ev_Ib_4(Code code1, Code code2, Code code3, std::uint32_t flags) noexcept {
	return OpCodeHandler_Ev_Ib{code1, code2, code3, flags};
}
constexpr OpCodeHandler_Ev_Ib2 Ev_Ib2_3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Ib2{code1, code2, code3, 0}; }
constexpr OpCodeHandler_Ev_Ib2 Ev_Ib2_4(Code code1, Code code2, Code code3, std::uint32_t flags) noexcept {
	return OpCodeHandler_Ev_Ib2{code1, code2, code3, flags};
}
constexpr OpCodeHandler_Ev_Iz Ev_Iz_3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Iz{code1, code2, code3, 0}; }
constexpr OpCodeHandler_Ev_Iz Ev_Iz_4(Code code1, Code code2, Code code3, std::uint32_t flags) noexcept {
	return OpCodeHandler_Ev_Iz{code1, code2, code3, flags};
}
constexpr OpCodeHandler_Ev_P Ev_P(Code code1, Code code2) noexcept { return OpCodeHandler_Ev_P{code1, code2}; }
constexpr OpCodeHandler_Ev_REXW Ev_REXW_1a(Code code, std::uint32_t value) noexcept { return OpCodeHandler_Ev_REXW{code, Code::INVALID, value}; }
constexpr OpCodeHandler_Ev_REXW Ev_REXW(Code code1, Code code2, std::uint32_t value) noexcept { return OpCodeHandler_Ev_REXW{code1, code2, value}; }
constexpr OpCodeHandler_Ev_Sw Ev_Sw(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_Sw{code1, code2, code3}; }
constexpr OpCodeHandler_Ev_VX Ev_VX(Code code1, Code code2) noexcept { return OpCodeHandler_Ev_VX{code1, code2}; }
constexpr OpCodeHandler_Ev_1 Ev1(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ev_1{code1, code2, code3}; }
constexpr OpCodeHandler_Evj Evj(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Evj{code1, code2, code3}; }
constexpr OpCodeHandler_Evw Evw(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Evw{code1, code2, code3}; }
constexpr OpCodeHandler_Ew Ew(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ew{code1, code2, code3}; }
constexpr OpCodeHandler_Gb_Eb Gb_Eb(Code code) noexcept { return OpCodeHandler_Gb_Eb{code}; }
constexpr OpCodeHandler_Gdq_Ev Gdq_Ev(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gdq_Ev{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Eb Gv_Eb(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Eb{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Eb_REX Gv_Eb_REX(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Eb_REX{code1, code2}; }
constexpr OpCodeHandler_Gv_Ev_32_64 Gv_Ev_32_64(Code code1, Code code2, bool b1, bool b2) noexcept {
	return OpCodeHandler_Gv_Ev_32_64{code1, code2, b1, b2};
}
constexpr OpCodeHandler_Gv_Ev Gv_Ev_3a(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ev{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ev Gv_Ev_3b(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Ev{code1, code2, Code::INVALID}; }
constexpr OpCodeHandler_Gv_Ev_Ib Gv_Ev_Ib(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ev_Ib{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ev_Ib_REX Gv_Ev_Ib_REX(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Ev_Ib_REX{code1, code2}; }
constexpr OpCodeHandler_Gv_Ev_Iz Gv_Ev_Iz(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ev_Iz{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ev_REX Gv_Ev_REX(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Ev_REX{code1, code2}; }
constexpr OpCodeHandler_Gv_Ev2 Gv_Ev2(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ev2{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ev3 Gv_Ev3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ev3{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ew Gv_Ew(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Ew{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_M Gv_M(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_M{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_M_as Gv_M_as(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_M_as{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Ma Gv_Ma(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Ma{code1, code2}; }
constexpr OpCodeHandler_Gv_Mp Gv_Mp_2(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_Mp{code1, code2, Code::INVALID}; }
constexpr OpCodeHandler_Gv_Mp Gv_Mp_3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Mp{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_Mv Gv_Mv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Gv_Mv{code1, code2, code3}; }
constexpr OpCodeHandler_Gv_N Gv_N(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_N{code1, code2}; }
constexpr OpCodeHandler_Gv_N_Ib_REX Gv_N_Ib_REX(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_N_Ib_REX{code1, code2}; }
constexpr OpCodeHandler_Gv_RX Gv_RX(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_RX{code1, code2}; }
constexpr OpCodeHandler_Gv_W Gv_W(Code code1, Code code2) noexcept { return OpCodeHandler_Gv_W{code1, code2}; }
constexpr OpCodeHandler_GvM_VX_Ib GvM_VX_Ib(Code code1, Code code2) noexcept { return OpCodeHandler_GvM_VX_Ib{code1, code2}; }
constexpr OpCodeHandler_Ib Ib(Code code) noexcept { return OpCodeHandler_Ib{code}; }
constexpr OpCodeHandler_Ib3 Ib3(Code code) noexcept { return OpCodeHandler_Ib3{code}; }
constexpr OpCodeHandler_IbReg IbReg(Code code, Register reg) noexcept { return OpCodeHandler_IbReg{code, reg}; }
constexpr OpCodeHandler_IbReg2 IbReg2(Code code1, Code code2) noexcept { return OpCodeHandler_IbReg2{code1, code2}; }
constexpr OpCodeHandler_Iw_Ib Iw_Ib(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Iw_Ib{code1, code2, code3}; }
constexpr OpCodeHandler_Jb Jb(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Jb{code1, code2, code3}; }
constexpr OpCodeHandler_Jb2 Jb2(Code code1, Code code2, Code code3, Code code4, Code code5, Code code6, Code code7) noexcept {
	return OpCodeHandler_Jb2{code1, code2, code3, code4, code5, code6, code7};
}
constexpr OpCodeHandler_Jdisp Jdisp(Code code1, Code code2) noexcept { return OpCodeHandler_Jdisp{code1, code2}; }
constexpr OpCodeHandler_Jx Jx(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Jx{code1, code2, code3}; }
constexpr OpCodeHandler_Jz Jz(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Jz{code1, code2, code3}; }
constexpr OpCodeHandler_M M_1(Code code) noexcept { return OpCodeHandler_M{code}; }
constexpr OpCodeHandler_M M_2(Code code1, Code code2) noexcept { return OpCodeHandler_M{code1, code2}; }
constexpr OpCodeHandler_M_REXW M_REXW_2(Code code1, Code code2) noexcept { return OpCodeHandler_M_REXW{code1, code2, 0, 0}; }
constexpr OpCodeHandler_M_REXW M_REXW_4(Code code1, Code code2, std::uint32_t flags1, std::uint32_t flags2) noexcept {
	return OpCodeHandler_M_REXW{code1, code2, flags1, flags2};
}
constexpr OpCodeHandler_MemBx MemBx(Code code) noexcept { return OpCodeHandler_MemBx{code}; }
constexpr OpCodeHandler_Mf Mf_1(Code code) noexcept { return OpCodeHandler_Mf{code}; }
constexpr OpCodeHandler_Mf Mf_2a(Code code1, Code code2) noexcept { return OpCodeHandler_Mf{code1, code2}; }
constexpr OpCodeHandler_Mf Mf_2b(Code code1, Code code2) noexcept { return OpCodeHandler_Mf{code1, code2}; }
constexpr OpCodeHandler_MIB_B MIB_B(Code code) noexcept { return OpCodeHandler_MIB_B{code}; }
constexpr OpCodeHandler_MP MP(Code code) noexcept { return OpCodeHandler_MP{code}; }
constexpr OpCodeHandler_Ms Ms(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ms{code1, code2, code3}; }
constexpr OpCodeHandler_MV MV(Code code) noexcept { return OpCodeHandler_MV{code}; }
constexpr OpCodeHandler_Mv_Gv Mv_Gv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Mv_Gv{code1, code2, code3}; }
constexpr OpCodeHandler_Mv_Gv_REXW Mv_Gv_REXW(Code code1, Code code2) noexcept { return OpCodeHandler_Mv_Gv_REXW{code1, code2}; }
constexpr OpCodeHandler_NIb NIb(Code code) noexcept { return OpCodeHandler_NIb{code}; }
constexpr OpCodeHandler_Ob_Reg Ob_Reg(Code code, Register reg) noexcept { return OpCodeHandler_Ob_Reg{code, reg}; }
constexpr OpCodeHandler_Ov_Reg Ov_Reg(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Ov_Reg{code1, code2, code3}; }
constexpr OpCodeHandler_P_Ev P_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_P_Ev{code1, code2}; }
constexpr OpCodeHandler_P_Ev_Ib P_Ev_Ib(Code code1, Code code2) noexcept { return OpCodeHandler_P_Ev_Ib{code1, code2}; }
constexpr OpCodeHandler_P_Q P_Q(Code code) noexcept { return OpCodeHandler_P_Q{code}; }
constexpr OpCodeHandler_P_Q_Ib P_Q_Ib(Code code) noexcept { return OpCodeHandler_P_Q_Ib{code}; }
constexpr OpCodeHandler_P_R P_R(Code code) noexcept { return OpCodeHandler_P_R{code}; }
constexpr OpCodeHandler_P_W P_W(Code code) noexcept { return OpCodeHandler_P_W{code}; }
constexpr OpCodeHandler_PushEv PushEv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_PushEv{code1, code2, code3}; }
constexpr OpCodeHandler_PushIb2 PushIb2(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_PushIb2{code1, code2, code3}; }
constexpr OpCodeHandler_PushIz PushIz(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_PushIz{code1, code2, code3}; }
constexpr OpCodeHandler_PushOpSizeReg PushOpSizeReg_4a(Code code1, Code code2, Code code3, Register reg) noexcept {
	return OpCodeHandler_PushOpSizeReg{code1, code2, code3, reg};
}
constexpr OpCodeHandler_PushOpSizeReg PushOpSizeReg_4b(Code code1, Code code2, Register reg) noexcept {
	return OpCodeHandler_PushOpSizeReg{code1, code2, Code::INVALID, reg};
}
constexpr OpCodeHandler_PushSimple2 PushSimple2(Code code1, Code code2, Code code3) noexcept {
	return OpCodeHandler_PushSimple2{code1, code2, code3};
}
constexpr OpCodeHandler_PushSimpleReg PushSimpleReg(std::uint32_t value, Code code1, Code code2, Code code3) noexcept {
	return OpCodeHandler_PushSimpleReg{value, code1, code2, code3};
}
constexpr OpCodeHandler_Q_P Q_P(Code code) noexcept { return OpCodeHandler_Q_P{code}; }
constexpr OpCodeHandler_R_C R_C_3a(Code code1, Code code2, Register reg) noexcept { return OpCodeHandler_R_C{code1, code2, reg}; }
constexpr OpCodeHandler_R_C R_C_3b(Code code, Register reg) noexcept { return OpCodeHandler_R_C{code, Code::INVALID, reg}; }
constexpr OpCodeHandler_rDI_P_N rDI_P_N(Code code) noexcept { return OpCodeHandler_rDI_P_N{code}; }
constexpr OpCodeHandler_rDI_VX_RX rDI_VX_RX(Code code) noexcept { return OpCodeHandler_rDI_VX_RX{code}; }
constexpr OpCodeHandler_Reg Reg(Code code, Register reg) noexcept { return OpCodeHandler_Reg{code, reg}; }
constexpr OpCodeHandler_Reg_Ib2 Reg_Ib2(Code code1, Code code2) noexcept { return OpCodeHandler_Reg_Ib2{code1, code2}; }
constexpr OpCodeHandler_Reg_Iz Reg_Iz(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Reg_Iz{code1, code2, code3}; }
constexpr OpCodeHandler_Reg_Ob Reg_Ob(Code code, Register reg) noexcept { return OpCodeHandler_Reg_Ob{code, reg}; }
constexpr OpCodeHandler_Reg_Ov Reg_Ov(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Reg_Ov{code1, code2, code3}; }
constexpr OpCodeHandler_Reg_Xb Reg_Xb(Code code, Register reg) noexcept { return OpCodeHandler_Reg_Xb{code, reg}; }
constexpr OpCodeHandler_Reg_Xv Reg_Xv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Reg_Xv{code1, code2, code3}; }
constexpr OpCodeHandler_Reg_Xv2 Reg_Xv2(Code code1, Code code2) noexcept { return OpCodeHandler_Reg_Xv2{code1, code2}; }
constexpr OpCodeHandler_Reg_Yb Reg_Yb(Code code, Register reg) noexcept { return OpCodeHandler_Reg_Yb{code, reg}; }
constexpr OpCodeHandler_Reg_Yv Reg_Yv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Reg_Yv{code1, code2, code3}; }
constexpr OpCodeHandler_RegIb RegIb(Code code, Register reg) noexcept { return OpCodeHandler_RegIb{code, reg}; }
constexpr OpCodeHandler_RegIb3 RegIb3(std::uint32_t value) noexcept { return OpCodeHandler_RegIb3{value}; }
constexpr OpCodeHandler_RegIz2 RegIz2(std::uint32_t value) noexcept { return OpCodeHandler_RegIz2{value}; }
constexpr OpCodeHandler_Reservednop Reservednop(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_Reservednop{handler1, handler2};
}
constexpr OpCodeHandler_RIb RIb(Code code) noexcept { return OpCodeHandler_RIb{code}; }
constexpr OpCodeHandler_RIbIb RIbIb(Code code) noexcept { return OpCodeHandler_RIbIb{code}; }
constexpr OpCodeHandler_Rv Rv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Rv{code1, code2, code3}; }
constexpr OpCodeHandler_Rv_32_64 Rv_32_64(Code code1, Code code2) noexcept { return OpCodeHandler_Rv_32_64{code1, code2}; }
constexpr OpCodeHandler_RvMw_Gw RvMw_Gw(Code code1, Code code2) noexcept { return OpCodeHandler_RvMw_Gw{code1, code2}; }
constexpr OpCodeHandler_Simple Simple(Code code) noexcept {
	return code == Code::Int3 ? OpCodeHandler_Simple::int3() : OpCodeHandler_Simple{false, code};
}
constexpr OpCodeHandler_Simple Simple_ModRM(Code code) noexcept { return OpCodeHandler_Simple{true, code}; }
constexpr OpCodeHandler_Simple2 Simple2_3a(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Simple2{code1, code2, code3}; }
constexpr OpCodeHandler_Simple2 Simple2_3b(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Simple2{code1, code2, code3}; }
constexpr OpCodeHandler_Simple2Iw Simple2Iw(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Simple2Iw{code1, code2, code3}; }
constexpr OpCodeHandler_Simple3 Simple3(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Simple3{code1, code2, code3}; }
constexpr OpCodeHandler_Simple4 Simple4(Code code1, Code code2) noexcept { return OpCodeHandler_Simple4{code1, code2}; }
constexpr OpCodeHandler_Simple4 Simple4b(Code code1, Code code2) noexcept { return OpCodeHandler_Simple4{code1, code2}; }
constexpr OpCodeHandler_Simple5 Simple5(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Simple5{code1, code2, code3}; }
constexpr OpCodeHandler_Simple5_a32 Simple5_a32(Code code1, Code code2, Code code3) noexcept {
	return OpCodeHandler_Simple5_a32{code1, code2, code3};
}
constexpr OpCodeHandler_Simple5_ModRM_as Simple5_ModRM_as(Code code1, Code code2, Code code3) noexcept {
	return OpCodeHandler_Simple5_ModRM_as{code1, code2, code3};
}
constexpr OpCodeHandler_SimpleReg SimpleReg(Code code, std::uint32_t value) noexcept { return OpCodeHandler_SimpleReg{code, value}; }
constexpr OpCodeHandler_ST_STi ST_STi(Code code) noexcept { return OpCodeHandler_ST_STi{code}; }
constexpr OpCodeHandler_STi STi(Code code) noexcept { return OpCodeHandler_STi{code}; }
constexpr OpCodeHandler_STi_ST STi_ST(Code code) noexcept { return OpCodeHandler_STi_ST{code}; }
constexpr OpCodeHandler_Sw_Ev Sw_Ev(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Sw_Ev{code1, code2, code3}; }
constexpr OpCodeHandler_V_Ev V_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_V_Ev{code1, code2}; }
constexpr OpCodeHandler_VM VM(Code code) noexcept { return OpCodeHandler_VM{code}; }
constexpr OpCodeHandler_VN VN(Code code) noexcept { return OpCodeHandler_VN{code}; }
constexpr OpCodeHandler_VQ VQ(Code code) noexcept { return OpCodeHandler_VQ{code}; }
constexpr OpCodeHandler_VRIbIb VRIbIb(Code code) noexcept { return OpCodeHandler_VRIbIb{code}; }
constexpr OpCodeHandler_VW VW_2(Code code) noexcept { return OpCodeHandler_VW{code}; }
constexpr OpCodeHandler_VW VW_3(Code code1, Code code2) noexcept { return OpCodeHandler_VW{code1, code2}; }
constexpr OpCodeHandler_VWIb VWIb_2(Code code) noexcept { return OpCodeHandler_VWIb{code}; }
constexpr OpCodeHandler_VWIb VWIb_3(Code code1, Code code2) noexcept { return OpCodeHandler_VWIb{code1, code2}; }
constexpr OpCodeHandler_VX_E_Ib VX_E_Ib(Code code1, Code code2) noexcept { return OpCodeHandler_VX_E_Ib{code1, code2}; }
constexpr OpCodeHandler_VX_Ev VX_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VX_Ev{code1, code2}; }
constexpr OpCodeHandler_Wbinvd Wbinvd() noexcept { return OpCodeHandler_Wbinvd{}; }
constexpr OpCodeHandler_WV WV(Code code) noexcept { return OpCodeHandler_WV{code}; }
constexpr OpCodeHandler_Xb_Yb Xb_Yb(Code code) noexcept { return OpCodeHandler_Xb_Yb{code}; }
constexpr OpCodeHandler_Xchg_Reg_rAX Xchg_Reg_rAX(std::uint32_t value) noexcept { return OpCodeHandler_Xchg_Reg_rAX{value}; }
constexpr OpCodeHandler_Xv_Yv Xv_Yv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Xv_Yv{code1, code2, code3}; }
constexpr OpCodeHandler_Yb_Reg Yb_Reg(Code code, Register reg) noexcept { return OpCodeHandler_Yb_Reg{code, reg}; }
constexpr OpCodeHandler_Yb_Xb Yb_Xb(Code code) noexcept { return OpCodeHandler_Yb_Xb{code}; }
constexpr OpCodeHandler_Yv_Reg Yv_Reg(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Yv_Reg{code1, code2, code3}; }
constexpr OpCodeHandler_Yv_Reg2 Yv_Reg2(Code code1, Code code2) noexcept { return OpCodeHandler_Yv_Reg2{code1, code2}; }
constexpr OpCodeHandler_Yv_Xv Yv_Xv(Code code1, Code code2, Code code3) noexcept { return OpCodeHandler_Yv_Xv{code1, code2, code3}; }
constexpr OpCodeHandler_M_Sw M_Sw(Code code) noexcept { return OpCodeHandler_M_Sw{code}; }
constexpr OpCodeHandler_Sw_M Sw_M(Code code) noexcept { return OpCodeHandler_Sw_M{code}; }
constexpr OpCodeHandler_Rq Rq(Code code) noexcept { return OpCodeHandler_Rq{code}; }
constexpr OpCodeHandler_Gd_Rd Gd_Rd(Code code) noexcept { return OpCodeHandler_Gd_Rd{code}; }
constexpr OpCodeHandler_PrefixEsCsSsDs PrefixEsCsSsDs(Register reg) noexcept { return OpCodeHandler_PrefixEsCsSsDs{reg}; }
constexpr OpCodeHandler_PrefixFsGs PrefixFsGs(Register reg) noexcept { return OpCodeHandler_PrefixFsGs{reg}; }
constexpr OpCodeHandler_Prefix66 Prefix66() noexcept { return OpCodeHandler_Prefix66{}; }
constexpr OpCodeHandler_Prefix67 Prefix67() noexcept { return OpCodeHandler_Prefix67{}; }
constexpr OpCodeHandler_PrefixF0 PrefixF0() noexcept { return OpCodeHandler_PrefixF0{}; }
constexpr OpCodeHandler_PrefixF2 PrefixF2() noexcept { return OpCodeHandler_PrefixF2{}; }
constexpr OpCodeHandler_PrefixF3 PrefixF3() noexcept { return OpCodeHandler_PrefixF3{}; }
constexpr OpCodeHandler_PrefixREX PrefixREX(const OpCodeHandler* handler, std::uint32_t value) noexcept {
	return OpCodeHandler_PrefixREX{handler, value};
}

} // namespace iced_x86::internal::legacy_ctors
