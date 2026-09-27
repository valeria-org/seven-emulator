// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// MVEX op code handlers (Rust: decoder/handlers/mvex.rs)

#pragma once

#include "internal/decoder/handlers.hpp"
#include "internal/mvex/mvex.hpp"

namespace iced_x86::internal {

struct OpCodeHandler_EH : OpCodeHandler {
	const OpCodeHandler* handlers[2];
	constexpr OpCodeHandler_EH(const OpCodeHandler* handler_eh0, const OpCodeHandler* handler_eh1) noexcept
		: OpCodeHandler(&decode, true), handlers{handler_eh0, handler_eh1} {
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_eh0));
		ICED_DEBUG_ASSERT(!is_null_instance_handler(handler_eh1));
	}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_M : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_M(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_MV : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_MV(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VW : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VW(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_HWIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_HWIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VWIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VWIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VHW : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VHW(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VHWIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VHWIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VKW : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VKW(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_KHW : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_KHW(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_KHWIb : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_KHWIb(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VSIB : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VSIB(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_VSIB_V : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_VSIB_V(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

struct OpCodeHandler_MVEX_V_VSIB : OpCodeHandler {
	Code code;
	explicit constexpr OpCodeHandler_MVEX_V_VSIB(Code code_) noexcept : OpCodeHandler(&decode, true), code(code_) {}
	ICED_DECODE_FN_DECL;
};

} // namespace iced_x86::internal
