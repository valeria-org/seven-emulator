// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Masm formatter memory size table (Rust: formatter/masm/mem_size_tbl.rs)

#pragma once

#include <cstdint>

#include "internal/formatter/formatter_string.hpp"

namespace iced_x86::internal::masm {

/// Memory size info (Rust: `mem_size_tbl::Info`)
struct MemSizeInfo {
	FormatterStringSlice keywords;
	std::uint32_t size;
	bool is_broadcast;
};

/// Gets the memory size infos (index = `MemorySize` value). It's constant data.
const MemSizeInfo* get_mem_size_tbl() noexcept;

} // namespace iced_x86::internal::masm
