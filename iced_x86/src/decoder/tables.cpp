// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/tables.hpp"
#include "internal/decoder/data_evex.hpp"
#include "internal/decoder/data_legacy.hpp"
#include "internal/decoder/data_mvex.hpp"
#include "internal/decoder/data_vex.hpp"
#include "internal/decoder/data_xop.hpp"

#include <cstddef>

namespace iced_x86::internal {

namespace {
struct InvalidMap {
	HandlerEntry entries[0x100];
	constexpr InvalidMap() noexcept : entries() {
		for (std::size_t i = 0; i < 0x100; i++)
			entries[i] = to_handler_entry(get_invalid_handler());
	}
};
} // namespace

static constexpr InvalidMap INVALID_MAP{};

// Rust creates the handlers and tables at runtime (lazy_static). Here they're constant data: no heap, no startup code, no locks.
constexpr DecoderTables DECODER_TABLES = {
	INVALID_MAP.entries,
	decoder_data_legacy::HANDLERS_MAP0,
	decoder_data_vex::HANDLERS_MAP0,
	decoder_data_vex::HANDLERS_0F,
	decoder_data_vex::HANDLERS_0F38,
	decoder_data_vex::HANDLERS_0F3A,
	decoder_data_evex::HANDLERS_0F,
	decoder_data_evex::HANDLERS_0F38,
	decoder_data_evex::HANDLERS_0F3A,
	decoder_data_evex::HANDLERS_MAP5,
	decoder_data_evex::HANDLERS_MAP6,
	decoder_data_xop::HANDLERS_MAP8,
	decoder_data_xop::HANDLERS_MAP9,
	decoder_data_xop::HANDLERS_MAP10,
	decoder_data_mvex::HANDLERS_0F,
	decoder_data_mvex::HANDLERS_0F38,
	decoder_data_mvex::HANDLERS_0F3A,
};

} // namespace iced_x86::internal
