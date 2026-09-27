// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Nasm formatter register names (Rust: formatter/nasm/regs.rs)

#pragma once

#include "internal/formatter/regs_tbl_ls.hpp"

namespace iced_x86::internal::nasm {

/// The register names (index = `Register` value). Same as `REGS_TBL` except `st0`-`st7` are used instead of
/// `st(0)`-`st(7)`. Generated constant data (src/formatter/nasm/regs.cpp)
extern const RegsTbl ALL_REGISTERS;

/// Gets the register names (see `ALL_REGISTERS`)
inline const FormatterString* get_all_registers() noexcept { return ALL_REGISTERS.data(); }

} // namespace iced_x86::internal::nasm
