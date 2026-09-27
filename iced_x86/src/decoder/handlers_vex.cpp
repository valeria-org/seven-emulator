// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// VEX/XOP op code handlers (Rust: decoder/handlers/vex.rs)

#include "internal/decoder/handlers_vex.hpp"

namespace iced_x86::internal {

#define ICED_DEBUG_ASSERT_VEX_XOP() \
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::VEX) || \
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::XOP))

void OpCodeHandler_VectorLength_VEX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VectorLength_VEX>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	const OpCodeHandler* handler = this_.handlers[static_cast<std::size_t>(decoder.state.vector_length)];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_VEX_Simple::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Simple>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VHEv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHEv>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code_w0);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VHEvIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHEvIb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code_w0);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_VW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VW>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg2));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VX_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VX_Ev>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Ev_VX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Ev_VX>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_WV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_WV>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg2));
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VM>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_MV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_MV>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_M::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_M>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_RdRq::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_RdRq>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ != 3)
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_rDI_VX_RX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_rDI_VX_RX>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	if (decoder.state.address_size == OpSize::Size64)
		instruction.set_op0_kind(OpKind::MemorySegRDI);
	else if (decoder.state.address_size == OpSize::Size32)
		instruction.set_op0_kind(OpKind::MemorySegEDI);
	else
		instruction.set_op0_kind(OpKind::MemorySegDI);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VWIb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0)
		instruction.set_code(this_.code_w1);
	else
		instruction.set_code(this_.code_w0);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg1));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg2));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_WVIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_WVIb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg2));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg1));
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_Ed_V_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Ed_V_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	instruction.set_op2_kind(OpKind::Immediate8);
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_VHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHW>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	if (decoder.state.mod_ == 3) {
		instruction.set_code(this_.code_r);
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg3));
	}
	else {
		instruction.set_code(this_.code_m);
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VWH::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VWH>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op2_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_WHV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_WHV>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	instruction.set_code(this_.code_r);
	write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	write_op2_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
}

void OpCodeHandler_VEX_VHM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHM>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_MHV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_MHV>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	write_op2_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHWIb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg3));
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_HRIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_HRIb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VHWIs4::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHWIs4>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	std::size_t b = decoder.read_u8();
	write_op3_reg(instruction, ((static_cast<std::uint32_t>(b) >> 4) & decoder.reg15_mask) + reg_u32(this_.base_reg));
}

void OpCodeHandler_VEX_VHIs4W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHIs4W>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op3_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op3_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	std::size_t b = decoder.read_u8();
	write_op2_reg(instruction, ((static_cast<std::uint32_t>(b) >> 4) & decoder.reg15_mask) + reg_u32(this_.base_reg));
}

void OpCodeHandler_VEX_VHWIs5::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHWIs5>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	std::uint32_t ib = static_cast<std::uint32_t>(decoder.read_u8());
	write_op3_reg(instruction, ((ib >> 4) & decoder.reg15_mask) + reg_u32(this_.base_reg));
	ICED_DEBUG_ASSERT(instruction.op4_kind() == OpKind::Immediate8); // It's hard coded
	InstructionInternal::internal_set_immediate8(instruction, ib & 0xF);
}

void OpCodeHandler_VEX_VHIs5W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VHIs5W>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op3_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op3_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	std::uint32_t ib = static_cast<std::uint32_t>(decoder.read_u8());
	write_op2_reg(instruction, ((ib >> 4) & decoder.reg15_mask) + reg_u32(this_.base_reg));
	ICED_DEBUG_ASSERT(instruction.op4_kind() == OpKind::Immediate8); // It's hard coded
	InstructionInternal::internal_set_immediate8(instruction, ib & 0xF);
}

void OpCodeHandler_VEX_VK_HK_RK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_HK_RK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (decoder.invalid_check_mask != 0 && (decoder.state.vvvv > 7 || decoder.state.extra_register_base != 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VK_RK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_RK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VK_RK_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_RK_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VK_WK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_WK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_M_VK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_M_VK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_VK_R::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_R>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.gpr));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_G_VK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_G_VK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.gpr));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_Gv_W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_W>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code_w0);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Gv_RX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_RX>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_Gv_GPR_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_GPR_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.base_reg));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VX_VSIB_HX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VX_VSIB_HX>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_code(this_.code);
	std::uint32_t reg_num = decoder.state.reg + decoder.state.extra_register_base;
	write_op0_reg(instruction, reg_num + reg_u32(this_.base_reg1));
	write_op2_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg3));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_vsib(instruction, this_.vsib_index, TupleType::N1);
		if (decoder.invalid_check_mask != 0) {
			std::uint32_t index_num = (reg_u32(instruction.memory_index()) - reg_u32(Register::XMM0)) % IcedConstants::VMM_COUNT;
			if (reg_num == index_num || decoder.state.vvvv == index_num || reg_num == decoder.state.vvvv)
				decoder.set_invalid_instruction();
		}
	}
}

