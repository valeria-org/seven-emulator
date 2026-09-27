// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_asm/memory_operand_size.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/rounding_control.hpp"

#include <cstdint>

namespace iced_x86::code_asm {

/// Code assembler operand state: segment override, memory size hint, broadcast, opmask register, `{z}`, `{sae}` and
/// rounding control.
///
/// This type is an implementation detail of the register types and `AsmMemoryOperand`, it's used by the
/// `CodeAssembler` to create the instruction.
class CodeAsmOpState {
	struct Flags1 {
		static constexpr std::uint8_t NONE = 0;
		static constexpr std::uint8_t SEGMENT_SHIFT = 0;
		static constexpr std::uint8_t SEGMENT_MASK = 7;
		static constexpr std::uint8_t SIZE_SHIFT = 3;
		static constexpr std::uint8_t SIZE_MASK = 0xF;
		static constexpr std::uint8_t BROADCAST = 0x80;
	};

	struct Flags2 {
		static constexpr std::uint8_t NONE = 0;
		static constexpr std::uint8_t OP_MASK_SHIFT = 0;
		static constexpr std::uint8_t OP_MASK_MASK = 7;
		static constexpr std::uint8_t ROUNDING_CONTROL_SHIFT = 3;
		static constexpr std::uint8_t ROUNDING_CONTROL_MASK = 7;
		static constexpr std::uint8_t ZEROING_MASKING = 0x40;
		static constexpr std::uint8_t SUPPRESS_ALL_EXCEPTIONS = 0x80;
	};

	static_assert(static_cast<std::uint32_t>(Register::ES) + 1 == static_cast<std::uint32_t>(Register::CS), "");
	static_assert(static_cast<std::uint32_t>(Register::ES) + 2 == static_cast<std::uint32_t>(Register::SS), "");
	static_assert(static_cast<std::uint32_t>(Register::ES) + 3 == static_cast<std::uint32_t>(Register::DS), "");
	static_assert(static_cast<std::uint32_t>(Register::ES) + 4 == static_cast<std::uint32_t>(Register::FS), "");
	static_assert(static_cast<std::uint32_t>(Register::ES) + 5 == static_cast<std::uint32_t>(Register::GS), "");
	static_assert(static_cast<std::uint32_t>(Register::K0) + 7 == static_cast<std::uint32_t>(Register::K7), "");
	static_assert(static_cast<std::uint32_t>(MemoryOperandSize::Zword) <= Flags1::SIZE_MASK, "");
	static_assert(static_cast<std::uint32_t>(RoundingControl::RoundTowardZero) <= Flags2::ROUNDING_CONTROL_MASK, "");

public:
	/// Creates a default state (no segment override, no size hint, no decorators)
	constexpr CodeAsmOpState() noexcept : flags1_(Flags1::NONE), flags2_(Flags2::NONE) {}

	/// `true` if nothing has been set
	[[nodiscard]] constexpr bool is_default() const noexcept { return (flags1_ | flags2_) == 0; }

