// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// MVEX handler factory fns (Rust: decoder/table_de/mvex_reader.rs)
//
// One `constexpr` fn per handler kind (the generator's `MvexOpCodeHandlerKind`), same name as the kind. They're only
// called by the generated tables (src/decoder/data_mvex.cpp, see `CppDecoderTableWriter`) to create the `constexpr`
// handlers. The args are the args of the generator's handler definition (same order, all `Code` values are passed):
// enum values, bools, ints, pointers to other handlers and braced lists of handler pointers (too many elements don't
// compile, a missing element is null which fails the ctor's constant evaluation).

#pragma once

#include "internal/decoder/handlers.hpp"
#include "internal/decoder/handlers_mvex.hpp"

#include <cstddef>
#include <cstdint>

namespace iced_x86::internal::mvex_ctors {

constexpr OpCodeHandler_RM RM(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_RM{handler1, handler2}; }
constexpr OpCodeHandler_Group Group(const OpCodeHandler* const (&handlers)[8]) noexcept { return OpCodeHandler_Group{handlers}; }
constexpr OpCodeHandler_W W(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_W{handler1, handler2}; }
constexpr OpCodeHandler_MandatoryPrefix2 MandatoryPrefix2(
		const OpCodeHandler* handler1, const OpCodeHandler* handler2, const OpCodeHandler* handler3, const OpCodeHandler* handler4) noexcept {
	return OpCodeHandler_MandatoryPrefix2{true, handler1, handler2, handler3, handler4};
}
constexpr OpCodeHandler_EH EH(const OpCodeHandler* handler1, const OpCodeHandler* handler2) noexcept { return OpCodeHandler_EH{handler1, handler2}; }
constexpr OpCodeHandler_MVEX_M M(Code code) noexcept { return OpCodeHandler_MVEX_M{code}; }
constexpr OpCodeHandler_MVEX_MV MV(Code code) noexcept { return OpCodeHandler_MVEX_MV{code}; }
constexpr OpCodeHandler_MVEX_VW VW(Code code) noexcept { return OpCodeHandler_MVEX_VW{code}; }
constexpr OpCodeHandler_MVEX_HWIb HWIb(Code code) noexcept { return OpCodeHandler_MVEX_HWIb{code}; }
constexpr OpCodeHandler_MVEX_VWIb VWIb(Code code) noexcept { return OpCodeHandler_MVEX_VWIb{code}; }
constexpr OpCodeHandler_MVEX_VHW VHW(Code code) noexcept { return OpCodeHandler_MVEX_VHW{code}; }
constexpr OpCodeHandler_MVEX_VHWIb VHWIb(Code code) noexcept { return OpCodeHandler_MVEX_VHWIb{code}; }
constexpr OpCodeHandler_MVEX_VKW VKW(Code code) noexcept { return OpCodeHandler_MVEX_VKW{code}; }
constexpr OpCodeHandler_MVEX_KHW KHW(Code code) noexcept { return OpCodeHandler_MVEX_KHW{code}; }
constexpr OpCodeHandler_MVEX_KHWIb KHWIb(Code code) noexcept { return OpCodeHandler_MVEX_KHWIb{code}; }
constexpr OpCodeHandler_MVEX_VSIB VSIB(Code code) noexcept { return OpCodeHandler_MVEX_VSIB{code}; }
constexpr OpCodeHandler_MVEX_VSIB_V VSIB_V(Code code) noexcept { return OpCodeHandler_MVEX_VSIB_V{code}; }
constexpr OpCodeHandler_MVEX_V_VSIB V_VSIB(Code code) noexcept { return OpCodeHandler_MVEX_V_VSIB{code}; }

} // namespace iced_x86::internal::mvex_ctors
