// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/handlers_fpu.hpp"

namespace iced_x86::internal {

void OpCodeHandler_ST_STi::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_ST_STi>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(Register::ST0);
	write_op1_reg(instruction, reg_u32(Register::ST0) + decoder.state.rm);
}

void OpCodeHandler_STi_ST::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_STi_ST>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, reg_u32(Register::ST0) + decoder.state.rm);
	instruction.set_op1_register(Register::ST0);
}

void OpCodeHandler_STi::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_STi>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, reg_u32(Register::ST0) + decoder.state.rm);
}

void OpCodeHandler_Mf::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Mf>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size != OpSize::Size16)
		instruction.set_code(this_.code32);
	else
		instruction.set_code(this_.code16);
	if (decoder.state.mod_ < 3) {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else
		decoder.set_invalid_instruction();
}

} // namespace iced_x86::internal
