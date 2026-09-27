// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: instruction_internal.rs. Internal code that needs raw access to Instruction's fields uses these functions.

#pragma once

#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/rep_prefix_kind.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instr_flags1.hpp"
#include "internal/instr_scale.hpp"
#include "internal/mvex_instr_flags.hpp"

namespace iced_x86::internal {

extern const std::uint8_t REG_TO_ADDR_SIZE[IcedConstants::REGISTER_ENUM_COUNT];

/// Friend of `Instruction` (Rust: `instruction_internal.rs`)
struct InstructionInternal {
	// Instruction has copies of these constants since it can't include internal headers
	static_assert(Instruction::F1_SEGMENT_PREFIX_MASK == InstrFlags1::SEGMENT_PREFIX_MASK, "");
	static_assert(Instruction::F1_SEGMENT_PREFIX_SHIFT == InstrFlags1::SEGMENT_PREFIX_SHIFT, "");
	static_assert(Instruction::F1_DATA_LENGTH_MASK == InstrFlags1::DATA_LENGTH_MASK, "");
	static_assert(Instruction::F1_DATA_LENGTH_SHIFT == InstrFlags1::DATA_LENGTH_SHIFT, "");
	static_assert(Instruction::F1_ROUNDING_CONTROL_MASK == InstrFlags1::ROUNDING_CONTROL_MASK, "");
	static_assert(Instruction::F1_ROUNDING_CONTROL_SHIFT == InstrFlags1::ROUNDING_CONTROL_SHIFT, "");
	static_assert(Instruction::F1_OP_MASK_MASK == InstrFlags1::OP_MASK_MASK, "");
	static_assert(Instruction::F1_OP_MASK_SHIFT == InstrFlags1::OP_MASK_SHIFT, "");
	static_assert(Instruction::F1_CODE_SIZE_MASK == InstrFlags1::CODE_SIZE_MASK, "");
	static_assert(Instruction::F1_CODE_SIZE_SHIFT == InstrFlags1::CODE_SIZE_SHIFT, "");
	static_assert(Instruction::F1_BROADCAST == InstrFlags1::BROADCAST, "");
	static_assert(Instruction::F1_SUPPRESS_ALL_EXCEPTIONS == InstrFlags1::SUPPRESS_ALL_EXCEPTIONS, "");
	static_assert(Instruction::F1_ZEROING_MASKING == InstrFlags1::ZEROING_MASKING, "");
	static_assert(Instruction::F1_REPE_PREFIX == InstrFlags1::REPE_PREFIX, "");
	static_assert(Instruction::F1_REPNE_PREFIX == InstrFlags1::REPNE_PREFIX, "");
	static_assert(Instruction::F1_LOCK_PREFIX == InstrFlags1::LOCK_PREFIX, "");
	static_assert(Instruction::F1_EQUALS_IGNORE_MASK == InstrFlags1::EQUALS_IGNORE_MASK, "");
	static_assert(Instruction::MVEX_REG_MEM_CONV_SHIFT == MvexInstrFlags::MVEX_REG_MEM_CONV_SHIFT, "");
	static_assert(Instruction::MVEX_REG_MEM_CONV_MASK == MvexInstrFlags::MVEX_REG_MEM_CONV_MASK, "");
	static_assert(Instruction::MVEX_EVICTION_HINT == MvexInstrFlags::EVICTION_HINT, "");
	static_assert(static_cast<std::uint32_t>(InstrScale::Scale1) == 0 && static_cast<std::uint32_t>(InstrScale::Scale8) == 3, "");

	static void internal_set_len(Instruction& self, std::uint32_t len) noexcept {
		ICED_DEBUG_ASSERT(len <= IcedConstants::MAX_INSTRUCTION_LENGTH);
		self.len_ = static_cast<std::uint8_t>(len);
	}

	static void internal_set_code_size(Instruction& self, CodeSize new_value) noexcept {
		self.flags1_ |= static_cast<std::uint32_t>(new_value) << InstrFlags1::CODE_SIZE_SHIFT;
	}

