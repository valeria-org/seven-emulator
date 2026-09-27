// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// The decoder tables (Rust: decoder/handlers/tables.rs)

#pragma once

#include "internal/decoder/handlers.hpp"

namespace iced_x86::internal {

// Each table has 0x100 handlers
struct DecoderTables {
	const HandlerEntry* invalid_map;
	const HandlerEntry* handlers_map0;
	const HandlerEntry* handlers_vex_map0;
	const HandlerEntry* handlers_vex_0f;
	const HandlerEntry* handlers_vex_0f38;
	const HandlerEntry* handlers_vex_0f3a;
	const HandlerEntry* handlers_evex_0f;
	const HandlerEntry* handlers_evex_0f38;
	const HandlerEntry* handlers_evex_0f3a;
	const HandlerEntry* handlers_evex_map5;
	const HandlerEntry* handlers_evex_map6;
	const HandlerEntry* handlers_xop_map8;
	const HandlerEntry* handlers_xop_map9;
	const HandlerEntry* handlers_xop_map10;
	const HandlerEntry* handlers_mvex_0f;
	const HandlerEntry* handlers_mvex_0f38;
	const HandlerEntry* handlers_mvex_0f3a;
};

// Constant data (src/decoder/tables.cpp), the tables are generated (src/decoder/data_*.cpp)
extern const DecoderTables DECODER_TABLES;

inline const DecoderTables& get_decoder_tables() noexcept { return DECODER_TABLES; }

} // namespace iced_x86::internal
