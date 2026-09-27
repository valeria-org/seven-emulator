// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/iced_constants.hpp"
#include "iced_x86/op_code_info.hpp"
#include <cstdint>

namespace iced_x86::internal {

struct OpCodeInfoInternal {
	// One `OpCodeInfo` per `Code` value (generated: src/encoder/op_code_info_table.cpp)
	static const OpCodeInfo TABLE[IcedConstants::CODE_ENUM_COUNT];

	// The op code strings and instruction strings (not NUL terminated). Each `OpCodeInfo`'s op code string
	// is immediately followed by its instruction string. They're stored in several chunks (each one < 64KB)
	// since some compilers have a max string literal length. `OpCodeInfo::strings_offset_` is
	// `(chunk_index << STRINGS_CHUNK_SHIFT) | offset_in_chunk`.
	static const char* const STRINGS[];
	static constexpr std::uint32_t STRINGS_CHUNK_SHIFT = 16;
	static constexpr std::uint32_t STRINGS_OFFSET_MASK = (1U << STRINGS_CHUNK_SHIFT) - 1;

	static const char* get_strings(const OpCodeInfo& info) noexcept {
		return STRINGS[info.strings_offset_ >> STRINGS_CHUNK_SHIFT] + (info.strings_offset_ & STRINGS_OFFSET_MASK);
	}
};

} // namespace iced_x86::internal