void OpCodeHandler_VEX_Gv_Gv_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_Gv_Ev>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	write_op1_reg(instruction, decoder.state.vvvv + gpr);
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Gv_Ev_Gv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_Ev_Gv>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	write_op2_reg(instruction, decoder.state.vvvv + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Ev_Gv_Gv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Ev_Gv_Gv>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	write_op2_reg(instruction, decoder.state.vvvv + gpr);
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Hv_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Hv_Ev>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.vvvv + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Hv_Ed_Id::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Hv_Ed_Id>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	instruction.set_op2_kind(OpKind::Immediate32);
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.vvvv + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.vvvv + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
}

void OpCodeHandler_VEX_GvM_VX_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_GvM_VX_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(this_.base_reg));
	instruction.set_op2_kind(OpKind::Immediate8);
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_Gv_Ev_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_Ev_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_op2_kind(OpKind::Immediate8);
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_Gv_Ev_Id::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_Ev_Id>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_op2_kind(OpKind::Immediate32);
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
}

void OpCodeHandler_VEX_VT_SIBMEM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VT_SIBMEM>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::TMM0));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_sib(instruction);
	}
}

void OpCodeHandler_VEX_SIBMEM_VT::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_SIBMEM_VT>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::TMM0));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_sib(instruction);
	}
}

void OpCodeHandler_VEX_VT::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VT>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::TMM0));
}

void OpCodeHandler_VEX_VT_RT_HT::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VT_RT_HT>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (decoder.invalid_check_mask != 0 && (decoder.state.vvvv > 7 || decoder.state.extra_register_base != 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::TMM0));
	write_op2_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::TMM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::TMM0));
		if (decoder.invalid_check_mask != 0) {
			if (decoder.state.extra_base_register_base != 0 || decoder.state.reg == decoder.state.vvvv || decoder.state.reg == decoder.state.rm ||
				decoder.state.rm == decoder.state.vvvv)
				decoder.set_invalid_instruction();
		}
	}
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_Gq_HK_RK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gq_HK_RK>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (decoder.invalid_check_mask != 0 && decoder.state.vvvv > 7)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	write_op1_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_VEX_VK_R_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_VK_R_Ib>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (((decoder.state.vvvv_invalid_check | decoder.state.extra_register_base) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(this_.gpr));
	else
		decoder.set_invalid_instruction();
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VEX_K_Jb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_K_Jb>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	decoder.state.flags |= StateFlags::BRANCH_IMM8;
	if (decoder.invalid_check_mask != 0 && decoder.state.vvvv > 7)
		decoder.set_invalid_instruction();
	write_op0_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::K0));
	ICED_DEBUG_ASSERT(decoder.is64b_mode);
	// The modrm byte has the imm8 value
	instruction.set_near_branch64(static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(decoder.state.modrm))) +
								  decoder.current_ip64());
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::NearBranch64);
}

void OpCodeHandler_VEX_K_Jz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_K_Jz>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if (decoder.invalid_check_mask != 0 && decoder.state.vvvv > 7)
		decoder.set_invalid_instruction();
	write_op0_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::K0));
	ICED_DEBUG_ASSERT(decoder.is64b_mode);
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::NearBranch64);
	// The modrm byte has the low 8 bits of imm32
	std::uint32_t imm = decoder.state.modrm | (static_cast<std::uint32_t>(decoder.read_u8()) << 8);
	imm |= static_cast<std::uint32_t>(decoder.read_u16()) << 16;
	instruction.set_near_branch64(static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(imm))) + decoder.current_ip64());
}

void OpCodeHandler_VEX_Gv_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Gv_Ev>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + gpr);
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VEX_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX_Ev>(self_ptr);
	ICED_DEBUG_ASSERT_VEX_XOP();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

#undef ICED_DEBUG_ASSERT_VEX_XOP

} // namespace iced_x86::internal
