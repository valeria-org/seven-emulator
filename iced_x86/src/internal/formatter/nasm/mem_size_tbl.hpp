// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Nasm formatter memory size table (Rust: formatter/nasm/mem_size_tbl.rs)

#pragma once

#include "internal/formatter/formatter_string.hpp"

namespace iced_x86::internal::nasm {

/// Memory size info (Rust: `mem_size_tbl::Info`)
struct MemSizeInfo {
	const FormatterString* keyword;
	const FormatterString* bcst_to;
};

/// Gets the memory size infos (index = `MemorySize` value). It's constant data.
const MemSizeInfo* get_mem_size_tbl() noexcept;

} // namespace iced_x86::internal::nasm
