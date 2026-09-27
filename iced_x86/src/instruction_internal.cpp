// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: instruction_internal.rs. Helpers used by the `Instruction::with*()` factory methods.

#include "internal/instruction_internal.hpp"

namespace iced_x86::internal {

namespace {
constexpr const char* INVALID_ADDRESS_SIZE = "Invalid address size";

void set_rep_prefix(Instruction& instruction, RepPrefixKind rep_prefix) noexcept {
	switch (rep_prefix) {
	case RepPrefixKind::None:
		break;
	case RepPrefixKind::Repe:
		InstructionInternal::internal_set_has_repe_prefix(instruction);
		break;
	case RepPrefixKind::Repne:
		InstructionInternal::internal_set_has_repne_prefix(instruction);
		break;
	}
}
} // namespace

Result<Instruction> InstructionInternal::with_string_reg_segrsi(Code code, std::uint32_t address_size, Register register_, Register segment_prefix,
	RepPrefixKind rep_prefix) {
	Instruction instruction;
	instruction.set_code(code);
	set_rep_prefix(instruction, rep_prefix);

	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	//instruction.set_op0_kind(OpKind::Register);
	instruction.set_op0_register(register_);

	switch (address_size) {
	case 64:
		instruction.set_op1_kind(OpKind::MemorySegRSI);
		break;
	case 32:
		instruction.set_op1_kind(OpKind::MemorySegESI);
		break;
	case 16:
		instruction.set_op1_kind(OpKind::MemorySegSI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	instruction.set_segment_prefix(segment_prefix);

	ICED_DEBUG_ASSERT(instruction.op_count() == 2);
	return instruction;
}

Result<Instruction> InstructionInternal::with_string_reg_esrdi(Code code, std::uint32_t address_size, Register register_, RepPrefixKind rep_prefix) {
	Instruction instruction;
	instruction.set_code(code);
	set_rep_prefix(instruction, rep_prefix);

	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	//instruction.set_op0_kind(OpKind::Register);
	instruction.set_op0_register(register_);

	switch (address_size) {
	case 64:
		instruction.set_op1_kind(OpKind::MemoryESRDI);
		break;
	case 32:
		instruction.set_op1_kind(OpKind::MemoryESEDI);
		break;
	case 16:
		instruction.set_op1_kind(OpKind::MemoryESDI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	ICED_DEBUG_ASSERT(instruction.op_count() == 2);
	return instruction;
}

Result<Instruction> InstructionInternal::with_string_esrdi_reg(Code code, std::uint32_t address_size, Register register_, RepPrefixKind rep_prefix) {
	Instruction instruction;
	instruction.set_code(code);
	set_rep_prefix(instruction, rep_prefix);

	switch (address_size) {
	case 64:
		instruction.set_op0_kind(OpKind::MemoryESRDI);
		break;
	case 32:
		instruction.set_op0_kind(OpKind::MemoryESEDI);
		break;
	case 16:
		instruction.set_op0_kind(OpKind::MemoryESDI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	//instruction.set_op1_kind(OpKind::Register);
	instruction.set_op1_register(register_);

	ICED_DEBUG_ASSERT(instruction.op_count() == 2);
	return instruction;
}

Result<Instruction> InstructionInternal::with_string_segrsi_esrdi(Code code, std::uint32_t address_size, Register segment_prefix, RepPrefixKind rep_prefix) {
	Instruction instruction;
	instruction.set_code(code);
	set_rep_prefix(instruction, rep_prefix);

	switch (address_size) {
	case 64:
		instruction.set_op0_kind(OpKind::MemorySegRSI);
		instruction.set_op1_kind(OpKind::MemoryESRDI);
		break;
	case 32:
		instruction.set_op0_kind(OpKind::MemorySegESI);
		instruction.set_op1_kind(OpKind::MemoryESEDI);
		break;
	case 16:
		instruction.set_op0_kind(OpKind::MemorySegSI);
		instruction.set_op1_kind(OpKind::MemoryESDI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	instruction.set_segment_prefix(segment_prefix);

	ICED_DEBUG_ASSERT(instruction.op_count() == 2);
	return instruction;
}

Result<Instruction> InstructionInternal::with_string_esrdi_segrsi(Code code, std::uint32_t address_size, Register segment_prefix, RepPrefixKind rep_prefix) {
	Instruction instruction;
	instruction.set_code(code);
	set_rep_prefix(instruction, rep_prefix);

	switch (address_size) {
	case 64:
		instruction.set_op0_kind(OpKind::MemoryESRDI);
		instruction.set_op1_kind(OpKind::MemorySegRSI);
		break;
	case 32:
		instruction.set_op0_kind(OpKind::MemoryESEDI);
		instruction.set_op1_kind(OpKind::MemorySegESI);
		break;
	case 16:
		instruction.set_op0_kind(OpKind::MemoryESDI);
		instruction.set_op1_kind(OpKind::MemorySegSI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	instruction.set_segment_prefix(segment_prefix);

	ICED_DEBUG_ASSERT(instruction.op_count() == 2);
	return instruction;
}

Result<Instruction> InstructionInternal::with_maskmov(Code code, std::uint32_t address_size, Register register1, Register register2, Register segment_prefix) {
	Instruction instruction;
	instruction.set_code(code);

	switch (address_size) {
	case 64:
		instruction.set_op0_kind(OpKind::MemorySegRDI);
		break;
	case 32:
		instruction.set_op0_kind(OpKind::MemorySegEDI);
		break;
	case 16:
		instruction.set_op0_kind(OpKind::MemorySegDI);
		break;
	default:
		return IcedError(INVALID_ADDRESS_SIZE);
	}

	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	//instruction.set_op1_kind(OpKind::Register);
	instruction.set_op1_register(register1);

	//instruction.set_op2_kind(OpKind::Register);
	instruction.set_op2_register(register2);

	instruction.set_segment_prefix(segment_prefix);

	ICED_DEBUG_ASSERT(instruction.op_count() == 3);
	return instruction;
}

} // namespace iced_x86::internal
