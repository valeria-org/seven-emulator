// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/register.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>

namespace iced_x86 {

/// Memory operand passed to one of `Instruction`'s `with*()` constructor methods
class MemoryOperand {
public:
	/// Segment override or `Register::None`
	Register segment_prefix = Register::None;

	/// Base register or `Register::None`
	Register base = Register::None;

	/// Index register or `Register::None`
	Register index = Register::None;

	/// Index register scale (1, 2, 4, or 8)
	std::uint32_t scale = 0;

	/// Memory displacement
	std::int64_t displacement = 0;

	/// 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	std::uint32_t displ_size = 0;

	/// `true` if it's broadcast memory (EVEX instructions)
	bool is_broadcast = false;

	/// Creates a default memory operand (all fields are 0/`Register::None`/`false`), same as Rust's `MemoryOperand::default()`
	constexpr MemoryOperand() noexcept = default;

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	constexpr MemoryOperand(Register base, Register index, std::uint32_t scale, std::int64_t displacement, std::uint32_t displ_size, bool is_broadcast,
		Register segment_prefix) noexcept
		: segment_prefix(segment_prefix), base(base), index(index), scale(scale), displacement(displacement), displ_size(displ_size),
		  is_broadcast(is_broadcast) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	static constexpr MemoryOperand new_(Register base, Register index, std::uint32_t scale, std::int64_t displacement, std::uint32_t displ_size,
		bool is_broadcast, Register segment_prefix) noexcept {
		return MemoryOperand(base, index, scale, displacement, displ_size, is_broadcast, segment_prefix);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	static constexpr MemoryOperand with_base_index_scale_bcst_seg(Register base, Register index, std::uint32_t scale, bool is_broadcast,
		Register segment_prefix) noexcept {
		return MemoryOperand(base, index, scale, 0, 0, is_broadcast, segment_prefix);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	static constexpr MemoryOperand with_base_displ_size_bcst_seg(Register base, std::int64_t displacement, std::uint32_t displ_size, bool is_broadcast,
		Register segment_prefix) noexcept {
		return MemoryOperand(base, Register::None, 1, displacement, displ_size, is_broadcast, segment_prefix);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	static constexpr MemoryOperand with_index_scale_displ_size_bcst_seg(Register index, std::uint32_t scale, std::int64_t displacement,
		std::uint32_t displ_size, bool is_broadcast, Register segment_prefix) noexcept {
		return MemoryOperand(Register::None, index, scale, displacement, displ_size, is_broadcast, segment_prefix);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `displacement`: Memory displacement
	/// * `is_broadcast`: `true` if it's broadcast memory (EVEX instructions)
	/// * `segment_prefix`: Segment override or `Register::None`
	static constexpr MemoryOperand with_base_displ_bcst_seg(Register base, std::int64_t displacement, bool is_broadcast, Register segment_prefix) noexcept {
		return MemoryOperand(base, Register::None, 1, displacement, 1, is_broadcast, segment_prefix);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	static constexpr MemoryOperand with_base_index_scale_displ_size(Register base, Register index, std::uint32_t scale, std::int64_t displacement,
		std::uint32_t displ_size) noexcept {
		return MemoryOperand(base, index, scale, displacement, displ_size, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	static constexpr MemoryOperand with_base_index_scale(Register base, Register index, std::uint32_t scale) noexcept {
		return MemoryOperand(base, index, scale, 0, 0, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `index`: Index register or `Register::None`
	static constexpr MemoryOperand with_base_index(Register base, Register index) noexcept {
		return MemoryOperand(base, index, 1, 0, 0, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	static constexpr MemoryOperand with_base_displ_size(Register base, std::int64_t displacement, std::uint32_t displ_size) noexcept {
		return MemoryOperand(base, Register::None, 1, displacement, displ_size, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `index`: Index register or `Register::None`
	/// * `scale`: Index register scale (1, 2, 4, or 8)
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 0 (no displ), 1 (16/32/64-bit, but use 2/4/8 if it doesn't fit in a `int8_t`), 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	static constexpr MemoryOperand with_index_scale_displ_size(Register index, std::uint32_t scale, std::int64_t displacement,
		std::uint32_t displ_size) noexcept {
		return MemoryOperand(Register::None, index, scale, displacement, displ_size, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	/// * `displacement`: Memory displacement
	static constexpr MemoryOperand with_base_displ(Register base, std::int64_t displacement) noexcept {
		return MemoryOperand(base, Register::None, 1, displacement, 1, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `base`: Base register or `Register::None`
	static constexpr MemoryOperand with_base(Register base) noexcept {
		return MemoryOperand(base, Register::None, 1, 0, 0, false, Register::None);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `displacement`: Memory displacement
	/// * `displ_size`: 2 (16-bit), 4 (32-bit) or 8 (64-bit)
	static constexpr MemoryOperand with_displ(std::uint64_t displacement, std::uint32_t displ_size) noexcept {
		return MemoryOperand(Register::None, Register::None, 1, static_cast<std::int64_t>(displacement), displ_size, false, Register::None);
	}

	/// Compares all fields
	constexpr bool operator==(const MemoryOperand& other) const noexcept {
		return segment_prefix == other.segment_prefix && base == other.base && index == other.index && scale == other.scale &&
			displacement == other.displacement && displ_size == other.displ_size && is_broadcast == other.is_broadcast;
	}
	/// Compares all fields
	constexpr bool operator!=(const MemoryOperand& other) const noexcept { return !(*this == other); }
};

} // namespace iced_x86

namespace std {
/// Hashes all fields of a `MemoryOperand`
template <>
struct hash<iced_x86::MemoryOperand> {
	std::size_t operator()(const iced_x86::MemoryOperand& op) const noexcept {
		std::size_t h = static_cast<std::size_t>(op.segment_prefix);
		auto combine = [&h](std::size_t v) { h ^= v + static_cast<std::size_t>(0x9E37'79B9'7F4A'7C15ULL) + (h << 6) + (h >> 2); };
		combine(static_cast<std::size_t>(op.base));
		combine(static_cast<std::size_t>(op.index));
		combine(op.scale);
		combine(static_cast<std::size_t>(static_cast<std::uint64_t>(op.displacement)));
		combine(op.displ_size);
		combine(op.is_broadcast ? 1 : 0);
		return h;
	}
};
} // namespace std
