// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Fast formatter tables (Rust: formatter/fast/{fmt_tbl,pseudo_ops_fast}.rs). Rust creates them at runtime, in C++
// they're constant data: the mnemonics/flags (fmt_data.cpp), register names (regs.cpp) and memory size keywords
// (mem_size_tbl.cpp) are generated, the pseudo ops are created at compile time from `pseudo_ops_defs.hpp`.

#include <cstddef>
#include <cstdint>

#include "iced_x86/internal/fast_fmt.hpp"
#include "internal/encoder/const_init.hpp"
#include "internal/formatter/fast/fast_fmt_flags.hpp"
#include "internal/formatter/fast/fmt_data.hpp"
#include "internal/formatter/pseudo_ops_defs.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal::fast {

// If this fails, the generator was updated and now the constant must be updated too
static_assert(MAX_MNEMONIC_STRING_LEN == MAX_MNEMONIC_LEN, "");
// If this fails, change FastString20 to eg. FastString24 (multiple of 4 or 8 depending on what's best for PERF)
static_assert(MAX_MNEMONIC_LEN <= FastStringMnemonic::SIZE, "");
// If this fails, the generator was updated and now FastStringMnemonic must be changed to the correct type
static_assert(FastStringMnemonic::SIZE == MNEMONIC_VALID_STRING_LENGTH, "");
static_assert(MNEMONICS_SIZE <= 0x1'0000, "MNEMONIC_OFFSETS are u16");
static_assert(FAST_FMT_FLAGS_FORCE_MEM_SIZE == FastFmtFlags::FORCE_MEM_SIZE, "");
static_assert(FAST_FMT_FLAGS_PSEUDO_OPS_KIND_SHIFT == FastFmtFlags::PSEUDO_OPS_KIND_SHIFT, "");

namespace {

using pseudo_ops_defs::PSEUDO_OPS_DEFS;
using pseudo_ops_defs::PSEUDO_OPS_KIND_COUNT;

constexpr std::size_t get_pseudo_ops_count() noexcept {
	std::size_t count = 0;
	for (const auto& def : PSEUDO_OPS_DEFS)
		count += def.size;
	return count;
}

// Size of all pseudo op strings (length byte + chars) + padding
constexpr std::size_t get_pseudo_ops_data_size() noexcept {
	std::size_t size = 0;
	for (const auto& def : PSEUDO_OPS_DEFS) {
		for (std::size_t i = 0; i < def.size; i++)
			size += 1 + def.prefix.size() + def.cc[i].size() + def.suffix.size();
	}
	// It's safe to read FastStringMnemonic::SIZE bytes from the last string
	return size + FastStringMnemonic::SIZE;
}

constexpr std::size_t PSEUDO_OPS_COUNT = get_pseudo_ops_count();
constexpr std::size_t PSEUDO_OPS_DATA_SIZE = get_pseudo_ops_data_size();

// The fast formatter's pseudo op mnemonics (Rust: pseudo_ops_fast.rs)
struct FastPseudoOps {
	/// All strings: a length byte followed by the chars
	std::uint8_t data[PSEUDO_OPS_DATA_SIZE];
	/// Offset of each string in `data`
	std::uint16_t offsets[PSEUDO_OPS_COUNT];
	/// Index of each `PseudoOpsKind`'s first string in `offsets` (`[kind + 1] - [kind]` = number of strings)
	std::uint16_t kind_start[PSEUDO_OPS_KIND_COUNT + 1];
};
static_assert(PSEUDO_OPS_DATA_SIZE <= 0x1'0000 && PSEUDO_OPS_COUNT <= 0xFFFF, "");

constexpr FastPseudoOps create_fast_pseudo_ops() noexcept {
	FastPseudoOps result{};
	std::size_t data_index = 0;
	std::size_t index = 0;
	for (std::size_t kind = 0; kind < PSEUDO_OPS_KIND_COUNT; kind++) {
		result.kind_start[kind] = static_cast<std::uint16_t>(index);
		const auto& def = PSEUDO_OPS_DEFS[kind];
		ICED_ASSERT(static_cast<std::size_t>(def.kind) == kind);
		for (std::size_t i = 0; i < def.size; i++) {
			const auto cc_s = def.cc[i];
			const std::size_t new_len = def.prefix.size() + cc_s.size() + def.suffix.size();
			ICED_ASSERT(new_len <= FastStringMnemonic::SIZE);
			result.offsets[index++] = static_cast<std::uint16_t>(data_index);
			result.data[data_index++] = static_cast<std::uint8_t>(new_len);
			for (const char c : def.prefix)
				result.data[data_index++] = static_cast<std::uint8_t>(c);
			for (const char c : cc_s)
				result.data[data_index++] = static_cast<std::uint8_t>(c);
			for (const char c : def.suffix)
				result.data[data_index++] = static_cast<std::uint8_t>(c);
		}
	}
	result.kind_start[PSEUDO_OPS_KIND_COUNT] = static_cast<std::uint16_t>(index);
	ICED_ASSERT(index == PSEUDO_OPS_COUNT);
	// Padding
	while (data_index < PSEUDO_OPS_DATA_SIZE)
		result.data[data_index++] = static_cast<std::uint8_t>(' ');
	return result;
}

ICED_CONSTINIT const FastPseudoOps FAST_PSEUDO_OPS = create_fast_pseudo_ops();

} // namespace

bool try_get_pseudo_op(Code code, std::uint32_t pseudo_ops_num, std::uint32_t imm8, FastStringMnemonic& mnemonic) noexcept {
	ICED_DEBUG_ASSERT(pseudo_ops_num != 0);
	std::size_t index = imm8;
	// The generator generates only valid values (1-based)
	auto pseudo_ops_kind = static_cast<PseudoOpsKind>(pseudo_ops_num - 1);
	// Not enough bits to store all values so some are mapped to the same value. Fix that here.
	if (pseudo_ops_kind == PseudoOpsKind::vpcmpd6 && code == Code::MVEX_Vpcmpud_kr_k1_zmm_zmmmt_imm8)
		pseudo_ops_kind = PseudoOpsKind::vpcmpud6;
	const auto kind = static_cast<std::size_t>(pseudo_ops_kind);
	ICED_ASSERT(kind < PSEUDO_OPS_KIND_COUNT);
	if (pseudo_ops_kind == PseudoOpsKind::pclmulqdq || pseudo_ops_kind == PseudoOpsKind::vpclmulqdq) {
		if (index <= 1) {
			// nothing
		}
		else if (index == 0x10)
			index = 2;
		else if (index == 0x11)
			index = 3;
		else
			index = static_cast<std::size_t>(-1);
	}
	const std::size_t start = FAST_PSEUDO_OPS.kind_start[kind];
	const std::size_t count = static_cast<std::size_t>(FAST_PSEUDO_OPS.kind_start[kind + 1]) - start;
	if (index < count) {
		mnemonic = FastStringMnemonic{&FAST_PSEUDO_OPS.data[FAST_PSEUDO_OPS.offsets[start + index]]};
		return true;
	}
	return false;
}

} // namespace iced_x86::internal::fast
