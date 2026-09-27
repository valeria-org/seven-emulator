// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: instruction_internal.rs: initialize_signed_immediate(), initialize_unsigned_immediate().
// They're in their own file since they use get_immediate_op_kind() (implemented by the encoder).

#include "internal/instruction_internal.hpp"

#include <limits>

namespace iced_x86::internal {

namespace {
constexpr const char* NOT_AN_IMMEDIATE_OPERAND = "Not an immediate operand";

// The helpers return an error message or nullptr (instead of a Result<>) to keep the stack frames small

const char* set_signed_immediate(Instruction& instruction, OpKind op_kind, std::int64_t immediate) noexcept {
	switch (op_kind) {
	case OpKind::Immediate8:
		// All i8 and all u8 values can be used
		if (std::numeric_limits<std::int8_t>::min() <= immediate && immediate <= std::numeric_limits<std::uint8_t>::max()) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8_2nd:
		// All i8 and all u8 values can be used
		if (std::numeric_limits<std::int8_t>::min() <= immediate && immediate <= std::numeric_limits<std::uint8_t>::max()) {
			InstructionInternal::internal_set_immediate8_2nd(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8to16:
	case OpKind::Immediate8to32:
	case OpKind::Immediate8to64:
		if (std::numeric_limits<std::int8_t>::min() <= immediate && immediate <= std::numeric_limits<std::int8_t>::max()) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate16:
		// All i16 and all u16 values can be used
		if (std::numeric_limits<std::int16_t>::min() <= immediate && immediate <= std::numeric_limits<std::uint16_t>::max()) {
			InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint16_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate32:
		// All i32 and all u32 values can be used
		if (std::numeric_limits<std::int32_t>::min() <= immediate && immediate <= std::numeric_limits<std::uint32_t>::max()) {
			instruction.set_immediate32(static_cast<std::uint32_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate32to64:
		if (std::numeric_limits<std::int32_t>::min() <= immediate && immediate <= std::numeric_limits<std::int32_t>::max()) {
			instruction.set_immediate32(static_cast<std::uint32_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate64:
		instruction.set_immediate64(static_cast<std::uint64_t>(immediate));
		return nullptr;

	default:
		return NOT_AN_IMMEDIATE_OPERAND;
	}

	return "Invalid signed immediate";
}

const char* set_unsigned_immediate(Instruction& instruction, OpKind op_kind, std::uint64_t immediate) noexcept {
	switch (op_kind) {
	case OpKind::Immediate8:
		if (immediate <= std::numeric_limits<std::uint8_t>::max()) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8_2nd:
		if (immediate <= std::numeric_limits<std::uint8_t>::max()) {
			InstructionInternal::internal_set_immediate8_2nd(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8to16:
		if (immediate <= static_cast<std::uint64_t>(std::numeric_limits<std::int8_t>::max()) || (0xFF80 <= immediate && immediate <= 0xFFFF)) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8to32:
		if (immediate <= static_cast<std::uint64_t>(std::numeric_limits<std::int8_t>::max()) || (0xFFFF'FF80 <= immediate && immediate <= 0xFFFF'FFFF)) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate8to64:
		// Allow 00..7F and FFFF_FFFF_FFFF_FF80..FFFF_FFFF_FFFF_FFFF
		if (immediate + 0x80 <= std::numeric_limits<std::uint8_t>::max()) {
			InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint8_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate16:
		if (immediate <= std::numeric_limits<std::uint16_t>::max()) {
			InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint16_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate32:
		if (immediate <= std::numeric_limits<std::uint32_t>::max()) {
			instruction.set_immediate32(static_cast<std::uint32_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate32to64:
		// Allow 0..7FFF_FFFF and FFFF_FFFF_8000_0000..FFFF_FFFF_FFFF_FFFF
		if (immediate + 0x8000'0000ULL <= std::numeric_limits<std::uint32_t>::max()) {
			instruction.set_immediate32(static_cast<std::uint32_t>(immediate));
			return nullptr;
		}
		break;

	case OpKind::Immediate64:
		instruction.set_immediate64(immediate);
		return nullptr;

	default:
		return NOT_AN_IMMEDIATE_OPERAND;
	}

	return "Invalid unsigned immediate";
}

// Sets the op kind. Returns an error message or nullptr
const char* set_imm_op_kind(Instruction& instruction, std::size_t operand, OpKind op_kind) noexcept {
	if (operand < 4) {
		instruction.set_op_kind(static_cast<std::uint32_t>(operand), op_kind);
		return nullptr;
	}
	if (operand == 4)
		return op_kind == OpKind::Immediate8 ? nullptr : "Invalid opkind";
	return "Invalid operand";
}
} // namespace

Result<void> InstructionInternal::initialize_signed_immediate(Instruction& instruction, std::size_t operand, std::int64_t immediate) {
	const auto op_kind_result = get_immediate_op_kind(instruction.code(), operand);
	if (op_kind_result.is_err())
		return op_kind_result.error();
	const OpKind op_kind = op_kind_result.value();
	const char* error = set_imm_op_kind(instruction, operand, op_kind);
	if (error == nullptr)
		error = set_signed_immediate(instruction, op_kind, immediate);
	if (error != nullptr)
		return IcedError(error);
	return {};
}

Result<void> InstructionInternal::initialize_unsigned_immediate(Instruction& instruction, std::size_t operand, std::uint64_t immediate) {
	const auto op_kind_result = get_immediate_op_kind(instruction.code(), operand);
	if (op_kind_result.is_err())
		return op_kind_result.error();
	const OpKind op_kind = op_kind_result.value();
	const char* error = set_imm_op_kind(instruction, operand, op_kind);
	if (error == nullptr)
		error = set_unsigned_immediate(instruction, op_kind, immediate);
	if (error != nullptr)
		return IcedError(error);
	return {};
}

} // namespace iced_x86::internal
