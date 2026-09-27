// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/handlers_legacy.hpp"

#include "iced_x86/decoder_options.hpp"

namespace iced_x86::internal {

void OpCodeHandler_VEX2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX2>(self_ptr);
	if (decoder.state.mod_ == 3 || decoder.is64b_mode) {
		decoder.vex2(instruction);
	}
	else {
		const OpCodeHandler* handler = this_.handler_mem;
		handler->decode(handler, decoder, instruction);
	}
}

void OpCodeHandler_VEX3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VEX3>(self_ptr);
	if (decoder.state.mod_ == 3 || decoder.is64b_mode) {
		decoder.vex3(instruction);
	}
	else {
		const OpCodeHandler* handler = this_.handler_mem;
		handler->decode(handler, decoder, instruction);
	}
}

void OpCodeHandler_XOP::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_XOP>(self_ptr);
	if ((decoder.state.modrm & 0x1F) < 8) {
		const OpCodeHandler* handler = this_.handler_reg0;
		handler->decode(handler, decoder, instruction);
	}
	else {
		decoder.xop(instruction);
	}
}

void OpCodeHandler_EVEX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EVEX>(self_ptr);
	if (decoder.state.mod_ == 3 || decoder.is64b_mode) {
		decoder.evex_mvex(instruction);
	}
	else {
		const OpCodeHandler* handler = this_.handler_mem;
		handler->decode(handler, decoder, instruction);
	}
}

void OpCodeHandler_PrefixEsCsSsDs::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PrefixEsCsSsDs>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	if (!decoder.is64b_mode || decoder.state.segment_prio == 0) {
		instruction.set_segment_prefix(this_.seg);
	}

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_PrefixFsGs::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PrefixFsGs>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	instruction.set_segment_prefix(this_.seg);
	decoder.state.segment_prio = 1;

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_Prefix66::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	decoder.state.flags |= StateFlags::HAS66;
	decoder.state.operand_size = decoder.default_inverted_operand_size;
	if (decoder.state.mandatory_prefix == DecoderMandatoryPrefix::PNP) {
		decoder.state.mandatory_prefix = DecoderMandatoryPrefix::P66;
	}

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_Prefix67::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	decoder.state.address_size = decoder.default_inverted_address_size;

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_PrefixF0::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	instruction.set_has_lock_prefix(true);
	decoder.state.flags |= StateFlags::LOCK;

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_PrefixF2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	InstructionInternal::internal_set_has_repne_prefix(instruction);
	decoder.state.mandatory_prefix = DecoderMandatoryPrefix::PF2;

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_PrefixF3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	InstructionInternal::internal_set_has_repe_prefix(instruction);
	decoder.state.mandatory_prefix = DecoderMandatoryPrefix::PF3;

	decoder.reset_rex_prefix_state();
	decoder.call_opcode_handlers_map0_table(instruction);
}

void OpCodeHandler_PrefixREX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PrefixREX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	if (decoder.is64b_mode) {
		decoder.state.flags |= StateFlags::HAS_REX;
		std::uint32_t b = this_.rex;
		if ((b & 8) != 0) {
			decoder.state.flags |= StateFlags::W;
			decoder.state.operand_size = OpSize::Size64;
		}
		else {
			decoder.state.flags &= ~StateFlags::W;
			if ((decoder.state.flags & StateFlags::HAS66) == 0) {
				decoder.state.operand_size = OpSize::Size32;
			}
			else {
				decoder.state.operand_size = OpSize::Size16;
			}
		}
		decoder.state.extra_register_base = (b & 4) << 1;
		decoder.state.extra_index_register_base = (b & 2) << 2;
		decoder.state.extra_base_register_base = (b & 1) << 3;

		decoder.call_opcode_handlers_map0_table(instruction);
	}
	else {
		const OpCodeHandler* handler = this_.handler;
		handler->decode(handler, decoder, instruction);
	}
}

void OpCodeHandler_Reg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(this_.reg);
}

void OpCodeHandler_RegIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RegIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(this_.reg);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_IbReg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_IbReg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_register(this_.reg);
	instruction.set_op0_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_AL_DX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_AL_DX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(Register::AL);
	instruction.set_op1_register(Register::DX);
}

void OpCodeHandler_DX_AL::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_DX_AL>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(Register::DX);
	instruction.set_op1_register(Register::AL);
}

void OpCodeHandler_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Ib3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ib3>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_MandatoryPrefix::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MandatoryPrefix>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.clear_mandatory_prefix(instruction);
	const OpCodeHandler* handler = this_.handlers[static_cast<std::size_t>(decoder.state.mandatory_prefix)];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_MandatoryPrefix3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MandatoryPrefix3>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	const MandatoryPrefix3Info& info = decoder.state.mod_ == 3 ? this_.handlers_reg[static_cast<std::size_t>(decoder.state.mandatory_prefix)]
																: this_.handlers_mem[static_cast<std::size_t>(decoder.state.mandatory_prefix)];
	const OpCodeHandler* handler = info.handler;
	if (info.mandatory_prefix) {
		decoder.clear_mandatory_prefix(instruction);
	}
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_MandatoryPrefix4::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MandatoryPrefix4>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::P66) == 1, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF3) == 2, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF2) == 3, "");
	const OpCodeHandler* handler;
	switch (decoder.state.mandatory_prefix) {
	case DecoderMandatoryPrefix::PNP:
		handler = this_.handler_np;
		break;
	case DecoderMandatoryPrefix::P66:
		handler = this_.handler_66;
		break;
	case DecoderMandatoryPrefix::PF3:
		if ((this_.flags & 4) != 0) {
			decoder.clear_mandatory_prefix_f3(instruction);
		}
		handler = this_.handler_f3;
		break;
	// The compiler generates worse code (indirect branch) unless I use `_` here
	default:
		ICED_DEBUG_ASSERT(decoder.state.mandatory_prefix == DecoderMandatoryPrefix::PF2);
		if ((this_.flags & 8) != 0) {
			decoder.clear_mandatory_prefix_f2(instruction);
		}
		handler = this_.handler_f2;
		break;
	}
	if (handler->has_modrm && (this_.flags & 0x10) != 0) {
		decoder.read_modrm();
	}
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_NIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_NIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Reservednop::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reservednop>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	const OpCodeHandler* handler =
		(decoder.options & DecoderOptions::FORCE_RESERVED_NOP) != 0 ? this_.reserved_nop_handler : this_.other_handler;
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Ev_Iz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Iz>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ < 3) {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	if (operand_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::Immediate32);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else if (operand_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::Immediate32to64);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else {
		instruction.set_op1_kind(OpKind::Immediate16);
		InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_Ev_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	instruction.set_op1_kind(this_.op_kinds[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Ev_Ib2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Ib2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op1_kind(OpKind::Immediate8);
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Ev_1::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_1>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, 1);
	decoder.state.flags |= StateFlags::NO_IMM;
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_CL::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_CL>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	instruction.set_op1_register(Register::CL);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Rv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Rv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
}

void OpCodeHandler_Rv_32_64::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Rv_32_64>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	}
}

