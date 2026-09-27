// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Register names used by the gas/intel/masm/nasm formatters (Rust: formatter/regs_tbl_ls.rs)

#pragma once

#include <array>

#include "iced_x86/iced_constants.hpp"
#include "internal/formatter/formatter_string.hpp"

namespace iced_x86::internal {

using RegsTbl = std::array<FormatterString, IcedConstants::REGISTER_ENUM_COUNT>;

/// The lowercase/uppercase register names (index = `Register` value). Generated constant data (src/formatter/regs_tbl_ls.cpp)
extern const RegsTbl REGS_TBL;

/// Gets the lowercase/uppercase register names (index = `Register` value)
inline const RegsTbl& get_regs_tbl() noexcept { return REGS_TBL; }

} // namespace iced_x86::internal
