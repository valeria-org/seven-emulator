// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Pseudo op mnemonics used by the gas/intel/masm/nasm formatters (Rust: formatter/pseudo_ops.rs)
// The pseudo ops are defined in pseudo_ops_defs.hpp (shared with the fast formatter)

#pragma once

#include <cstddef>
#include <cstdint>

#include "internal/formatter/formatter_string.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

/// The pseudo op mnemonics of a `PseudoOpsKind` (index = imm8 value). It points to constant data.
class PseudoOps {
public:
	constexpr PseudoOps(const char* strings, const std::uint16_t* offsets, std::size_t size) noexcept
		: strings_(strings), offsets_(offsets), size_(size) {}

	constexpr std::size_t size() const noexcept { return size_; }

	FormatterString operator[](std::size_t index) const noexcept {
		ICED_ASSERT(index < size_);
		return FormatterString(strings_ + offsets_[index]);
	}

private:
	const char* strings_;
	const std::uint16_t* offsets_;
	std::size_t size_;
};

/// Gets the pseudo op mnemonics (index = imm8 value) of a `PseudoOpsKind`
PseudoOps get_pseudo_ops(PseudoOpsKind kind) noexcept;

} // namespace iced_x86::internal