void OpCodeHandler_Rq::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Rq>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
}

void OpCodeHandler_Ev_REXW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_REXW>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	static_assert(StateFlags::HAS66 != 4, "");
	if ((((this_.flags & 4) | (decoder.state.flags & StateFlags::HAS66)) & decoder.invalid_check_mask) == (4 | StateFlags::HAS66)) {
		decoder.set_invalid_instruction();
	}
	if (decoder.state.mod_ == 3) {
		if ((decoder.state.flags & StateFlags::W) != 0) {
			write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
		}
		else {
			write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
		}
		if ((this_.disallow_reg & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		if ((this_.disallow_mem & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Evj::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Evj>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
		if (decoder.state.mod_ < 3) {
			instruction.set_op0_kind(OpKind::Memory);
			decoder.read_op_mem(instruction);
		}
		else {
			if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
				 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
				write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
			}
			else {
				write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::AX));
			}
		}
	}
	else {
		OpSize operand_size = decoder.state.operand_size;
		if (operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
		if (decoder.state.mod_ < 3) {
			instruction.set_op0_kind(OpKind::Memory);
			decoder.read_op_mem(instruction);
		}
		else {
			std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
			write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
		}
	}
}

void OpCodeHandler_Ep::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ep>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size == OpSize::Size64 && (decoder.options & DecoderOptions::AMD) == 0) {
		instruction.set_code(this_.code64);
	}
	else if (decoder.state.operand_size == OpSize::Size16) {
		instruction.set_code(this_.code16);
	}
	else {
		instruction.set_code(this_.code32);
	}
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Evw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Evw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ew::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ew>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ms::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ms>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.mod_ < 3) {
		if (decoder.is64b_mode) {
			instruction.set_code(this_.code64);
		}
		else if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Gv_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
}

void OpCodeHandler_Gd_Rd::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gd_Rd>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Gv_M_as::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_M_as>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize address_size = decoder.state.address_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(address_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(address_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gdq_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gdq_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (operand_size != OpSize::Size64) {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	else {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Ev3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev3>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Ev2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if (operand_size != OpSize::Size16) {
			write_op1_reg(instruction, index + reg_u32(Register::EAX));
		}
		else {
			write_op1_reg(instruction, index + reg_u32(Register::AX));
		}
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_R_C::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_R_C>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	}
	std::uint32_t extra_register_base = decoder.state.extra_register_base;
	// LOCK MOV CR0 is supported by some AMD CPUs
	if (this_.base_reg == Register::CR0 && instruction.has_lock_prefix() && (decoder.options & DecoderOptions::AMD) != 0) {
		if ((extra_register_base & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
		extra_register_base = 8;
		instruction.set_has_lock_prefix(false);
		decoder.state.flags &= ~StateFlags::LOCK;
	}
	std::uint32_t reg = decoder.state.reg + extra_register_base;
	if (decoder.invalid_check_mask != 0) {
		if (this_.base_reg == Register::CR0) {
			if (reg == 1 || (reg != 8 && reg >= 5)) {
				decoder.set_invalid_instruction();
			}
		}
		else if (this_.base_reg == Register::DR0) {
			if (reg > 7) {
				decoder.set_invalid_instruction();
			}
		}
		else {
			ICED_DEBUG_ASSERT(!decoder.is64b_mode);
			ICED_DEBUG_ASSERT(this_.base_reg == Register::TR0);
		}
	}
	write_op1_reg(instruction, reg + reg_u32(this_.base_reg));
}

void OpCodeHandler_C_R::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_C_R>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
	}
	std::uint32_t extra_register_base = decoder.state.extra_register_base;
	// LOCK MOV CR0 is supported by some AMD CPUs
	if (this_.base_reg == Register::CR0 && instruction.has_lock_prefix() && (decoder.options & DecoderOptions::AMD) != 0) {
		if ((extra_register_base & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
		extra_register_base = 8;
		instruction.set_has_lock_prefix(false);
		decoder.state.flags &= ~StateFlags::LOCK;
	}
	std::uint32_t reg = decoder.state.reg + extra_register_base;
	if (decoder.invalid_check_mask != 0) {
		if (this_.base_reg == Register::CR0) {
			if (reg == 1 || (reg != 8 && reg >= 5)) {
				decoder.set_invalid_instruction();
			}
		}
		else if (this_.base_reg == Register::DR0) {
			if (reg > 7) {
				decoder.set_invalid_instruction();
			}
		}
		else {
			ICED_DEBUG_ASSERT(!decoder.is64b_mode);
			ICED_DEBUG_ASSERT(this_.base_reg == Register::TR0);
		}
	}
	write_op0_reg(instruction, reg + reg_u32(this_.base_reg));
}

void OpCodeHandler_Jb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Jb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.state.flags |= StateFlags::BRANCH_IMM8;
	std::uint64_t b = static_cast<std::uint64_t>(static_cast<std::int8_t>(decoder.read_u8()));
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_near_branch64(b + decoder.current_ip64());
			instruction.set_code(this_.code64);
			instruction.set_op0_kind(OpKind::NearBranch64);
		}
		else {
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(b) + decoder.current_ip32())));
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch16);
		}
	}
	else {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_near_branch32(static_cast<std::uint32_t>(b) + decoder.current_ip32());
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::NearBranch32);
		}
		else {
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(b) + decoder.current_ip32())));
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch16);
		}
	}
}

