// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// EVEX handler factory fns (Rust: decoder/table_de/evex_reader.rs)
//
// One `constexpr` fn per handler kind (the generator's `EvexOpCodeHandlerKind`), same name as the kind. They're only
// called by the generated tables (src/decoder/data_evex.cpp, see `CppDecoderTableWriter`) to create the `constexpr`
// handlers. The args are the args of the generator's handler definition (same order, all `Code` values are passed):
// enum values, bools, ints, pointers to other handlers and braced lists of handler pointers (too many elements don't
// compile, a missing element is null which fails the ctor's constant evaluation).

#pragma once

#include "internal/decoder/handlers.hpp"
#include "internal/decoder/handlers_evex.hpp"

#include <cstddef>
#include <cstdint>

namespace iced_x86::internal::evex_ctors {

constexpr OpCodeHandler_EVEX_VkHW VkHW_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHW{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHW VkHW_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHW{reg, code, tuple_type, true};
}
// Rust: OpCodeHandler_EVEX_VkHW::new1()
constexpr OpCodeHandler_EVEX_VkHW VkHW_5(Register reg1, Register reg2, Register reg3, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHW{reg1, reg2, reg3, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHW_er VkHW_er_4(Register reg, Code code, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_VkHW_er{reg, code, tuple_type, b_, false};
}
constexpr OpCodeHandler_EVEX_VkHW_er VkHW_er_4b(Register reg, Code code, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_VkHW_er{reg, code, tuple_type, b_, true};
}
constexpr OpCodeHandler_EVEX_VkHW_er_ur VkHW_er_ur_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHW_er_ur{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHW_er_ur VkHW_er_ur_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHW_er_ur{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_VkHWIb VkHWIb_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHWIb{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHWIb VkHWIb_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHWIb{reg, code, tuple_type, true};
}
// Rust: OpCodeHandler_EVEX_VkHWIb::new1()
constexpr OpCodeHandler_EVEX_VkHWIb VkHWIb_5(Register reg1, Register reg2, Register reg3, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHWIb{reg1, reg2, reg3, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHWIb_er VkHWIb_er_4(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHWIb_er{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkHWIb_er VkHWIb_er_4b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHWIb_er{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_VkM VkM(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_VkM{reg, code, tuple_type}; }
constexpr OpCodeHandler_EVEX_VkW VkW_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkW{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkW VkW_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkW{reg, code, tuple_type, true};
}
// Rust: OpCodeHandler_EVEX_VkW::new1()
constexpr OpCodeHandler_EVEX_VkW VkW_4(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkW{reg1, reg2, code, tuple_type, false};
}
// Rust: OpCodeHandler_EVEX_VkW::new1()
constexpr OpCodeHandler_EVEX_VkW VkW_4b(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkW{reg1, reg2, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_VkW_er VkW_er_4(Register reg, Code code, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_VkW_er{reg, code, tuple_type, b_};
}
// Rust: OpCodeHandler_EVEX_VkW_er::new1()
constexpr OpCodeHandler_EVEX_VkW_er VkW_er_5(Register reg1, Register reg2, Code code, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_VkW_er{reg1, reg2, code, tuple_type, b_};
}
// Rust: OpCodeHandler_EVEX_VkW_er::new2()
constexpr OpCodeHandler_EVEX_VkW_er VkW_er_6(Register reg1, Register reg2, Code code, TupleType tuple_type, bool b1, bool b2) noexcept {
	return OpCodeHandler_EVEX_VkW_er{reg1, reg2, code, tuple_type, b1, b2};
}
constexpr OpCodeHandler_EVEX_VkWIb VkWIb_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkWIb{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_VkWIb VkWIb_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkWIb{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_VkWIb_er VkWIb_er(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkWIb_er{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VM VM(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_VM{reg, code, tuple_type}; }
constexpr OpCodeHandler_EVEX_VSIB_k1 VSIB_k1(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VSIB_k1{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VSIB_k1_VX VSIB_k1_VX(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VSIB_k1_VX{reg1, reg2, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VW VW(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_VW{reg, code, tuple_type}; }
constexpr OpCodeHandler_EVEX_VW_er VW_er(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VW_er{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VX_Ev VX_Ev(Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_VX_Ev{code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_WkHV WkHV(Register reg, Code code) noexcept { return OpCodeHandler_EVEX_WkHV{reg, code}; }
constexpr OpCodeHandler_EVEX_WkV WkV_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_WkV{reg, code, tuple_type};
}
// Rust: OpCodeHandler_EVEX_WkV::new2()
constexpr OpCodeHandler_EVEX_WkV WkV_4a(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_WkV{reg1, reg2, code, tuple_type};
}
// Rust: OpCodeHandler_EVEX_WkV::new1()
constexpr OpCodeHandler_EVEX_WkV WkV_4b(Register reg, Code code, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_WkV{reg, code, tuple_type, b_};
}
constexpr OpCodeHandler_EVEX_WkVIb WkVIb(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_WkVIb{reg1, reg2, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_WkVIb_er WkVIb_er(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_WkVIb_er{reg1, reg2, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_WV WV(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_WV{reg, code, tuple_type}; }
constexpr OpCodeHandler_RM RM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_RM{handler1, handler2}; }
constexpr OpCodeHandler_Group Group(const OpCodeHandler* const (&handlers)[8]) noexcept { return OpCodeHandler_Group{handlers}; }
constexpr OpCodeHandler_W W(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_W{handler1, handler2}; }
constexpr OpCodeHandler_MandatoryPrefix2 MandatoryPrefix2(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix2{true, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_VectorLength_EVEX VectorLength(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3) noexcept {
	return OpCodeHandler_VectorLength_EVEX{handler1, handler2, handler3};
}
constexpr OpCodeHandler_VectorLength_EVEX_er VectorLength_er(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3) noexcept {
	return OpCodeHandler_VectorLength_EVEX_er{handler1, handler2, handler3};
}
constexpr OpCodeHandler_EVEX_Ed_V_Ib Ed_V_Ib(Register reg, Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_Ed_V_Ib{reg, code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_Ev_VX Ev_VX(Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_Ev_VX{code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_Ev_VX_Ib Ev_VX_Ib(Register reg, Code code1, Code code2) noexcept {
	return OpCodeHandler_EVEX_Ev_VX_Ib{reg, code1, code2};
}
constexpr OpCodeHandler_EVEX_Gv_W_er Gv_W_er(Register reg, Code code1, Code code2, TupleType tuple_type, bool b_) noexcept {
	return OpCodeHandler_EVEX_Gv_W_er{reg, code1, code2, tuple_type, b_};
}
constexpr OpCodeHandler_EVEX_GvM_VX_Ib GvM_VX_Ib(Register reg, Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_GvM_VX_Ib{reg, code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_HkWIb HkWIb_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_HkWIb{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_HkWIb HkWIb_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_HkWIb{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_HWIb HWIb(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_HWIb{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_KkHW KkHW_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHW{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_KkHW KkHW_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHW{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_KkHWIb_sae KkHWIb_sae_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHWIb_sae{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_KkHWIb_sae KkHWIb_sae_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHWIb_sae{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_KkHWIb KkHWIb_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHWIb{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_KkHWIb KkHWIb_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkHWIb{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_KkWIb KkWIb_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkWIb{reg, code, tuple_type, false};
}
constexpr OpCodeHandler_EVEX_KkWIb KkWIb_3b(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KkWIb{reg, code, tuple_type, true};
}
constexpr OpCodeHandler_EVEX_KP1HW KP1HW(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_KP1HW{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_KR KR(Register reg, Code code) noexcept { return OpCodeHandler_EVEX_KR{reg, code}; }
constexpr OpCodeHandler_EVEX_MV MV(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_MV{reg, code, tuple_type}; }
constexpr OpCodeHandler_EVEX_V_H_Ev_er V_H_Ev_er(Register reg, Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_V_H_Ev_er{reg, code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_V_H_Ev_Ib V_H_Ev_Ib(Register reg, Code code1, Code code2, TupleType tuple_type1, TupleType tuple_type2) noexcept {
	return OpCodeHandler_EVEX_V_H_Ev_Ib{reg, code1, code2, tuple_type1, tuple_type2};
}
constexpr OpCodeHandler_EVEX_VHM VHM(Register reg, Code code, TupleType tuple_type) noexcept { return OpCodeHandler_EVEX_VHM{reg, code, tuple_type}; }
// Rust: OpCodeHandler_EVEX_VHW::new2()
constexpr OpCodeHandler_EVEX_VHW VHW_3(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VHW{reg, code, tuple_type};
}
// Rust: OpCodeHandler_EVEX_VHW::new()
constexpr OpCodeHandler_EVEX_VHW VHW_4(Register reg, Code code1, Code code2, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VHW{reg, code1, code2, tuple_type};
}
constexpr OpCodeHandler_EVEX_VHWIb VHWIb(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VHWIb{reg, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VK VK(Register reg, Code code) noexcept { return OpCodeHandler_EVEX_VK{reg, code}; }
constexpr OpCodeHandler_EVEX_Vk_VSIB Vk_VSIB(Register reg1, Register reg2, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_Vk_VSIB{reg1, reg2, code, tuple_type};
}
constexpr OpCodeHandler_EVEX_VkEv_REXW VkEv_REXW_2(Register reg, Code code) noexcept {
	return OpCodeHandler_EVEX_VkEv_REXW{reg, code, Code::INVALID};
}
constexpr OpCodeHandler_EVEX_VkEv_REXW VkEv_REXW_3(Register reg, Code code1, Code code2) noexcept {
	return OpCodeHandler_EVEX_VkEv_REXW{reg, code1, code2};
}
constexpr OpCodeHandler_EVEX_VkHM VkHM(Register reg, Code code, TupleType tuple_type) noexcept {
	return OpCodeHandler_EVEX_VkHM{reg, code, tuple_type};
}

} // namespace iced_x86::internal::evex_ctors