	static bool internal_has_repe_or_repne_prefix(const Instruction& self) noexcept {
		return (self.flags1_ & (InstrFlags1::REPE_PREFIX | InstrFlags1::REPNE_PREFIX)) != 0;
	}

	static void internal_set_has_repe_prefix(Instruction& self) noexcept {
		self.flags1_ = (self.flags1_ & ~InstrFlags1::REPNE_PREFIX) | InstrFlags1::REPE_PREFIX;
	}

	static std::uint32_t internal_has_any_of_lock_rep_repne_prefix(const Instruction& self) noexcept {
		return self.flags1_ & (InstrFlags1::LOCK_PREFIX | InstrFlags1::REPE_PREFIX | InstrFlags1::REPNE_PREFIX);
	}

	static bool internal_has_op_mask_or_zeroing_masking(const Instruction& self) noexcept {
		return (self.flags1_ & ((InstrFlags1::OP_MASK_MASK << InstrFlags1::OP_MASK_SHIFT) | InstrFlags1::ZEROING_MASKING)) != 0;
	}

	static void internal_clear_has_repe_repne_prefix(Instruction& self) noexcept {
		self.flags1_ &= ~(InstrFlags1::REPE_PREFIX | InstrFlags1::REPNE_PREFIX);
	}

	static void internal_set_has_repne_prefix(Instruction& self) noexcept {
		self.flags1_ = (self.flags1_ & ~InstrFlags1::REPE_PREFIX) | InstrFlags1::REPNE_PREFIX;
	}

	static bool internal_op0_is_not_reg_or_op1_is_not_reg(const Instruction& self) noexcept {
		static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
		return (static_cast<std::uint32_t>(self.op_kinds_[0]) | static_cast<std::uint32_t>(self.op_kinds_[1])) != 0;
	}

	static void internal_set_memory_displ_size(Instruction& self, std::uint32_t new_value) noexcept {
		ICED_DEBUG_ASSERT(new_value <= 4);
		self.displ_size_ = static_cast<std::uint8_t>(new_value);
	}

	static std::uint32_t internal_get_memory_index_scale(const Instruction& self) noexcept { return self.scale_; }

	static void internal_set_memory_index_scale(Instruction& self, InstrScale new_value) noexcept { self.scale_ = static_cast<std::uint8_t>(new_value); }

	static void internal_set_immediate8(Instruction& self, std::uint32_t new_value) noexcept { self.immediate_ = new_value; }

	static void internal_set_immediate8_2nd(Instruction& self, std::uint32_t new_value) noexcept { self.mem_displ_ = new_value; }

	static void internal_set_immediate16(Instruction& self, std::uint32_t new_value) noexcept { self.immediate_ = new_value; }

	static void internal_set_immediate64_lo(Instruction& self, std::uint32_t new_value) noexcept { self.immediate_ = new_value; }

	static void internal_set_immediate64_hi(Instruction& self, std::uint32_t new_value) noexcept { self.mem_displ_ = new_value; }

	static void internal_set_near_branch16(Instruction& self, std::uint32_t new_value) noexcept { self.mem_displ_ = new_value; }

	static void internal_set_far_branch16(Instruction& self, std::uint32_t new_value) noexcept { self.immediate_ = new_value; }

	static void internal_set_far_branch_selector(Instruction& self, std::uint32_t new_value) noexcept { self.mem_displ_ = new_value; }

	static std::uint32_t internal_segment_prefix_raw(const Instruction& self) noexcept {
		return ((self.flags1_ >> InstrFlags1::SEGMENT_PREFIX_SHIFT) & InstrFlags1::SEGMENT_PREFIX_MASK) - 1;
	}