void OpCodeHandler_Jx::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Jx>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.state.flags |= StateFlags::XBEGIN;
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::NearBranch64);
			std::uint64_t imm = static_cast<std::uint64_t>(static_cast<std::int32_t>(decoder.read_u32()));
			instruction.set_near_branch64(imm + decoder.current_ip64());
		}
		else if (decoder.state.operand_size == OpSize::Size64) {
			instruction.set_code(this_.code64);
			instruction.set_op0_kind(OpKind::NearBranch64);
			std::uint64_t imm = static_cast<std::uint64_t>(static_cast<std::int32_t>(decoder.read_u32()));
			instruction.set_near_branch64(imm + decoder.current_ip64());
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch64);
			std::uint64_t imm = static_cast<std::uint64_t>(static_cast<std::int16_t>(decoder.read_u16()));
			instruction.set_near_branch64(imm + decoder.current_ip64());
		}
	}
	else {
		ICED_DEBUG_ASSERT(decoder.default_code_size == CodeSize::Code16 || decoder.default_code_size == CodeSize::Code32);
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::NearBranch32);
			std::uint32_t imm = static_cast<std::uint32_t>(decoder.read_u32());
			instruction.set_near_branch32(imm + decoder.current_ip32());
		}
		else {
			ICED_DEBUG_ASSERT(decoder.state.operand_size == OpSize::Size16);
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch32);
			std::uint32_t imm = static_cast<std::uint32_t>(static_cast<std::int16_t>(decoder.read_u16()));
			instruction.set_near_branch32(imm + decoder.current_ip32());
		}
	}
}

void OpCodeHandler_Jz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Jz>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_code(this_.code64);
			instruction.set_op0_kind(OpKind::NearBranch64);
			std::size_t imm;
			if (decoder.try_read_u32(imm)) {
				instruction.set_near_branch64(static_cast<std::uint64_t>(static_cast<std::int32_t>(imm)) + decoder.current_ip64());
				return;
			}
			decoder.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch16);
			std::uint32_t imm = static_cast<std::uint32_t>(decoder.read_u16());
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(imm + decoder.current_ip32())));
		}
	}
	else {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::NearBranch32);
			std::uint32_t imm = static_cast<std::uint32_t>(decoder.read_u32());
			instruction.set_near_branch32(imm + decoder.current_ip32());
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::NearBranch16);
			std::uint32_t imm = static_cast<std::uint32_t>(decoder.read_u16());
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(imm + decoder.current_ip32())));
		}
	}
}

void OpCodeHandler_Jb2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Jb2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.state.flags |= StateFlags::BRANCH_IMM8;
	std::size_t b = decoder.read_u8();
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_near_branch64(static_cast<std::uint64_t>(static_cast<std::int8_t>(b)) + decoder.current_ip64());
			instruction.set_op0_kind(OpKind::NearBranch64);
			if (decoder.state.address_size == OpSize::Size64) {
				instruction.set_code(this_.code64_64);
			}
			else {
				instruction.set_code(this_.code64_32);
			}
		}
		else {
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(static_cast<std::int8_t>(b)) + decoder.current_ip32())));
			instruction.set_op0_kind(OpKind::NearBranch16);
			if (decoder.state.address_size == OpSize::Size64) {
				instruction.set_code(this_.code16_64);
			}
			else {
				instruction.set_code(this_.code16_32);
			}
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_near_branch32(static_cast<std::uint32_t>(static_cast<std::int8_t>(b)) + decoder.current_ip32());
			instruction.set_op0_kind(OpKind::NearBranch32);
			if (decoder.state.address_size == OpSize::Size32) {
				instruction.set_code(this_.code32_32);
			}
			else {
				instruction.set_code(this_.code32_16);
			}
		}
		else {
			InstructionInternal::internal_set_near_branch16(instruction,
				static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(static_cast<std::int8_t>(b)) + decoder.current_ip32())));
			instruction.set_op0_kind(OpKind::NearBranch16);
			if (decoder.state.address_size == OpSize::Size32) {
				instruction.set_code(this_.code16_32);
			}
			else {
				instruction.set_code(this_.code16_16);
			}
		}
	}
}

void OpCodeHandler_Jdisp::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Jdisp>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	ICED_DEBUG_ASSERT(!decoder.is64b_mode);
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op0_kind(OpKind::NearBranch32);
		instruction.set_near_branch32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op0_kind(OpKind::NearBranch16);
		InstructionInternal::internal_set_near_branch16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_PushOpSizeReg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushOpSizeReg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_register(this_.reg);
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_PushEv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushEv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if (decoder.is64b_mode) {
			if (decoder.state.operand_size != OpSize::Size16) {
				write_op0_reg(instruction, index + reg_u32(Register::RAX));
			}
			else {
				write_op0_reg(instruction, index + reg_u32(Register::AX));
			}
		}
		else {
			if (decoder.state.operand_size == OpSize::Size32) {
				write_op0_reg(instruction, index + reg_u32(Register::EAX));
			}
			else {
				write_op0_reg(instruction, index + reg_u32(Register::AX));
			}
		}
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_Gv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op1_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_Gv_flags::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv_flags>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op1_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_Gv_32_64::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv_32_64>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register base_reg;
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		base_reg = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		base_reg = Register::EAX;
	}
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(base_reg));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(base_reg));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_Gv_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op1_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Ev_Gv_CL::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv_CL>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op1_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	instruction.set_op2_register(Register::CL);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Mp::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Mp>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size == OpSize::Size64 && (decoder.options & DecoderOptions::AMD) == 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else if (decoder.state.operand_size == OpSize::Size16) {
		instruction.set_code(this_.code16);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::AX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Eb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Eb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op1_reg(instruction, index + reg_u32(Register::AL));
	}
}

