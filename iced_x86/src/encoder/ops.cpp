// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/encoder/ops.hpp"
#include "iced_x86/encoder.hpp"
#include "internal/encoder/encoder_flags.hpp"
#include "internal/encoder/encoder_internal.hpp"
#include "internal/iced_assert.hpp"
#include <string>

namespace iced_x86::internal {

using E = EncoderInternal;

void InvalidOpHandler::encode(Encoder&, const Instruction&, std::uint32_t) const { ICED_UNREACHABLE(); }

void OpModRM_rm_mem_only::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (must_use_sib)
		E::encoder_flags(encoder) |= EncoderFlags::MUST_USE_SIB;
	E::add_reg_or_mem(encoder, instruction, operand, Register::None, Register::None, true, false);
}

void OpModRM_rm::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_reg_or_mem(encoder, instruction, operand, reg_lo, reg_hi, true, true);
}

void OpRegEmbed8::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_reg(encoder, instruction, operand, reg_lo, reg_hi);
}

void OpModRM_rm_reg_only::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_reg_or_mem(encoder, instruction, operand, reg_lo, reg_hi, false, true);
}

void OpModRM_reg::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_mod_rm_register(encoder, instruction, operand, reg_lo, reg_hi);
}

void OpModRM_reg_mem::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_mod_rm_register(encoder, instruction, operand, reg_lo, reg_hi);
	E::encoder_flags(encoder) |= EncoderFlags::REG_IS_MEMORY;
}

void OpModRM_regF0::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (encoder.bitness() != 64 && instruction.op_kind(operand) == OpKind::Register &&
		static_cast<std::uint32_t>(instruction.op_register(operand)) >= static_cast<std::uint32_t>(reg_lo) + 8 &&
		static_cast<std::uint32_t>(instruction.op_register(operand)) <= static_cast<std::uint32_t>(reg_lo) + 15) {
		E::encoder_flags(encoder) |= EncoderFlags::PF0;
		// reg_lo is eg. CR0 and CR0 + 15 == CR15, a valid value (CR0-CR15 are consecutive enum values)
		const Register new_reg_lo = static_cast<Register>(static_cast<std::uint32_t>(reg_lo) + 8);
		const Register new_reg_hi = static_cast<Register>(static_cast<std::uint32_t>(reg_lo) + 15);
		E::add_mod_rm_register(encoder, instruction, operand, new_reg_lo, new_reg_hi);
	}
	else
		E::add_mod_rm_register(encoder, instruction, operand, reg_lo, reg_hi);
}

void OpReg::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	(void)E::verify_op_kind(encoder, operand, OpKind::Register, instruction.op_kind(operand));
	(void)E::verify_register(encoder, operand, register_, instruction.op_register(operand));
}

void OpRegSTi::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Register, instruction.op_kind(operand)))
		return;
	const Register reg = instruction.op_register(operand);
	if (!E::verify_register_range(encoder, operand, reg, Register::ST0, Register::ST7))
		return;
	ICED_DEBUG_ASSERT((E::op_code(encoder) & 7) == 0);
	E::op_code(encoder) |= static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::ST0);
}

void OprDI::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	const std::uint32_t reg_size = get_reg_size(instruction.op_kind(operand));
	if (reg_size == 0) {
		E::set_error_message(encoder, "Operand " + std::to_string(operand) +
			": expected OpKind = OpKind::MemorySegDI, OpKind::MemorySegEDI or OpKind::MemorySegRDI");
		return;
	}
	E::set_addr_size(encoder, reg_size);
}