	/// Merges two states (eg. the destination register's opmask and the source operand's `{sae}`/broadcast).
	///
	/// The segment, size and rounding control are copied from `this` if they're set, else from `other`.
	[[nodiscard]] constexpr CodeAsmOpState merge(CodeAsmOpState other) const noexcept {
		constexpr std::uint8_t FLAGS1_KEEP = Flags1::BROADCAST;
		constexpr std::uint8_t FLAGS2_KEEP = static_cast<std::uint8_t>(
			(Flags2::OP_MASK_MASK << Flags2::OP_MASK_SHIFT) | Flags2::SUPPRESS_ALL_EXCEPTIONS | Flags2::ZEROING_MASKING);
		// If `this` and `other` have both set an opmask register, we'll produce the wrong result but still a valid register
		static_assert(Flags2::OP_MASK_MASK == 7, "");

		std::uint8_t flags1 = static_cast<std::uint8_t>((flags1_ | other.flags1_) & FLAGS1_KEEP);
		std::uint8_t flags2 = static_cast<std::uint8_t>((flags2_ | other.flags2_) & FLAGS2_KEEP);

		// No error if it's set in both `this` and `other`
		constexpr std::uint8_t SEGMENT_BITS = Flags1::SEGMENT_MASK << Flags1::SEGMENT_SHIFT;
		flags1 |= static_cast<std::uint8_t>(((flags1_ & SEGMENT_BITS) != 0 ? flags1_ : other.flags1_) & SEGMENT_BITS);
		constexpr std::uint8_t SIZE_BITS = Flags1::SIZE_MASK << Flags1::SIZE_SHIFT;
		flags1 |= static_cast<std::uint8_t>(((flags1_ & SIZE_BITS) != 0 ? flags1_ : other.flags1_) & SIZE_BITS);
		constexpr std::uint8_t RC_BITS = Flags2::ROUNDING_CONTROL_MASK << Flags2::ROUNDING_CONTROL_SHIFT;
		flags2 |= static_cast<std::uint8_t>(((flags2_ & RC_BITS) != 0 ? flags2_ : other.flags2_) & RC_BITS);

		CodeAsmOpState result;
		result.flags1_ = flags1;
		result.flags2_ = flags2;
		return result;
	}

	/// Gets the memory size hint
	[[nodiscard]] constexpr MemoryOperandSize size() const noexcept {
		return static_cast<MemoryOperandSize>((flags1_ >> Flags1::SIZE_SHIFT) & Flags1::SIZE_MASK);
	}

	/// Sets the memory size hint and clears the broadcast flag
	constexpr void ptr(MemoryOperandSize size) noexcept {
		flags1_ = static_cast<std::uint8_t>((flags1_ & ~((Flags1::SIZE_MASK << Flags1::SIZE_SHIFT) | Flags1::BROADCAST)) |
			(static_cast<std::uint8_t>(size) << Flags1::SIZE_SHIFT));
	}

	/// Sets the memory size hint and the broadcast flag
	constexpr void bcst(MemoryOperandSize size) noexcept {
		flags1_ = static_cast<std::uint8_t>((flags1_ & ~(Flags1::SIZE_MASK << Flags1::SIZE_SHIFT)) |
			(static_cast<std::uint8_t>(size) << Flags1::SIZE_SHIFT) | Flags1::BROADCAST);
	}

	/// Gets the segment override or `Register::None`
	[[nodiscard]] constexpr Register segment() const noexcept {
		// 0 = none, 1 = ES, ..., 6 = GS
		const std::uint32_t number = static_cast<std::uint32_t>((flags1_ >> Flags1::SEGMENT_SHIFT) & Flags1::SEGMENT_MASK) - 1;
		if (number < 6)
			return static_cast<Register>(static_cast<std::uint32_t>(Register::ES) + number);
		return Register::None;
	}

	/// Adds an `ES` segment override
	constexpr void set_es() noexcept { set_segment(1); }
	/// Adds a `CS` segment override
	constexpr void set_cs() noexcept { set_segment(2); }
	/// Adds an `SS` segment override
	constexpr void set_ss() noexcept { set_segment(3); }
	/// Adds a `DS` segment override
	constexpr void set_ds() noexcept { set_segment(4); }
	/// Adds an `FS` segment override
	constexpr void set_fs() noexcept { set_segment(5); }
	/// Adds a `GS` segment override
	constexpr void set_gs() noexcept { set_segment(6); }

	/// Gets the opmask register (`K1`-`K7`) or `Register::None`
	[[nodiscard]] constexpr Register op_mask() const noexcept {
		const std::uint32_t number = static_cast<std::uint32_t>((flags2_ >> Flags2::OP_MASK_SHIFT) & Flags2::OP_MASK_MASK);
		if (number == 0)
			return Register::None;
		return static_cast<Register>(static_cast<std::uint32_t>(Register::K0) + number);
	}

