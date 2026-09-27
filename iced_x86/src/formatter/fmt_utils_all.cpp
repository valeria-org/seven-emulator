// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/internal/fmt_utils_all.hpp"

#include "internal/code_internal.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

bool show_segment_prefix_bool(Register default_seg_reg, const Instruction& instruction, bool show_useless_prefixes) noexcept {
	if (code_ignores_segment(instruction.code()))
		return show_useless_prefixes;
	const Register prefix_seg = instruction.segment_prefix();
	ICED_DEBUG_ASSERT(prefix_seg != Register::None);
	if (is_code64(instruction.code_size())) {
		// ES,CS,SS,DS are ignored
		if (prefix_seg == Register::FS || prefix_seg == Register::GS)
			return true;
		return show_useless_prefixes;
	}
	if (default_seg_reg == Register::None)
		default_seg_reg = get_default_segment_register(instruction);
	if (prefix_seg != default_seg_reg)
		return true;
	return show_useless_prefixes;
}

} // namespace iced_x86::internal
