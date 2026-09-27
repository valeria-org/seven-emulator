// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// VEX handler factory fns (Rust: decoder/table_de/vex_reader.rs)
//
// One `constexpr` fn per handler kind (the generator's `VexOpCodeHandlerKind`), same name as the kind. They're only
// called by the generated tables (src/decoder/data_vex.cpp and data_xop.cpp, see `CppDecoderTableWriter`) to create the
// `constexpr` handlers. The args are the args of the generator's handler definition (same order, all `Code` values are
// passed): enum values, bools, ints, pointers to other handlers and braced lists of handler pointers (too many elements
// don't compile, a missing element is null which fails the ctor's constant evaluation).

#pragma once

#include "internal/decoder/handlers.hpp"
#include "internal/decoder/handlers_vex.hpp"

#include <cstddef>
#include <cstdint>

namespace iced_x86::internal::vex_ctors {

constexpr OpCodeHandler_VEX_VHEv VHEv(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_VHEv{reg, code1, code2}; }
constexpr OpCodeHandler_VEX_VHEvIb VHEvIb(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_VHEvIb{reg, code1, code2}; }
constexpr OpCodeHandler_VEX_VHIs4W VHIs4W(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHIs4W{reg, code}; }
constexpr OpCodeHandler_VEX_VHIs5W VHIs5W(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHIs5W{reg, code}; }
constexpr OpCodeHandler_VEX_VHM VHM(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHM{reg, code}; }
constexpr OpCodeHandler_VEX_VHW VHW_2(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHW{reg, reg, reg, code, code}; }
constexpr OpCodeHandler_VEX_VHW VHW_3(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_VHW{reg, reg, reg, code1, code2}; }
// Rust: OpCodeHandler_VEX_VHW::new1()
constexpr OpCodeHandler_VEX_VHW VHW_4(Register reg1, Register reg2, Register reg3, Code code) noexcept {
	return OpCodeHandler_VEX_VHW{reg1, reg2, reg3, code};
}
constexpr OpCodeHandler_VEX_VHWIb VHWIb_2(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHWIb{reg, reg, reg, code}; }
constexpr OpCodeHandler_VEX_VHWIb VHWIb_4(Register reg1, Register reg2, Register reg3, Code code) noexcept {
	return OpCodeHandler_VEX_VHWIb{reg1, reg2, reg3, code};
}
constexpr OpCodeHandler_VEX_VHWIs4 VHWIs4(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHWIs4{reg, code}; }
constexpr OpCodeHandler_VEX_VHWIs5 VHWIs5(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VHWIs5{reg, code}; }
constexpr OpCodeHandler_VEX_VK_HK_RK VK_HK_RK(Code code) noexcept { return OpCodeHandler_VEX_VK_HK_RK{code}; }
constexpr OpCodeHandler_VEX_VK_R VK_R(Code code, Register reg) noexcept { return OpCodeHandler_VEX_VK_R{code, reg}; }
constexpr OpCodeHandler_VEX_VK_RK VK_RK(Code code) noexcept { return OpCodeHandler_VEX_VK_RK{code}; }
constexpr OpCodeHandler_VEX_VK_RK_Ib VK_RK_Ib(Code code) noexcept { return OpCodeHandler_VEX_VK_RK_Ib{code}; }
constexpr OpCodeHandler_VEX_VK_WK VK_WK(Code code) noexcept { return OpCodeHandler_VEX_VK_WK{code}; }
constexpr OpCodeHandler_VEX_VM VM(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VM{reg, code}; }
constexpr OpCodeHandler_VEX_VW VW_2(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VW{reg, reg, code}; }
constexpr OpCodeHandler_VEX_VW VW_3(Register reg1, Register reg2, Code code) noexcept { return OpCodeHandler_VEX_VW{reg1, reg2, code}; }
constexpr OpCodeHandler_VEX_VWH VWH(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VWH{reg, code}; }
constexpr OpCodeHandler_VEX_VWIb VWIb_2(Register reg, Code code) noexcept { return OpCodeHandler_VEX_VWIb{reg, reg, code, code}; }
constexpr OpCodeHandler_VEX_VWIb VWIb_3(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_VWIb{reg, reg, code1, code2}; }
constexpr OpCodeHandler_VEX_VX_Ev VX_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_VX_Ev{code1, code2}; }
constexpr OpCodeHandler_VEX_VX_VSIB_HX VX_VSIB_HX(Register reg1, Register reg2, Register reg3, Code code) noexcept {
	return OpCodeHandler_VEX_VX_VSIB_HX{reg1, reg2, reg3, code};
}
constexpr OpCodeHandler_VEX_WHV WHV(Register reg, Code code) noexcept { return OpCodeHandler_VEX_WHV{reg, code}; }
constexpr OpCodeHandler_VEX_WV WV(Register reg, Code code) noexcept { return OpCodeHandler_VEX_WV{reg, code}; }
constexpr OpCodeHandler_VEX_WVIb WVIb(Register reg1, Register reg2, Code code) noexcept { return OpCodeHandler_VEX_WVIb{reg1, reg2, code}; }
constexpr OpCodeHandler_VEX_VT_SIBMEM VT_SIBMEM(Code code) noexcept { return OpCodeHandler_VEX_VT_SIBMEM{code}; }
constexpr OpCodeHandler_VEX_SIBMEM_VT SIBMEM_VT(Code code) noexcept { return OpCodeHandler_VEX_SIBMEM_VT{code}; }
constexpr OpCodeHandler_VEX_VT VT(Code code) noexcept { return OpCodeHandler_VEX_VT{code}; }
constexpr OpCodeHandler_VEX_VT_RT_HT VT_RT_HT(Code code) noexcept { return OpCodeHandler_VEX_VT_RT_HT{code}; }
constexpr OpCodeHandler_Options_DontReadModRM Options_DontReadModRM(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, std::uint32_t options) noexcept {
	return OpCodeHandler_Options_DontReadModRM{handler1, handler2, options};
}
constexpr OpCodeHandler_VEX_Gq_HK_RK Gq_HK_RK(Code code) noexcept { return OpCodeHandler_VEX_Gq_HK_RK{code}; }
constexpr OpCodeHandler_VEX_VK_R_Ib VK_R_Ib(Code code, Register reg) noexcept { return OpCodeHandler_VEX_VK_R_Ib{code, reg}; }
constexpr OpCodeHandler_VEX_Gv_Ev Gv_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_Ev{code1, code2}; }
constexpr OpCodeHandler_VEX_Ev Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Ev{code1, code2}; }
constexpr OpCodeHandler_VEX_K_Jb K_Jb(Code code) noexcept { return OpCodeHandler_VEX_K_Jb{code}; }
constexpr OpCodeHandler_VEX_K_Jz K_Jz(Code code) noexcept { return OpCodeHandler_VEX_K_Jz{code}; }
constexpr OpCodeHandler_Bitness Bitness(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_Bitness{handler1, handler2};
}
constexpr OpCodeHandler_Bitness_DontReadModRM Bitness_DontReadModRM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_Bitness_DontReadModRM{handler1, handler2};
}
constexpr OpCodeHandler_RM RM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_RM{handler1, handler2}; }
constexpr OpCodeHandler_Group Group(const OpCodeHandler* const (&handlers)[8]) noexcept { return OpCodeHandler_Group{handlers}; }
constexpr OpCodeHandler_Group8x64 Group8x64(const OpCodeHandler* const (&handlers1)[8], const OpCodeHandler* const (&handlers2)[0x40]) noexcept {
	return OpCodeHandler_Group8x64{handlers1, handlers2};
}
constexpr OpCodeHandler_W W(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_W{handler1, handler2}; }
constexpr OpCodeHandler_MandatoryPrefix2 MandatoryPrefix2_1(const OpCodeHandler* handler) noexcept {
	return OpCodeHandler_MandatoryPrefix2{true, handler, get_invalid_handler(), get_invalid_handler(), get_invalid_handler()};
}
constexpr OpCodeHandler_MandatoryPrefix2 MandatoryPrefix2_4(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix2{true, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_MandatoryPrefix2 MandatoryPrefix2_NoModRM(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix2{false, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_VectorLength_VEX VectorLength_NoModRM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_VectorLength_VEX{false, handler1, handler2};
}
constexpr OpCodeHandler_VectorLength_VEX VectorLength(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept {
	return OpCodeHandler_VectorLength_VEX{true, handler1, handler2};
}
constexpr OpCodeHandler_VEX_Ed_V_Ib Ed_V_Ib(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Ed_V_Ib{reg, code1, code2}; }
constexpr OpCodeHandler_VEX_Ev_VX Ev_VX(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Ev_VX{code1, code2}; }
constexpr OpCodeHandler_VEX_G_VK G_VK(Code code, Register reg) noexcept { return OpCodeHandler_VEX_G_VK{code, reg}; }
constexpr OpCodeHandler_VEX_Gv_Ev_Gv Gv_Ev_Gv(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_Ev_Gv{code1, code2}; }
constexpr OpCodeHandler_VEX_Ev_Gv_Gv Ev_Gv_Gv(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Ev_Gv_Gv{code1, code2}; }
constexpr OpCodeHandler_VEX_Gv_Ev_Ib Gv_Ev_Ib(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_Ev_Ib{code1, code2}; }
constexpr OpCodeHandler_VEX_Gv_Ev_Id Gv_Ev_Id(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_Ev_Id{code1, code2}; }
constexpr OpCodeHandler_VEX_Gv_GPR_Ib Gv_GPR_Ib(Register reg, Code code1, Code code2) noexcept {
	return OpCodeHandler_VEX_Gv_GPR_Ib{reg, code1, code2};
}
constexpr OpCodeHandler_VEX_Gv_Gv_Ev Gv_Gv_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_Gv_Ev{code1, code2}; }
constexpr OpCodeHandler_VEX_Gv_RX Gv_RX(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_RX{reg, code1, code2}; }
constexpr OpCodeHandler_VEX_Gv_W Gv_W(Register reg, Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Gv_W{reg, code1, code2}; }
constexpr OpCodeHandler_VEX_GvM_VX_Ib GvM_VX_Ib(Register reg, Code code1, Code code2) noexcept {
	return OpCodeHandler_VEX_GvM_VX_Ib{reg, code1, code2};
}
constexpr OpCodeHandler_VEX_HRIb HRIb(Register reg, Code code) noexcept { return OpCodeHandler_VEX_HRIb{reg, code}; }
constexpr OpCodeHandler_VEX_Hv_Ed_Id Hv_Ed_Id(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Hv_Ed_Id{code1, code2}; }
constexpr OpCodeHandler_VEX_Hv_Ev Hv_Ev(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_Hv_Ev{code1, code2}; }
constexpr OpCodeHandler_VEX_M M(Code code) noexcept { return OpCodeHandler_VEX_M{code}; }
constexpr OpCodeHandler_VEX_MHV MHV(Register reg, Code code) noexcept { return OpCodeHandler_VEX_MHV{reg, code}; }
constexpr OpCodeHandler_VEX_M_VK M_VK(Code code) noexcept { return OpCodeHandler_VEX_M_VK{code}; }
constexpr OpCodeHandler_VEX_MV MV(Register reg, Code code) noexcept { return OpCodeHandler_VEX_MV{reg, code}; }
constexpr OpCodeHandler_VEX_rDI_VX_RX rDI_VX_RX(Register reg, Code code) noexcept { return OpCodeHandler_VEX_rDI_VX_RX{reg, code}; }
constexpr OpCodeHandler_VEX_RdRq RdRq(Code code1, Code code2) noexcept { return OpCodeHandler_VEX_RdRq{code1, code2}; }
constexpr OpCodeHandler_VEX_Simple Simple(Code code) noexcept { return OpCodeHandler_VEX_Simple{code}; }

} // namespace iced_x86::internal::vex_ctors