void OpCodeHandler_Gv_Ew::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ew>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::AX));
	}
}

void OpCodeHandler_PushSimple2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushSimple2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_Simple2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
}

void OpCodeHandler_Simple2Iw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple2Iw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_kind(OpKind::Immediate16);
	InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
}

void OpCodeHandler_Simple3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple3>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_Simple5::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple5>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.address_size)]);
}

void OpCodeHandler_Simple5_a32::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple5_a32>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.address_size != OpSize::Size32 && decoder.invalid_check_mask != 0) {
		decoder.set_invalid_instruction();
	}
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.address_size)]);
}

void OpCodeHandler_Simple5_ModRM_as::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple5_ModRM_as>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize address_size = decoder.state.address_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(address_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(address_size)];
	write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
}

void OpCodeHandler_Simple4::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple4>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
}

void OpCodeHandler_PushSimpleReg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushSimpleReg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
			write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
		}
		else {
			instruction.set_code(this_.code16);
			write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::AX));
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
			write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
		}
		else {
			instruction.set_code(this_.code16);
			write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::AX));
		}
	}
}

void OpCodeHandler_SimpleReg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_SimpleReg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	static_assert(static_cast<std::uint32_t>(OpSize::Size16) == 0, "");
	static_assert(static_cast<std::uint32_t>(OpSize::Size32) == 1, "");
	static_assert(static_cast<std::uint32_t>(OpSize::Size64) == 2, "");
	std::uint32_t size_index = static_cast<std::uint32_t>(decoder.state.operand_size);

	// SAFETY: this.code + {0,1,2} is a valid Code value, see ctor
	instruction.set_code(static_cast<Code>(size_index + static_cast<std::uint32_t>(this_.code)));
	static_assert(reg_u32(Register::AX) + 16 == reg_u32(Register::EAX), "");
	static_assert(reg_u32(Register::AX) + 32 == reg_u32(Register::RAX), "");
	write_op0_reg(instruction, size_index * 16 + this_.index + decoder.state.extra_base_register_base + reg_u32(Register::AX));
}

static constexpr Code XCHG_REG_RAX_CODES[3 * 16] = {
	Code::Nopw,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Xchg_r16_AX,
	Code::Nopd,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Xchg_r32_EAX,
	Code::Nopq,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
	Code::Xchg_r64_RAX,
};

void OpCodeHandler_Xchg_Reg_rAX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Xchg_Reg_rAX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));

	if (this_.index == 0 && decoder.state.mandatory_prefix == DecoderMandatoryPrefix::PF3 && (decoder.options & DecoderOptions::NO_PAUSE) == 0) {
		decoder.clear_mandatory_prefix_f3(instruction);
		instruction.set_code(Code::Pause);
	}
	else {
		static_assert(static_cast<std::uint32_t>(OpSize::Size16) == 0, "");
		static_assert(static_cast<std::uint32_t>(OpSize::Size32) == 1, "");
		static_assert(static_cast<std::uint32_t>(OpSize::Size64) == 2, "");
		std::uint32_t size_index = static_cast<std::uint32_t>(decoder.state.operand_size);
		std::uint32_t code_index = this_.index + decoder.state.extra_base_register_base;

		ICED_DEBUG_ASSERT(size_index * 16 + code_index < sizeof(XCHG_REG_RAX_CODES) / sizeof(XCHG_REG_RAX_CODES[0]));
		instruction.set_code(XCHG_REG_RAX_CODES[static_cast<std::size_t>(size_index * 16 + code_index)]);
		if (code_index != 0) {
			static_assert(reg_u32(Register::AX) + 16 == reg_u32(Register::EAX), "");
			static_assert(reg_u32(Register::AX) + 32 == reg_u32(Register::RAX), "");
			std::uint32_t reg = size_index * 16 + code_index + reg_u32(Register::AX);
			write_op0_reg(instruction, reg);
			write_op1_reg(instruction, size_index * 16 + reg_u32(Register::AX));
		}
	}
}

void OpCodeHandler_Reg_Iz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Iz>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	if (operand_size == OpSize::Size32) {
		instruction.set_op0_register(Register::EAX);
		instruction.set_op1_kind(OpKind::Immediate32);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else if (operand_size == OpSize::Size64) {
		instruction.set_op0_register(Register::RAX);
		instruction.set_op1_kind(OpKind::Immediate32to64);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else {
		instruction.set_op0_register(Register::AX);
		instruction.set_op1_kind(OpKind::Immediate16);
		InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
}

static constexpr Register WITH_REX_PREFIX_MOV_REGISTERS[16] = {
	Register::AL,
	Register::CL,
	Register::DL,
	Register::BL,
	Register::SPL,
	Register::BPL,
	Register::SIL,
	Register::DIL,
	Register::R8L,
	Register::R9L,
	Register::R10L,
	Register::R11L,
	Register::R12L,
	Register::R13L,
	Register::R14L,
	Register::R15L,
};

void OpCodeHandler_RegIb3::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RegIb3>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(Code::Mov_r8_imm8);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if ((decoder.state.flags & StateFlags::HAS_REX) != 0) {
		// SAFETY: index <= 7 (see ctor) and extra_base_register_base == 0 or 8 so index = 0-15
		std::uint32_t register_ =
			reg_u32(WITH_REX_PREFIX_MOV_REGISTERS[static_cast<std::size_t>(this_.index + decoder.state.extra_base_register_base)]);
		write_op0_reg(instruction, register_);
	}
	else {
		write_op0_reg(instruction, this_.index + reg_u32(Register::AL));
	}
}

void OpCodeHandler_RegIz2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RegIz2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_code(Code::Mov_r32_imm32);
		write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
		instruction.set_op1_kind(OpKind::Immediate32);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_code(Code::Mov_r64_imm64);
		write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
		instruction.set_op1_kind(OpKind::Immediate64);
		std::uint64_t q = decoder.read_u64();
		InstructionInternal::internal_set_immediate64_lo(instruction, static_cast<std::uint32_t>(q));
		InstructionInternal::internal_set_immediate64_hi(instruction, static_cast<std::uint32_t>(q >> 32));
	}
	else {
		instruction.set_code(Code::Mov_r16_imm16);
		write_op0_reg(instruction, this_.index + decoder.state.extra_base_register_base + reg_u32(Register::AX));
		instruction.set_op1_kind(OpKind::Immediate16);
		InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_PushIb2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushIb2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
			instruction.set_op0_kind(OpKind::Immediate8to64);
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::Immediate8to16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::Immediate8to32);
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::Immediate8to16);
		}
	}
}