void OpIb::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	switch (E::imm_size(encoder)) {
	case ImmSize::Size1:
		if (!E::verify_op_kind(encoder, operand, OpKind::Immediate8_2nd, instruction.op_kind(operand)))
			return;
		E::imm_size(encoder) = ImmSize::Size1_1;
		E::immediate_hi(encoder) = instruction.immediate8_2nd();
		break;
	case ImmSize::Size2:
		if (!E::verify_op_kind(encoder, operand, OpKind::Immediate8_2nd, instruction.op_kind(operand)))
			return;
		E::imm_size(encoder) = ImmSize::Size2_1;
		E::immediate_hi(encoder) = instruction.immediate8_2nd();
		break;
	default: {
		const OpKind op_imm_kind = instruction.op_kind(operand);
		if (!E::verify_op_kind(encoder, operand, op_kind, op_imm_kind))
			return;
		E::imm_size(encoder) = ImmSize::Size1;
		E::immediate(encoder) = instruction.immediate8();
		break;
	}
	}
}

void OpIw::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Immediate16, instruction.op_kind(operand)))
		return;
	E::imm_size(encoder) = ImmSize::Size2;
	E::immediate(encoder) = instruction.immediate16();
}

void OpId::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	const OpKind op_imm_kind = instruction.op_kind(operand);
	if (!E::verify_op_kind(encoder, operand, op_kind, op_imm_kind))
		return;
	E::imm_size(encoder) = ImmSize::Size4;
	E::immediate(encoder) = instruction.immediate32();
}

void OpIq::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Immediate64, instruction.op_kind(operand)))
		return;
	E::imm_size(encoder) = ImmSize::Size8;
	const std::uint64_t imm = instruction.immediate64();
	E::immediate(encoder) = static_cast<std::uint32_t>(imm);
	E::immediate_hi(encoder) = static_cast<std::uint32_t>(imm >> 32);
}

void OpI4::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	const OpKind op_imm_kind = instruction.op_kind(operand);
	if (!E::verify_op_kind(encoder, operand, OpKind::Immediate8, op_imm_kind))
		return;
	ICED_DEBUG_ASSERT(E::imm_size(encoder) == ImmSize::SizeIbReg);
	ICED_DEBUG_ASSERT((E::immediate(encoder) & 0xF) == 0);
	if (instruction.immediate8() > 0xF) {
		static constexpr char HEX[] = "0123456789ABCDEF";
		const std::uint32_t imm = instruction.immediate8();
		std::string msg = "Operand " + std::to_string(operand) + ": Immediate value must be 0-15, but value is 0x";
		msg.push_back(HEX[(imm >> 4) & 0xF]);
		msg.push_back(HEX[imm & 0xF]);
		E::set_error_message(encoder, std::move(msg));
		return;
	}
	E::imm_size(encoder) = ImmSize::Size1;
	E::immediate(encoder) |= instruction.immediate8();
}

void OpX::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	const std::uint32_t regx_size = get_xreg_size(instruction.op_kind(operand));
	if (regx_size == 0) {
		E::set_error_message(encoder, "Operand " + std::to_string(operand) +
			": expected OpKind = OpKind::MemorySegSI, OpKind::MemorySegESI or OpKind::MemorySegRSI");
		return;
	}
	switch (instruction.code()) {
	case Code::Movsb_m8_m8:
	case Code::Movsw_m16_m16:
	case Code::Movsd_m32_m32:
	case Code::Movsq_m64_m64: {
		const std::uint32_t regy_size = get_yreg_size(instruction.op0_kind());
		if (regx_size != regy_size) {
			E::set_error_message(encoder, "Same sized register must be used: reg #1 size = " + std::to_string(regy_size * 8) +
				", reg #2 size = " + std::to_string(regx_size * 8));
			return;
		}
		break;
	}
	default:
		break;
	}
	E::set_addr_size(encoder, regx_size);
}

