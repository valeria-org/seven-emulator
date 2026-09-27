// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/gas/mem_size_tbl.rs

#pragma once

#include <array>

#include "iced_x86/iced_constants.hpp"
#include "internal/formatter/formatter_string.hpp"

namespace iced_x86::internal::gas {

/// The broadcast decorator (eg. `1to8`) of each `MemorySize` (the empty string if it's not a broadcast memory size)
using MemSizeTbl = std::array<const FormatterString*, IcedConstants::MEMORY_SIZE_ENUM_COUNT>;

/// Gets the broadcast decorator of each `MemorySize` (Rust: `MEM_SIZE_TBL`). It's constant data.
const MemSizeTbl& get_mem_size_tbl() noexcept;

} // namespace iced_x86::internal::gas