void OpCodeHandler_PushIz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_PushIz>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
			instruction.set_op0_kind(OpKind::Immediate32to64);
			instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::Immediate16);
			InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
			instruction.set_op0_kind(OpKind::Immediate32);
			instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
		}
		else {
			instruction.set_code(this_.code16);
			instruction.set_op0_kind(OpKind::Immediate16);
			InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
		}
	}
}

void OpCodeHandler_Gv_Ma::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ma>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	else {
		instruction.set_code(this_.code16);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::AX));
	}
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_RvMw_Gw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RvMw_Gw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register base_reg;
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
		base_reg = Register::EAX;
	}
	else {
		instruction.set_code(this_.code16);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::AX));
		base_reg = Register::AX;
	}
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(base_reg));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Ev_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (operand_size == OpSize::Size32) {
		instruction.set_op2_kind(OpKind::Immediate8to32);
	}
	else if (operand_size == OpSize::Size64) {
		instruction.set_op2_kind(OpKind::Immediate8to64);
	}
	else {
		instruction.set_op2_kind(OpKind::Immediate8to16);
	}
}

void OpCodeHandler_Gv_Ev_Ib_REX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev_Ib_REX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	ICED_DEBUG_ASSERT(decoder.state.mod_ == 3);
	write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
}

void OpCodeHandler_Gv_Ev_32_64::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev_32_64>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register base_reg;
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		base_reg = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		base_reg = Register::EAX;
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(base_reg));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(base_reg));
		if ((this_.disallow_reg & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		if ((this_.disallow_mem & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Ev_Iz::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev_Iz>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (operand_size == OpSize::Size32) {
		instruction.set_op2_kind(OpKind::Immediate32);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else if (operand_size == OpSize::Size64) {
		instruction.set_op2_kind(OpKind::Immediate32to64);
		instruction.set_immediate32(static_cast<std::uint32_t>(decoder.read_u32()));
	}
	else {
		instruction.set_op2_kind(OpKind::Immediate16);
		InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_Yb_Reg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Yb_Reg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_register(this_.reg);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemoryESDI);
	}
}

void OpCodeHandler_Yv_Reg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Yv_Reg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemoryESDI);
	}
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_op1_register(Register::EAX);
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_op1_register(Register::RAX);
	}
	else {
		instruction.set_op1_register(Register::AX);
	}
}

void OpCodeHandler_Yv_Reg2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Yv_Reg2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op1_register(Register::DX);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemoryESDI);
	}
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
	}
	else {
		instruction.set_code(this_.code16);
	}
}

void OpCodeHandler_Reg_Xb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Xb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(this_.reg);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::MemorySegRSI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::MemorySegESI);
	}
	else {
		instruction.set_op1_kind(OpKind::MemorySegSI);
	}
}

void OpCodeHandler_Reg_Xv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Xv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::MemorySegRSI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::MemorySegESI);
	}
	else {
		instruction.set_op1_kind(OpKind::MemorySegSI);
	}
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_op0_register(Register::EAX);
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_op0_register(Register::RAX);
	}
	else {
		instruction.set_op0_register(Register::AX);
	}
}

void OpCodeHandler_Reg_Xv2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Xv2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_register(Register::DX);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::MemorySegRSI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::MemorySegESI);
	}
	else {
		instruction.set_op1_kind(OpKind::MemorySegSI);
	}
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
	}
	else {
		instruction.set_code(this_.code16);
	}
}

void OpCodeHandler_Reg_Yb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Yb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(this_.reg);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op1_kind(OpKind::MemoryESDI);
	}
}

void OpCodeHandler_Reg_Yv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Yv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op1_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op1_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op1_kind(OpKind::MemoryESDI);
	}
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_op0_register(Register::EAX);
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_op0_register(Register::RAX);
	}
	else {
		instruction.set_op0_register(Register::AX);
	}
}

void OpCodeHandler_Yb_Xb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Yb_Xb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemoryESRDI);
		instruction.set_op1_kind(OpKind::MemorySegRSI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemoryESEDI);
		instruction.set_op1_kind(OpKind::MemorySegESI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemoryESDI);
		instruction.set_op1_kind(OpKind::MemorySegSI);
	}
}

void OpCodeHandler_Yv_Xv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Yv_Xv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemoryESRDI);
		instruction.set_op1_kind(OpKind::MemorySegRSI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemoryESEDI);
		instruction.set_op1_kind(OpKind::MemorySegESI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemoryESDI);
		instruction.set_op1_kind(OpKind::MemorySegSI);
	}
}

void OpCodeHandler_Xb_Yb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Xb_Yb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemorySegRSI);
		instruction.set_op1_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemorySegESI);
		instruction.set_op1_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemorySegSI);
		instruction.set_op1_kind(OpKind::MemoryESDI);
	}
}

