// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/nasm/mem_size_tbl.rs

#include "internal/formatter/nasm/mem_size_tbl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/iced_constants.hpp"
#include "internal/encoder/const_init.hpp"
#include "internal/formatter/fmt_consts.hpp"
#include "internal/formatter/nasm/mem_size_tbl_data.hpp"

namespace iced_x86::internal::nasm {

static_assert(sizeof(MEM_SIZE_TBL_DATA) / sizeof(MEM_SIZE_TBL_DATA[0]) == IcedConstants::MEMORY_SIZE_ENUM_COUNT, "");
static_assert(sizeof(BCST_TO_DATA) / sizeof(BCST_TO_DATA[0]) ==
				  IcedConstants::MEMORY_SIZE_ENUM_COUNT - static_cast<std::size_t>(IcedConstants::FIRST_BROADCAST_MEMORY_SIZE),
			  "");

namespace {
struct MemSizeTblHolder {
	std::array<MemSizeInfo, IcedConstants::MEMORY_SIZE_ENUM_COUNT> infos;

	constexpr MemSizeTblHolder() noexcept : infos{} {
		const auto& c = FORMATTER_CONSTANTS;
		constexpr auto FIRST_BROADCAST = static_cast<std::size_t>(IcedConstants::FIRST_BROADCAST_MEMORY_SIZE);
		for (std::size_t i = 0; i < infos.size(); i++) {
			auto& info = infos[i];
			info.keyword = &get_memory_keyword(c, MEM_SIZE_TBL_DATA[i]);
			if (i < FIRST_BROADCAST)
				info.bcst_to = &c.empty;
			else
				info.bcst_to = &get_bcst_to_string(c, BCST_TO_DATA[i - FIRST_BROADCAST]);
		}
	}
};
ICED_CONSTINIT const MemSizeTblHolder HOLDER;
} // namespace

const MemSizeInfo* get_mem_size_tbl() noexcept {
	return HOLDER.infos.data();
}

} // namespace iced_x86::internal::nasm
