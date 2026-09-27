// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// EVEX op code handlers (Rust: decoder/handlers/evex.rs)

#include "internal/decoder/handlers_evex.hpp"

#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/rounding_control.hpp"

namespace iced_x86::internal {

// Used by all handlers that set the rounding control (`vector_length + RoundingControl::RoundToNearest`)
static_assert(static_cast<std::uint32_t>(RoundingControl::None) == 0, "");
static_assert(static_cast<std::uint32_t>(RoundingControl::RoundToNearest) == 1, "");
static_assert(static_cast<std::uint32_t>(RoundingControl::RoundDown) == 2, "");
static_assert(static_cast<std::uint32_t>(RoundingControl::RoundUp) == 3, "");
static_assert(static_cast<std::uint32_t>(RoundingControl::RoundTowardZero) == 4, "");

#define ICED_ASSERT_EVEX() ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::EVEX))

void OpCodeHandler_VectorLength_EVEX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VectorLength_EVEX>(self_ptr);
	ICED_ASSERT_EVEX();
	const OpCodeHandler* handler = this_.handlers[static_cast<std::size_t>(decoder.state.vector_length)];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_VectorLength_EVEX_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VectorLength_EVEX_er>(self_ptr);
	ICED_ASSERT_EVEX();
	VectorLength index = decoder.state.vector_length;
	static_assert(StateFlags::B > 3, "");
	ICED_DEBUG_ASSERT(decoder.state.mod_ <= 3);
	if (((decoder.state.flags & StateFlags::B) | decoder.state.mod_) == (StateFlags::B | 3))
		index = VectorLength::L512;
	const OpCodeHandler* handler = this_.handlers[static_cast<std::size_t>(index)];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_EVEX_V_H_Ev_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_V_H_Ev_er>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	TupleType tuple_type;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		tuple_type = this_.tuple_type_w1;
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code_w0);
		tuple_type = this_.tuple_type_w0;
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			InstructionInternal::internal_set_rounding_control(
				instruction, static_cast<std::uint32_t>(decoder.state.vector_length) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
		}
	}
	else {
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, tuple_type);
	}
}

void OpCodeHandler_EVEX_V_H_Ev_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_V_H_Ev_Ib>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code_w0);
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0)
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type_w1);
		else
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type_w0);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_Ed_V_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_Ed_V_Ib>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
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
		if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0)
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type64);
		else
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type32);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VkHW_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHW_er>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.only_sae)
				instruction.set_suppress_all_exceptions(true);
			else {
				InstructionInternal::internal_set_rounding_control(
					instruction, static_cast<std::uint32_t>(decoder.state.vector_length) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
			}
		}
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkHW_er_ur::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHW_er_ur>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	std::uint32_t reg_num0 = decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex;
	write_op0_reg(instruction, reg_num0 + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_num2 = decoder.state.rm + decoder.state.extra_base_register_base_evex;
		write_op2_reg(instruction, reg_num2 + reg_u32(this_.base_reg));
		if (decoder.invalid_check_mask != 0 && (reg_num0 == decoder.state.vvvv || reg_num0 == reg_num2))
			decoder.set_invalid_instruction();
		if ((decoder.state.flags & StateFlags::B) != 0) {
			InstructionInternal::internal_set_rounding_control(
				instruction, static_cast<std::uint32_t>(decoder.state.vector_length) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
		}
	}
	else {
		if (decoder.invalid_check_mask != 0 && reg_num0 == decoder.state.vvvv)
			decoder.set_invalid_instruction();
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkW_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkW_er>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.only_sae)
				instruction.set_suppress_all_exceptions(true);
			else {
				InstructionInternal::internal_set_rounding_control(
					instruction, static_cast<std::uint32_t>(decoder.state.vector_length) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
			}
		}
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkWIb_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkWIb_er>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_suppress_all_exceptions(true);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_is_broadcast(true);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VkW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkW>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_WkV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_WkV>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::B) | decoder.state.vvvv_invalid_check) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg2));
	if (((decoder.state.flags & StateFlags::Z) & this_.disallow_zeroing_masking & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg1));
	else {
		instruction.set_op0_kind(OpKind::Memory);
		if (((decoder.state.flags & StateFlags::Z) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkM>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::B) | decoder.state.vvvv_invalid_check) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_WkVIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_WkVIb>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::B) | decoder.state.vvvv_invalid_check) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg2));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg1));
	else {
		instruction.set_op0_kind(OpKind::Memory);
		if (((decoder.state.flags & StateFlags::Z) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_HkWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_HkWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg1));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_HWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_HWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);
	if ((((decoder.state.flags & (StateFlags::Z | StateFlags::B)) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();

	write_op0_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg1));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_WkVIb_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_WkVIb_er>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg2));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg1));
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_suppress_all_exceptions(true);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		if (((decoder.state.flags & (StateFlags::B | StateFlags::Z)) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VW_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VW_er>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.vvvv_invalid_check | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_suppress_all_exceptions(true);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VW>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::Z | StateFlags::B)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_WV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_WV>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::Z | StateFlags::B)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg2));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VM>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::Z | StateFlags::B)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VK::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VK>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::K0));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_EVEX_KR::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KR>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa |
		  decoder.state.extra_register_base | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_EVEX_KkHWIb_sae::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KkHWIb_sae>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.extra_register_base | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_suppress_all_exceptions(true);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VkHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHW>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg3));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkHM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHM>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg3));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VkHWIb_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkHWIb_er>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg3));
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_suppress_all_exceptions(true);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_KkHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KkHW>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.extra_register_base | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_KP1HW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KP1HW>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.aaa | decoder.state.extra_register_base | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0)
			instruction.set_is_broadcast(true);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_KkHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KkHWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	instruction.set_op3_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.extra_register_base | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
}