void OpCodeHandler_Xv_Yv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Xv_Yv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemorySegRSI);
		instruction.set_op1_kind(OpKind::MemoryESRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemorySegESI);
		instruction.set_op1_kind(OpKind::MemoryESEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemorySegSI);
		instruction.set_op1_kind(OpKind::MemoryESDI);
	}
}

void OpCodeHandler_Ev_Sw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Sw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t seg = decoder.read_op_seg_reg();
	write_op1_reg(instruction, seg);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op0_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_M_Sw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_M_Sw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	std::uint32_t seg = decoder.read_op_seg_reg();
	write_op1_reg(instruction, seg);
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_M::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_M>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Sw_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Sw_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t sreg = decoder.read_op_seg_reg();
	if (decoder.invalid_check_mask != 0 && sreg == reg_u32(Register::CS)) {
		decoder.set_invalid_instruction();
	}
	write_op0_reg(instruction, sreg);
	if (decoder.state.mod_ == 3) {
		std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
		write_op1_reg(instruction, reg_base + decoder.state.rm + decoder.state.extra_base_register_base);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Sw_M::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Sw_M>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	std::uint32_t seg = decoder.read_op_seg_reg();
	write_op0_reg(instruction, seg);
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ap::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ap>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op0_kind(OpKind::FarBranch32);
		instruction.set_far_branch32(static_cast<std::uint32_t>(decoder.read_u32()));
		InstructionInternal::internal_set_far_branch_selector(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op0_kind(OpKind::FarBranch16);
		std::uint32_t d = static_cast<std::uint32_t>(decoder.read_u32());
		InstructionInternal::internal_set_far_branch16(instruction, static_cast<std::uint32_t>(static_cast<std::uint16_t>(d)));
		InstructionInternal::internal_set_far_branch_selector(instruction, d >> 16);
	}
}

void OpCodeHandler_Reg_Ob::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Ob>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op0_register(this_.reg);
	decoder.displ_index = static_cast<std::uint8_t>(decoder.data_ptr);
	//instruction_internal::internal_set_memory_index_scale(instruction, 0);
	//instruction.set_memory_base(Register::None);
	//instruction.set_memory_index(Register::None);
	instruction.set_op1_kind(OpKind::Memory);
	if (decoder.state.address_size == OpSize::Size64) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		decoder.state.flags |= StateFlags::ADDR64;
		instruction.set_memory_displacement64(decoder.read_u64());
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u32()));
	}
	else {
		InstructionInternal::internal_set_memory_displ_size(instruction, 2);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_Ob_Reg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ob_Reg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_register(this_.reg);
	decoder.displ_index = static_cast<std::uint8_t>(decoder.data_ptr);
	//instruction_internal::internal_set_memory_index_scale(instruction, 0);
	//instruction.set_memory_base(Register::None);
	//instruction.set_memory_index(Register::None);
	instruction.set_op0_kind(OpKind::Memory);
	if (decoder.state.address_size == OpSize::Size64) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		decoder.state.flags |= StateFlags::ADDR64;
		instruction.set_memory_displacement64(decoder.read_u64());
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u32()));
	}
	else {
		InstructionInternal::internal_set_memory_displ_size(instruction, 2);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_Reg_Ov::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Ov>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.displ_index = static_cast<std::uint8_t>(decoder.data_ptr);
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_op0_register(Register::EAX);
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_op0_register(Register::RAX);
	}
	else {
		instruction.set_op0_register(Register::AX);
	}
	//instruction_internal::internal_set_memory_index_scale(instruction, 0);
	//instruction.set_memory_base(Register::None);
	//instruction.set_memory_index(Register::None);
	instruction.set_op1_kind(OpKind::Memory);
	if (decoder.state.address_size == OpSize::Size64) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		decoder.state.flags |= StateFlags::ADDR64;
		instruction.set_memory_displacement64(decoder.read_u64());
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u32()));
	}
	else {
		InstructionInternal::internal_set_memory_displ_size(instruction, 2);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_Ov_Reg::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ov_Reg>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	decoder.displ_index = static_cast<std::uint8_t>(decoder.data_ptr);
	instruction.set_code(this_.code[static_cast<std::size_t>(decoder.state.operand_size)]);
	if (decoder.state.operand_size == OpSize::Size32) {
		instruction.set_op1_register(Register::EAX);
	}
	else if (decoder.state.operand_size == OpSize::Size64) {
		instruction.set_op1_register(Register::RAX);
	}
	else {
		instruction.set_op1_register(Register::AX);
	}
	//instruction_internal::internal_set_memory_index_scale(instruction, 0);
	//instruction.set_memory_base(Register::None);
	//instruction.set_memory_index(Register::None);
	instruction.set_op0_kind(OpKind::Memory);
	if (decoder.state.address_size == OpSize::Size64) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		decoder.state.flags |= StateFlags::ADDR64;
		instruction.set_memory_displacement64(decoder.read_u64());
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u32()));
	}
	else {
		InstructionInternal::internal_set_memory_displ_size(instruction, 2);
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(decoder.read_u16()));
	}
}

void OpCodeHandler_BranchIw::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_BranchIw>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_kind(OpKind::Immediate16);
	InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_BranchSimple::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_BranchSimple>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.is64b_mode) {
		if ((((decoder.options ^ DecoderOptions::AMD) & DecoderOptions::AMD) |
			 (static_cast<std::uint32_t>(decoder.state.operand_size) - static_cast<std::uint32_t>(OpSize::Size16))) != 0) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_Iw_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Iw_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_kind(OpKind::Immediate16);
	instruction.set_op1_kind(OpKind::Immediate8_2nd);
	InstructionInternal::internal_set_immediate16(instruction, static_cast<std::uint32_t>(decoder.read_u16()));
	InstructionInternal::internal_set_immediate8_2nd(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.is64b_mode) {
		if (decoder.state.operand_size != OpSize::Size16) {
			instruction.set_code(this_.code64);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
	else {
		if (decoder.state.operand_size == OpSize::Size32) {
			instruction.set_code(this_.code32);
		}
		else {
			instruction.set_code(this_.code16);
		}
	}
}

void OpCodeHandler_Reg_Ib2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Reg_Ib2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op0_register(Register::EAX);
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op0_register(Register::AX);
	}
}

