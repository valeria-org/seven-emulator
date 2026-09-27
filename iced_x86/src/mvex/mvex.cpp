// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/mvex/mvex.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"

namespace iced_x86::internal {

// Used by Instruction::memory_size()
MemorySize get_mvex_memory_size(Code code, MvexRegMemConv reg_mem_conv) noexcept {
	const MvexInfo& mvex = get_mvex_info(code);
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 1 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast1), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 2 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast4), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 3 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvFloat16), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 4 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint8), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 5 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint8), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 6 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint16), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 7 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint16), "");
	const std::size_t sss = (static_cast<std::uint32_t>(reg_mem_conv) - static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone)) & 7;
	return MVEX_MEMSZ_LUT[static_cast<std::size_t>(mvex.tuple_type_lut_kind) * 8 + sss];
}

} // namespace iced_x86::internal
