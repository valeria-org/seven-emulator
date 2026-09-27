// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/gas/regs.rs

#pragma once

#include "internal/formatter/regs_tbl_ls.hpp"

namespace iced_x86::internal::gas {

/// All register names with a `%` prefix (Rust: `ALL_REGISTERS`). Generated constant data (src/formatter/gas/regs.cpp)
extern const RegsTbl ALL_REGISTERS;

/// Gets all register names with a `%` prefix (Rust: `ALL_REGISTERS`)
inline const RegsTbl& get_all_registers() noexcept { return ALL_REGISTERS; }

} // namespace iced_x86::internal::gas