void OpCodeHandler_IbReg2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_IbReg2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op1_register(Register::EAX);
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op1_register(Register::AX);
	}
}

void OpCodeHandler_eAX_DX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_eAX_DX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op1_register(Register::DX);
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op0_register(Register::EAX);
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op0_register(Register::AX);
	}
}

void OpCodeHandler_DX_eAX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_DX_eAX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_op0_register(Register::DX);
	if (decoder.state.operand_size != OpSize::Size16) {
		instruction.set_code(this_.code32);
		instruction.set_op1_register(Register::EAX);
	}
	else {
		instruction.set_code(this_.code16);
		instruction.set_op1_register(Register::AX);
	}
}

void OpCodeHandler_Eb_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Eb_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.mod_ < 3) {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op0_reg(instruction, index + reg_u32(Register::AL));
	}
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Eb_1::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Eb_1>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, 1);
	decoder.state.flags |= StateFlags::NO_IMM;
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op0_reg(instruction, index + reg_u32(Register::AL));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Eb_CL::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Eb_CL>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_register(Register::CL);
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op0_reg(instruction, index + reg_u32(Register::AL));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Eb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Eb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op0_reg(instruction, index + reg_u32(Register::AL));
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Eb_Gb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Eb_Gb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	std::uint32_t index = decoder.state.reg + decoder.state.extra_register_base;
	if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
		index += 4;
	}
	write_op1_reg(instruction, index + reg_u32(Register::AL));
	if (decoder.state.mod_ == 3) {
		index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op0_reg(instruction, index + reg_u32(Register::AL));
	}
	else {
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gb_Eb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gb_Eb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	std::uint32_t index = decoder.state.reg + decoder.state.extra_register_base;
	if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
		index += 4;
	}
	write_op0_reg(instruction, index + reg_u32(Register::AL));

	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op1_reg(instruction, index + reg_u32(Register::AL));
	}
}

void OpCodeHandler_M::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_M>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code_w1);
	}
	else {
		instruction.set_code(this_.code_w0);
	}
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_M_REXW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_M_REXW>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		std::uint32_t flags = (decoder.state.flags & StateFlags::W) != 0 ? this_.flags64 : this_.flags32;
		if ((flags & (HandlerFlags::XACQUIRE | HandlerFlags::XRELEASE)) != 0) {
			decoder.set_xacquire_xrelease(instruction, flags);
		}
		decoder.state.flags |= this_.state_flags_or_value;
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_MemBx::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MemBx>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_memory_index(Register::AL);
	instruction.set_op0_kind(OpKind::Memory);
	//instruction.set_memory_displacement64(0);
	//instruction_internal::internal_set_memory_index_scale(instruction, 0);
	//instruction_internal::internal_set_memory_displ_size(instruction, 0);
	static_assert(reg_u32(Register::BX) + 16 == reg_u32(Register::EBX), "");
	static_assert(reg_u32(Register::BX) + 32 == reg_u32(Register::RBX), "");
	write_base_reg(instruction, static_cast<std::uint32_t>(decoder.state.address_size) * 16 + reg_u32(Register::BX));
}

void OpCodeHandler_VW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VW>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		instruction.set_code(this_.code_r);
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		instruction.set_code(this_.code_m);
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_WV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_WV>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ < 3) {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
}

void OpCodeHandler_rDI_VX_RX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_rDI_VX_RX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemorySegRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemorySegEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemorySegDI);
	}
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_rDI_P_N::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_rDI_P_N>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	if (decoder.state.address_size == OpSize::Size64) {
		instruction.set_op0_kind(OpKind::MemorySegRDI);
	}
	else if (decoder.state.address_size == OpSize::Size32) {
		instruction.set_op0_kind(OpKind::MemorySegEDI);
	}
	else {
		instruction.set_op0_kind(OpKind::MemorySegDI);
	}
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_VM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VM>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_MV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MV>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VQ::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VQ>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_P_Q::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_Q>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Q_P::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Q_P>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_MP::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MP>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_P_Q_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_Q_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_P_W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_W>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_P_R::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_R>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_P_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_P_Ev_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_P_Ev_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Ev_P::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_P>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op1_reg(instruction, decoder.state.reg + reg_u32(Register::MM0));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_W>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code_w1);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code_w0);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_V_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_V_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if (decoder.state.operand_size != OpSize::Size64) {
		instruction.set_code(this_.code_w0);
		gpr = Register::EAX;
	}
	else {
		instruction.set_code(this_.code_w1);
		gpr = Register::RAX;
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VWIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code_w1);
	}
	else {
		instruction.set_code(this_.code_w0);
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VRIbIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VRIbIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	instruction.set_op3_kind(OpKind::Immediate8_2nd);
	std::uint32_t w = static_cast<std::uint32_t>(decoder.read_u16());
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(static_cast<std::uint8_t>(w)));
	InstructionInternal::internal_set_immediate8_2nd(instruction, w >> 8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_RIbIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RIbIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::Immediate8);
	instruction.set_op2_kind(OpKind::Immediate8_2nd);
	std::uint32_t w = static_cast<std::uint32_t>(decoder.read_u16());
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(static_cast<std::uint8_t>(w)));
	InstructionInternal::internal_set_immediate8_2nd(instruction, w >> 8);
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_RIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RIb>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	instruction.set_op1_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Ed_V_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ed_V_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_VX_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VX_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_VX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_VX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_VX_E_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VX_E_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Gv_RX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_RX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	if ((decoder.state.flags & StateFlags::W) != 0) {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::XMM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_B_MIB::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_B_MIB>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.reg > 3 || (decoder.state.extra_register_base & decoder.invalid_check_mask) != 0) {
		decoder.set_invalid_instruction();
	}
	instruction.set_code(this_.code);
	write_op0_reg(instruction, (decoder.state.reg & 3) + reg_u32(Register::BND0));
	if (decoder.state.mod_ < 3) {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_mpx(instruction);
		// It can't be EIP since if it's MPX + 64-bit mode, the address size is always 64-bit
		if (decoder.invalid_check_mask != 0 && instruction.memory_base() == Register::RIP) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_MIB_B::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MIB_B>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.reg > 3 || (decoder.state.extra_register_base & decoder.invalid_check_mask) != 0) {
		decoder.set_invalid_instruction();
	}
	instruction.set_code(this_.code);
	write_op1_reg(instruction, (decoder.state.reg & 3) + reg_u32(Register::BND0));
	if (decoder.state.mod_ < 3) {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_mpx(instruction);
		// It can't be EIP since if it's MPX + 64-bit mode, the address size is always 64-bit
		if (decoder.invalid_check_mask != 0 && instruction.memory_base() == Register::RIP) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_B_BM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_B_BM>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.reg > 3 || (decoder.state.extra_register_base & decoder.invalid_check_mask) != 0) {
		decoder.set_invalid_instruction();
	}
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	write_op0_reg(instruction, (decoder.state.reg & 3) + reg_u32(Register::BND0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, (decoder.state.rm & 3) + reg_u32(Register::BND0));
		if (decoder.state.rm > 3 || (decoder.state.extra_base_register_base & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_mpx(instruction);
	}
}

void OpCodeHandler_BM_B::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_BM_B>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.reg > 3 || (decoder.state.extra_register_base & decoder.invalid_check_mask) != 0) {
		decoder.set_invalid_instruction();
	}
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	write_op1_reg(instruction, (decoder.state.reg & 3) + reg_u32(Register::BND0));
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, (decoder.state.rm & 3) + reg_u32(Register::BND0));
		if (decoder.state.rm > 3 || (decoder.state.extra_base_register_base & decoder.invalid_check_mask) != 0) {
			decoder.set_invalid_instruction();
		}
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem_mpx(instruction);
	}
}

