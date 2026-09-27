// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// 3DNow! op code handler (Rust: decoder/handlers/d3now.rs)

#pragma once

#include "internal/decoder/handlers.hpp"

namespace iced_x86::internal {

struct OpCodeHandler_D3NOW : OpCodeHandler {
	constexpr OpCodeHandler_D3NOW() noexcept : OpCodeHandler(&decode, true) {}
	ICED_DECODE_FN_DECL;
};

} // namespace iced_x86::internal
