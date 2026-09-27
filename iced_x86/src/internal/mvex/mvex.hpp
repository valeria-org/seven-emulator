// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: mvex/mod.rs, mvex/mvex_info.rs, mvex/mvex_tt_lut.rs, mvex/mvex_memsz_lut.rs

#pragma once

#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/mvex_conv_fn.hpp"
#include "iced_x86/mvex_eh_bit.hpp"
#include "iced_x86/mvex_tuple_type_lut_kind.hpp"
#include "iced_x86/tuple_type.hpp"
#include "internal/iced_assert.hpp"
#include "internal/mvex_info_flags1.hpp"
#include "internal/mvex_info_flags2.hpp"

namespace iced_x86::internal {

struct MvexInfo {
	MvexTupleTypeLutKind tuple_type_lut_kind;
	MvexEHBit eh_bit;
	MvexConvFn conv_fn;
	std::uint8_t invalid_conv_fns;
	std::uint8_t invalid_swizzle_fns;
	std::uint8_t flags1;
	std::uint8_t flags2;
	std::uint8_t pad;

	constexpr MvexInfo(MvexTupleTypeLutKind tuple_type_lut_kind_, MvexEHBit eh_bit_, MvexConvFn conv_fn_, std::uint8_t invalid_conv_fns_,
		std::uint8_t invalid_swizzle_fns_, std::uint8_t flags1_, std::uint8_t flags2_) noexcept
		: tuple_type_lut_kind(tuple_type_lut_kind_), eh_bit(eh_bit_), conv_fn(conv_fn_), invalid_conv_fns(invalid_conv_fns_),
		  invalid_swizzle_fns(invalid_swizzle_fns_), flags1(flags1_), flags2(flags2_), pad(0) {}

	constexpr bool is_ndd() const noexcept { return (flags1 & MvexInfoFlags1::NDD) != 0; }
	constexpr bool is_nds() const noexcept { return (flags1 & MvexInfoFlags1::NDS) != 0; }
	constexpr bool can_use_eviction_hint() const noexcept { return (flags1 & MvexInfoFlags1::EVICTION_HINT) != 0; }
	constexpr bool can_use_imm_rounding_control() const noexcept { return (flags1 & MvexInfoFlags1::IMM_ROUNDING_CONTROL) != 0; }
	constexpr bool can_use_rounding_control() const noexcept { return (flags1 & MvexInfoFlags1::ROUNDING_CONTROL) != 0; }
	constexpr bool can_use_suppress_all_exceptions() const noexcept { return (flags1 & MvexInfoFlags1::SUPPRESS_ALL_EXCEPTIONS) != 0; }
	constexpr bool ignores_op_mask_register() const noexcept { return (flags1 & MvexInfoFlags1::IGNORES_OP_MASK_REGISTER) != 0; }
	constexpr bool require_op_mask_register() const noexcept { return (flags1 & MvexInfoFlags1::REQUIRE_OP_MASK_REGISTER) != 0; }
	constexpr bool no_sae_rc() const noexcept { return (flags2 & MvexInfoFlags2::NO_SAE_ROUNDING_CONTROL) != 0; }
	constexpr bool is_conv_fn_32() const noexcept { return (flags2 & MvexInfoFlags2::CONV_FN32) != 0; }
	constexpr bool ignores_eviction_hint() const noexcept { return (flags2 & MvexInfoFlags2::IGNORES_EVICTION_HINT) != 0; }
};
static_assert(sizeof(MvexInfo) == 8, "");

// Generated: src/mvex/mvex_data.cpp
extern const MvexInfo MVEX_INFO[IcedConstants::MVEX_LENGTH];
// Generated: src/mvex/mvex_tt_lut.cpp. Index = tuple_type_lut_kind * 8 + sss
extern const TupleType MVEX_TUPLE_TYPE_LUT[IcedConstants::MVEX_TUPLE_TYPE_LUT_KIND_ENUM_COUNT * 8];
// Generated: src/mvex/mvex_memsz_lut.cpp. Index = tuple_type_lut_kind * 8 + sss
extern const MemorySize MVEX_MEMSZ_LUT[IcedConstants::MVEX_TUPLE_TYPE_LUT_KIND_ENUM_COUNT * 8];

inline const MvexInfo& get_mvex_info(Code code) noexcept {
	ICED_DEBUG_ASSERT(IcedConstants::is_mvex(code));
	return MVEX_INFO[static_cast<std::size_t>(code) - IcedConstants::MVEX_START];
}

} // namespace iced_x86::internal
