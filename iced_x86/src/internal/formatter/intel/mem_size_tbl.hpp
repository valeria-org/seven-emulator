// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/intel/mem_size_tbl.rs

#pragma once

#include <array>

#include "iced_x86/iced_constants.hpp"
#include "internal/formatter/formatter_string.hpp"

namespace iced_x86::internal::intel {

/// Memory size info (Rust: `mem_size_tbl::Info`)
struct MemSizeInfo {
	/// The broadcast decorator (eg. `1to8`) or the empty string if it's not a broadcast memory size
	const FormatterString* bcst_to;
	/// The memory keywords, eg. `dword ptr`
	FormatterStringSlice keywords;
};

using MemSizeTbl = std::array<MemSizeInfo, IcedConstants::MEMORY_SIZE_ENUM_COUNT>;

/// Gets the info of each `MemorySize` (Rust: `MEM_SIZE_TBL`). It's constant data.
const MemSizeTbl& get_mem_size_tbl() noexcept;

} // namespace iced_x86::internal::intel