void OpY::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	const std::uint32_t regy_size = OpX::get_yreg_size(instruction.op_kind(operand));
	if (regy_size == 0) {
		E::set_error_message(encoder, "Operand " + std::to_string(operand) +
			": expected OpKind = OpKind::MemoryESDI, OpKind::MemoryESEDI or OpKind::MemoryESRDI");
		return;
	}
	switch (instruction.code()) {
	case Code::Cmpsb_m8_m8:
	case Code::Cmpsw_m16_m16:
	case Code::Cmpsd_m32_m32:
	case Code::Cmpsq_m64_m64: {
		const std::uint32_t regx_size = OpX::get_xreg_size(instruction.op0_kind());
		if (regx_size != regy_size) {
			E::set_error_message(encoder, "Same sized register must be used: reg #1 size = " + std::to_string(regx_size * 8) +
				", reg #2 size = " + std::to_string(regy_size * 8));
			return;
		}
		break;
	}
	default:
		break;
	}
	E::set_addr_size(encoder, regy_size);
}

void OpMRBX::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Memory, instruction.op_kind(operand)))
		return;
	const Register base = instruction.memory_base();
	if (instruction.memory_displ_size() != 0 || instruction.memory_displacement64() != 0 || instruction.memory_index_scale() != 1 ||
		instruction.memory_index() != Register::AL || (base != Register::BX && base != Register::EBX && base != Register::RBX)) {
		E::set_error_message(encoder, "Operand " + std::to_string(operand) + ": Operand must be [bx+al], [ebx+al], or [rbx+al]");
		return;
	}
	std::uint32_t reg_size;
	if (base == Register::RBX)
		reg_size = 8;
	else if (base == Register::EBX)
		reg_size = 4;
	else {
		ICED_DEBUG_ASSERT(base == Register::BX);
		reg_size = 2;
	}
	E::set_addr_size(encoder, reg_size);
}

void OpJ::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_branch(encoder, op_kind, imm_size, instruction, operand);
}

void OpJx::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_branch_x(encoder, imm_size, instruction, operand);
}

std::optional<OpKind> OpJx::near_branch_op_kind() const noexcept {
	// xbegin is special and doesn't mask the target IP. We need to know the code size to return the correct value.
	// Instruction::with_xbegin() should be used to create the instruction and this method should never be called.
	ICED_DEBUG_ASSERT(false);
	return std::nullopt;
}

void OpJdisp::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_branch_disp(encoder, displ_size, instruction, operand);
}

void OpA::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::add_far_branch(encoder, instruction, operand, size);
}

void OpO::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const { E::add_abs_mem(encoder, instruction, operand); }

void OpImm::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Immediate8, instruction.op_kind(operand)))
		return;
	if (instruction.immediate8() != value) {
		static constexpr char HEX[] = "0123456789ABCDEF";
		const std::uint32_t actual = instruction.immediate8();
		std::string msg = "Operand " + std::to_string(operand) + ": Expected 0x";
		msg.push_back(HEX[(value >> 4) & 0xF]);
		msg.push_back(HEX[value & 0xF]);
		msg += ", actual: 0x";
		msg.push_back(HEX[(actual >> 4) & 0xF]);
		msg.push_back(HEX[actual & 0xF]);
		E::set_error_message(encoder, std::move(msg));
	}
}

void OpHx::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Register, instruction.op_kind(operand)))
		return;
	const Register reg = instruction.op_register(operand);
	if (!E::verify_register_range(encoder, operand, reg, reg_lo, reg_hi))
		return;
	E::encoder_flags(encoder) |= (static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(reg_lo)) << EncoderFlags::VVVVV_SHIFT;
}

void OpVsib::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	E::encoder_flags(encoder) |= EncoderFlags::MUST_USE_SIB;
	E::add_reg_or_mem_full(encoder, instruction, operand, Register::None, Register::None, vsib_index_reg_lo, vsib_index_reg_hi, true, false);
}

void OpIsX::encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const {
	if (!E::verify_op_kind(encoder, operand, OpKind::Register, instruction.op_kind(operand)))
		return;
	const Register reg = instruction.op_register(operand);
	if (!E::verify_register_range(encoder, operand, reg, reg_lo, reg_hi))
		return;
	E::imm_size(encoder) = ImmSize::SizeIbReg;
	E::immediate(encoder) = (static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(reg_lo)) << 4;
}

} // namespace iced_x86::internal