void OpCodeHandler_EVEX_WkHV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_WkHV>(self_ptr);
	ICED_ASSERT_EVEX();
	instruction.set_code(this_.code);

	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	write_op2_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
}

void OpCodeHandler_EVEX_VHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VHWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	instruction.set_op3_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3)
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_VHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VHW>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg1));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg2));
	if (decoder.state.mod_ == 3) {
		instruction.set_code(this_.code_r);
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg3));
	}
	else {
		instruction.set_code(this_.code_m);
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VHM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VHM>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.aaa) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op2_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_Gv_W_er::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_Gv_W_er>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.vvvv_invalid_check | decoder.state.aaa | decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code_w1);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code_w0);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.only_sae)
				instruction.set_suppress_all_exceptions(true);
			else {
				InstructionInternal::internal_set_rounding_control(
					instruction, static_cast<std::uint32_t>(decoder.state.vector_length) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
			}
		}
	}
	else {
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VX_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VX_Ev>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	TupleType tuple_type;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		tuple_type = this_.tuple_type_w1;
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		tuple_type = this_.tuple_type_w0;
		gpr = reg_u32(Register::EAX);
	}
	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, tuple_type);
	}
}

void OpCodeHandler_EVEX_Ev_VX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_Ev_VX>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	TupleType tuple_type;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		tuple_type = this_.tuple_type_w1;
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		tuple_type = this_.tuple_type_w0;
		gpr = reg_u32(Register::EAX);
	}
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3)
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, tuple_type);
	}
}

void OpCodeHandler_EVEX_Ev_VX_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_Ev_VX_Ib>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa |
		  decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
}

void OpCodeHandler_EVEX_MV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_MV>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VkEv_REXW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VkEv_REXW>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::B) | decoder.state.vvvv_invalid_check) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	std::uint32_t gpr;
	if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0) {
		ICED_DEBUG_ASSERT(this_.code64 != Code::INVALID);
		instruction.set_code(this_.code64);
		gpr = reg_u32(Register::RAX);
	}
	else {
		instruction.set_code(this_.code32);
		gpr = reg_u32(Register::EAX);
	}

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + gpr);
	else
		decoder.set_invalid_instruction();
}

void OpCodeHandler_EVEX_Vk_VSIB::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_Vk_VSIB>(self_ptr);
	ICED_ASSERT_EVEX();
	if (decoder.invalid_check_mask != 0 &&
		(((decoder.state.flags & (StateFlags::Z | StateFlags::B)) | (decoder.state.vvvv_invalid_check & 0xF)) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	std::uint32_t reg_num = decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex;
	write_op0_reg(instruction, reg_num + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_vsib(instruction, this_.vsib_base, this_.tuple_type);
		if (decoder.invalid_check_mask != 0) {
			if (reg_num == ((reg_u32(instruction.memory_index()) - reg_u32(Register::XMM0)) % IcedConstants::VMM_COUNT))
				decoder.set_invalid_instruction();
		}
	}
}

void OpCodeHandler_EVEX_VSIB_k1_VX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VSIB_k1_VX>(self_ptr);
	ICED_ASSERT_EVEX();
	if (decoder.invalid_check_mask != 0 &&
		(((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | (decoder.state.vvvv_invalid_check & 0xF)) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_vsib(instruction, this_.vsib_index, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_VSIB_k1::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_VSIB_k1>(self_ptr);
	ICED_ASSERT_EVEX();
	if (decoder.invalid_check_mask != 0 &&
		(((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | (decoder.state.vvvv_invalid_check & 0xF)) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_vsib(instruction, this_.vsib_index, this_.tuple_type);
	}
}

void OpCodeHandler_EVEX_GvM_VX_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_GvM_VX_Ib>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & (StateFlags::B | StateFlags::Z)) | decoder.state.vvvv_invalid_check | decoder.state.aaa) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(this_.base_reg));
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
		if ((decoder.state.flags & decoder.is64b_mode_and_w) != 0)
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type64);
		else
			decoder.read_op_mem_tuple_type(instruction, this_.tuple_type32);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_EVEX_KkWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX_KkWIb>(self_ptr);
	ICED_ASSERT_EVEX();
	if ((((decoder.state.flags & StateFlags::Z) | decoder.state.vvvv_invalid_check | decoder.state.extra_register_base |
		  decoder.state.extra_register_base_evex) &
		 decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(this_.base_reg));
		if (((decoder.state.flags & StateFlags::B) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		if ((decoder.state.flags & StateFlags::B) != 0) {
			if (this_.can_broadcast)
				instruction.set_is_broadcast(true);
			else if (decoder.invalid_check_mask != 0)
				decoder.set_invalid_instruction();
		}
		decoder.read_op_mem_tuple_type(instruction, this_.tuple_type);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

#undef ICED_ASSERT_EVEX

} // namespace iced_x86::internal