void OpCodeHandler_B_Ev::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_B_Ev>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if (decoder.state.reg > 3 || (decoder.state.extra_register_base & decoder.invalid_check_mask) != 0) {
		decoder.set_invalid_instruction();
	}
	Register base_reg;
	if (decoder.is64b_mode) {
		instruction.set_code(this_.code64);
		base_reg = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		base_reg = Register::EAX;
	}
	write_op0_reg(instruction, (decoder.state.reg & 3) + reg_u32(Register::BND0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(base_reg));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem_mpx(instruction);
		// It can't be EIP since if it's MPX + 64-bit mode, the address size is always 64-bit
		if ((this_.rip_rel_mask & decoder.invalid_check_mask) != 0 && instruction.memory_base() == Register::RIP) {
			decoder.set_invalid_instruction();
		}
	}
}

void OpCodeHandler_Mv_Gv_REXW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Mv_Gv_REXW>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_N_Ib_REX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_N_Ib_REX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	if ((decoder.state.flags & StateFlags::W) != 0) {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	instruction.set_op2_kind(OpKind::Immediate8);
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Gv_N::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_N>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
	}
	else {
		instruction.set_code(this_.code32);
	}
	if ((decoder.state.flags & StateFlags::W) != 0) {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_VN::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_VN>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	instruction.set_code(this_.code);
	write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + reg_u32(Register::MM0));
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_Gv_Mv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Mv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op0_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Mv_Gv::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Mv_Gv>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	OpSize operand_size = decoder.state.operand_size;
	instruction.set_code(this_.code[static_cast<std::size_t>(operand_size)]);
	std::uint32_t reg_base = this_.reg_base[static_cast<std::size_t>(operand_size)];
	write_op1_reg(instruction, reg_base + decoder.state.reg + decoder.state.extra_register_base);
	if (decoder.state.mod_ == 3) {
		decoder.set_invalid_instruction();
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Eb_REX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Eb_REX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		std::uint32_t index = decoder.state.rm + decoder.state.extra_base_register_base;
		if ((decoder.state.flags & StateFlags::HAS_REX) != 0 && index >= 4) {
			index += 4;
		}
		write_op1_reg(instruction, index + reg_u32(Register::AL));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Gv_Ev_REX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Gv_Ev_REX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op0_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ == 3) {
		if ((decoder.state.flags & StateFlags::W) != 0) {
			write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::RAX));
		}
		else {
			write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(Register::EAX));
		}
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
}

void OpCodeHandler_Ev_Gv_REX::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Ev_Gv_REX>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::RAX));
	}
	else {
		instruction.set_code(this_.code32);
		write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::EAX));
	}
	if (decoder.state.mod_ < 3) {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	else {
		decoder.set_invalid_instruction();
	}
}

void OpCodeHandler_GvM_VX_Ib::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_GvM_VX_Ib>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	write_op1_reg(instruction, decoder.state.reg + decoder.state.extra_register_base + reg_u32(Register::XMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	Register gpr;
	if ((decoder.state.flags & StateFlags::W) != 0) {
		instruction.set_code(this_.code64);
		gpr = Register::RAX;
	}
	else {
		instruction.set_code(this_.code32);
		gpr = Register::EAX;
	}
	if (decoder.state.mod_ == 3) {
		write_op0_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base + reg_u32(gpr));
	}
	else {
		instruction.set_op0_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	InstructionInternal::internal_set_immediate8(instruction, static_cast<std::uint32_t>(decoder.read_u8()));
}

void OpCodeHandler_Wbinvd::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	if ((decoder.options & DecoderOptions::NO_WBNOINVD) != 0 || decoder.state.mandatory_prefix != DecoderMandatoryPrefix::PF3) {
		instruction.set_code(Code::Wbinvd);
	}
	else {
		decoder.clear_mandatory_prefix_f3(instruction);
		instruction.set_code(Code::Wbnoinvd);
	}
}
} // namespace iced_x86::internal
