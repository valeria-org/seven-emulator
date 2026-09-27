// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/masm/mem_size_tbl.rs

#include "internal/formatter/masm/mem_size_tbl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/iced_constants.hpp"
#include "internal/encoder/const_init.hpp"
#include "internal/formatter/fmt_consts.hpp"
#include "internal/formatter/masm/mem_size_tbl_data.hpp"

namespace iced_x86::internal::masm {

static_assert(sizeof(MEM_SIZE_TBL_DATA) / sizeof(MEM_SIZE_TBL_DATA[0]) == IcedConstants::MEMORY_SIZE_ENUM_COUNT, "");

namespace {
struct MemSizeTblHolder {
	std::array<MemSizeInfo, IcedConstants::MEMORY_SIZE_ENUM_COUNT> infos;

	constexpr MemSizeTblHolder() noexcept : infos{} {
		const auto& ac = ARRAY_CONSTANTS;
		for (std::size_t i = 0; i < infos.size(); i++) {
			const std::uint32_t d = MEM_SIZE_TBL_DATA[i];
			auto& info = infos[i];
			info.keywords = get_memory_keywords(ac, d & MEMORY_KEYWORDS_MASK);
			info.size = SIZES[d >> SIZE_KIND_SHIFT];
			info.is_broadcast = i >= static_cast<std::size_t>(IcedConstants::FIRST_BROADCAST_MEMORY_SIZE);
		}
	}
};
ICED_CONSTINIT const MemSizeTblHolder HOLDER;
} // namespace

const MemSizeInfo* get_mem_size_tbl() noexcept {
	return HOLDER.infos.data();
}

} // namespace iced_x86::internal::masm