	static Register internal_op_register(const Instruction& self, std::uint32_t operand) noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand < 4)
			return self.regs_[operand];
		ICED_DEBUG_ASSERT(operand == 4);
		return self.op4_register();
	}

	static std::uint32_t internal_op_mask(const Instruction& self) noexcept { return (self.flags1_ >> InstrFlags1::OP_MASK_SHIFT) & InstrFlags1::OP_MASK_MASK; }

	static void internal_set_op_mask(Instruction& self, std::uint32_t new_value) noexcept {
		ICED_DEBUG_ASSERT(new_value <= 7);
		self.flags1_ |= new_value << InstrFlags1::OP_MASK_SHIFT;
	}

	static void internal_set_rounding_control(Instruction& self, std::uint32_t new_value) noexcept {
		ICED_DEBUG_ASSERT(new_value < IcedConstants::ROUNDING_CONTROL_ENUM_COUNT);
		self.flags1_ |= new_value << InstrFlags1::ROUNDING_CONTROL_SHIFT;
	}

	static bool internal_has_rounding_control_or_sae(const Instruction& self) noexcept {
		return (self.flags1_ & ((InstrFlags1::ROUNDING_CONTROL_MASK << InstrFlags1::ROUNDING_CONTROL_SHIFT) | InstrFlags1::SUPPRESS_ALL_EXCEPTIONS)) != 0;
	}

	static void internal_set_declare_data_len(Instruction& self, std::uint32_t new_value) noexcept {
		ICED_DEBUG_ASSERT(new_value <= 0x10);
		self.flags1_ |= (new_value - 1) << InstrFlags1::DATA_LENGTH_SHIFT;
	}

	static void internal_set_mvex_reg_mem_conv(Instruction& self, std::uint32_t new_value) noexcept {
		ICED_DEBUG_ASSERT(new_value < IcedConstants::MVEX_REG_MEM_CONV_ENUM_COUNT);
		self.immediate_ |= new_value << MvexInstrFlags::MVEX_REG_MEM_CONV_SHIFT;
	}

	static std::uint32_t get_address_size_in_bytes(Register base_reg, Register index_reg, std::uint32_t displ_size, CodeSize code_size) noexcept {
		const std::uint32_t size =
			static_cast<std::uint32_t>(REG_TO_ADDR_SIZE[static_cast<std::size_t>(base_reg)] | REG_TO_ADDR_SIZE[static_cast<std::size_t>(index_reg)]);
		if (size != 0)
			return size;
		if (displ_size >= 2)
			return displ_size;
		switch (code_size) {
		case CodeSize::Code64:
			return 8;
		case CodeSize::Code32:
			return 4;
		case CodeSize::Code16:
			return 2;
		default:
			return 8;
		}
	}

	// ---- Used by the `Instruction::with*()` methods (src/instruction_internal.cpp) ----

	static Result<void> initialize_signed_immediate(Instruction& instruction, std::size_t operand, std::int64_t immediate);
	static Result<void> initialize_unsigned_immediate(Instruction& instruction, std::size_t operand, std::uint64_t immediate);
	static Result<Instruction> with_string_reg_segrsi(Code code, std::uint32_t address_size, Register register_, Register segment_prefix,
		RepPrefixKind rep_prefix);
	static Result<Instruction> with_string_reg_esrdi(Code code, std::uint32_t address_size, Register register_, RepPrefixKind rep_prefix);
	static Result<Instruction> with_string_esrdi_reg(Code code, std::uint32_t address_size, Register register_, RepPrefixKind rep_prefix);
	static Result<Instruction> with_string_segrsi_esrdi(Code code, std::uint32_t address_size, Register segment_prefix, RepPrefixKind rep_prefix);
	static Result<Instruction> with_string_esrdi_segrsi(Code code, std::uint32_t address_size, Register segment_prefix, RepPrefixKind rep_prefix);
	static Result<Instruction> with_maskmov(Code code, std::uint32_t address_size, Register register1, Register register2, Register segment_prefix);

	// ---- Implemented by the encoder component (they use the encoder's op handlers table) ----

	static Result<OpKind> get_immediate_op_kind(Code code, std::size_t operand);
	static Result<OpKind> get_near_branch_op_kind(Code code, std::size_t operand);
	static Result<OpKind> get_far_branch_op_kind(Code code, std::size_t operand);
};

} // namespace iced_x86::internal