	/// Adds a `{k1}` opmask register
	constexpr void set_k1() noexcept { set_op_mask(1); }
	/// Adds a `{k2}` opmask register
	constexpr void set_k2() noexcept { set_op_mask(2); }
	/// Adds a `{k3}` opmask register
	constexpr void set_k3() noexcept { set_op_mask(3); }
	/// Adds a `{k4}` opmask register
	constexpr void set_k4() noexcept { set_op_mask(4); }
	/// Adds a `{k5}` opmask register
	constexpr void set_k5() noexcept { set_op_mask(5); }
	/// Adds a `{k6}` opmask register
	constexpr void set_k6() noexcept { set_op_mask(6); }
	/// Adds a `{k7}` opmask register
	constexpr void set_k7() noexcept { set_op_mask(7); }

	/// Gets the rounding control
	[[nodiscard]] constexpr RoundingControl rounding_control() const noexcept {
		return static_cast<RoundingControl>((flags2_ >> Flags2::ROUNDING_CONTROL_SHIFT) & Flags2::ROUNDING_CONTROL_MASK);
	}

	/// Round to nearest (even)
	constexpr void rn_sae() noexcept { set_rounding_control(RoundingControl::RoundToNearest); }
	/// Round down (toward -inf)
	constexpr void rd_sae() noexcept { set_rounding_control(RoundingControl::RoundDown); }
	/// Round up (toward +inf)
	constexpr void ru_sae() noexcept { set_rounding_control(RoundingControl::RoundUp); }
	/// Round toward zero (truncate)
	constexpr void rz_sae() noexcept { set_rounding_control(RoundingControl::RoundTowardZero); }

	/// `true` if it's a broadcast memory operand
	[[nodiscard]] constexpr bool is_broadcast() const noexcept { return (flags1_ & Flags1::BROADCAST) != 0; }

	/// `true` if suppress all exceptions (`{sae}`) is enabled
	[[nodiscard]] constexpr bool suppress_all_exceptions() const noexcept { return (flags2_ & Flags2::SUPPRESS_ALL_EXCEPTIONS) != 0; }
	/// Enables suppress all exceptions `{sae}`
	constexpr void set_suppress_all_exceptions() noexcept { flags2_ |= Flags2::SUPPRESS_ALL_EXCEPTIONS; }

	/// `true` if zeroing masking (`{z}`) is enabled
	[[nodiscard]] constexpr bool zeroing_masking() const noexcept { return (flags2_ & Flags2::ZEROING_MASKING) != 0; }
	/// Enables zeroing masking `{z}`
	constexpr void set_zeroing_masking() noexcept { flags2_ |= Flags2::ZEROING_MASKING; }

	constexpr bool operator==(const CodeAsmOpState& other) const noexcept { return flags1_ == other.flags1_ && flags2_ == other.flags2_; }
	constexpr bool operator!=(const CodeAsmOpState& other) const noexcept { return !(*this == other); }

private:
	constexpr void set_segment(std::uint8_t number) noexcept {
		flags1_ = static_cast<std::uint8_t>((flags1_ & ~(Flags1::SEGMENT_MASK << Flags1::SEGMENT_SHIFT)) | (number << Flags1::SEGMENT_SHIFT));
	}

	constexpr void set_op_mask(std::uint8_t number) noexcept {
		flags2_ = static_cast<std::uint8_t>((flags2_ & ~(Flags2::OP_MASK_MASK << Flags2::OP_MASK_SHIFT)) | (number << Flags2::OP_MASK_SHIFT));
	}

	constexpr void set_rounding_control(RoundingControl rc) noexcept {
		flags2_ = static_cast<std::uint8_t>((flags2_ & ~(Flags2::ROUNDING_CONTROL_MASK << Flags2::ROUNDING_CONTROL_SHIFT)) |
			(static_cast<std::uint8_t>(rc) << Flags2::ROUNDING_CONTROL_SHIFT));
	}

	std::uint8_t flags1_; // Flags1
	std::uint8_t flags2_; // Flags2
};

} // namespace iced_x86::code_asm
