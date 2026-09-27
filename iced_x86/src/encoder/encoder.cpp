// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/encoder.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/memory_size_ext.hpp"
#include "iced_x86/register_ext.hpp"
#include "internal/encoder/enc_flags3.hpp"
#include "internal/encoder/encoder_flags.hpp"
#include "internal/encoder/encoder_internal.hpp"
#include "internal/encoder/imm_sizes.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instruction_internal.hpp"
#include <cstring>
#include <limits>
#include <utility>

namespace iced_x86 {

namespace {
using internal::DisplSize;
using internal::EncFlags3;
using internal::EncoderFlags;
using internal::EncoderInternal;
using internal::ImmSize;

std::string to_hex(std::uint64_t value, int min_digits) {
	char buf[17];
	int len = 0;
	do {
		buf[len++] = "0123456789ABCDEF"[value & 0xF];
		value >>= 4;
	} while (value != 0);
	while (len < min_digits)
		buf[len++] = '0';
	std::string result;
	result.reserve(static_cast<std::size_t>(len));
	while (len > 0)
		result.push_back(buf[--len]);
	return result;
}

#ifndef NDEBUG
std::string op_kind_str(OpKind value) { return to_string(value); }
std::string register_str(Register value) { return to_string(value); }
#endif

// Not inlined to keep the stack frames of the callers small (it's only called if there's an error)
ICED_NOINLINE void set_distance_error(Encoder& e, const char* prefix, std::uint64_t next_ip, int next_ip_digits, std::uint64_t target, int target_digits,
	std::int64_t diff, const char* diff_type) {
	std::string msg(prefix);
	msg += " is too far away: next_ip: 0x";
	msg += to_hex(next_ip, next_ip_digits);
	msg += " target: 0x";
	msg += to_hex(target, target_digits);
	msg += ", diff = ";
	msg += std::to_string(diff);
	msg += ", diff must fit in an ";
	msg += diff_type;
	EncoderInternal::set_error_message(e, std::move(msg));
}

std::string op_str(std::uint32_t operand) { return "Operand " + std::to_string(operand) + ": "; }

// Error messages are built with one append per statement so only one temporary is alive at a time,
// which keeps the stack frames small (every temporary of a long `a + b + c + ...` expression gets its own stack slot).
void append_reg(std::string& msg, Register value) {
#ifndef NDEBUG
	msg += register_str(value);
#else
	msg += "Register value ";
	msg += std::to_string(static_cast<std::uint32_t>(value));
#endif
}

// The following functions aren't inlined to keep the stack frames of the callers small (they're only called if there's an error)

ICED_NOINLINE void set_op_error(Encoder& e, std::uint32_t operand, const char* message) {
	EncoderInternal::set_error_message(e, op_str(operand) + message);
}

ICED_NOINLINE void set_op_error_num(Encoder& e, std::uint32_t operand, const char* message1, std::uint64_t value, const char* message2) {
	EncoderInternal::set_error_message(e, op_str(operand) + message1 + std::to_string(value) + message2);
}

ICED_NOINLINE void set_op_error_hex(Encoder& e, std::uint32_t operand, const char* message1, std::uint64_t value) {
	EncoderInternal::set_error_message(e, op_str(operand) + message1 + to_hex(value, 1));
}

ICED_NOINLINE void set_op_error_op_kind(Encoder& e, std::uint32_t operand, const char* message, OpKind op_kind, bool release_value_prefix) {
#ifndef NDEBUG
	(void)release_value_prefix;
	EncoderInternal::set_error_message(e, op_str(operand) + message + op_kind_str(op_kind));
#else
	EncoderInternal::set_error_message(
		e, op_str(operand) + message + (release_value_prefix ? "OpKind value " : "") + std::to_string(static_cast<std::uint32_t>(op_kind)));
#endif
}

ICED_NOINLINE void set_invalid_reg_size_error(Encoder& e, std::uint32_t reg_size, const char* message) {
	EncoderInternal::set_error_message(e, "Invalid register size: " + std::to_string(reg_size * 8) + message);
}

ICED_NOINLINE void set_invalid_16bit_regs_error(Encoder& e, std::uint32_t operand, Register base, Register index) {
	std::string msg = op_str(operand);
	msg += "Invalid 16-bit base + index registers: base=";
#ifndef NDEBUG
	msg += register_str(base);
	msg += ", index=";
	msg += register_str(index);
#else
	msg += std::to_string(static_cast<std::uint32_t>(base));
	msg += ", index=";
	msg += std::to_string(static_cast<std::uint32_t>(index));
#endif
	EncoderInternal::set_error_message(e, std::move(msg));
}

} // namespace

Encoder::Encoder(PrivateTag, std::uint32_t bitness, std::size_t capacity)
	: current_rip_(0), buffer_(), handler_(&internal::OP_CODE_HANDLERS[0]), error_message_(), bitness_(bitness), eip_(0),
	  displ_addr_(0), imm_addr_(0), immediate_(0), immediate_hi_(0), displ_(0), displ_hi_(0), op_code_(0), internal_vex_wig_lig_(0),
	  internal_vex_lig_(0), internal_evex_wig_(0), internal_evex_lig_(0), internal_mvex_wig_(0), prevent_vex2_(0),
	  opsize16_flags_(bitness != 16 ? EncoderFlags::P66 : 0), opsize32_flags_(bitness == 16 ? EncoderFlags::P66 : 0),
	  adrsize16_flags_(bitness != 16 ? EncoderFlags::P67 : 0), adrsize32_flags_(bitness != 32 ? EncoderFlags::P67 : 0), encoder_flags_(0),
	  displ_size_(DisplSize::None), imm_size_(ImmSize::None), mod_rm_(0), sib_(0) {
	if (capacity != 0)
		buffer_.reserve(capacity);
}

Encoder::Encoder(std::uint32_t bitness) noexcept : Encoder(PrivateTag{}, bitness, 0) {
	ICED_ASSERT(bitness == 16 || bitness == 32 || bitness == 64);
}

Result<Encoder> Encoder::try_new(std::uint32_t bitness) { return try_with_capacity(bitness, 0); }

Result<Encoder> Encoder::try_with_capacity(std::uint32_t bitness, std::size_t capacity) {
	if (bitness != 16 && bitness != 32 && bitness != 64)
		return IcedError("Invalid bitness");
	return Encoder(PrivateTag{}, bitness, capacity);
}

Result<std::size_t> Encoder::encode(const Instruction& instruction, std::uint64_t rip) {
	current_rip_ = rip;
	eip_ = static_cast<std::uint32_t>(rip);

	encoder_flags_ = EncoderFlags::NONE;
	displ_size_ = DisplSize::None;
	imm_size_ = ImmSize::None;
	mod_rm_ = 0;
	// sib doesn't need to be initialized, but the compiler generates better
	// code since it can clear 8 bytes with one instruction, but 7 bytes
	// requires 3 instructions.
	sib_ = 0;

	const internal::EncOpCodeHandler* handler = &internal::OP_CODE_HANDLERS[static_cast<std::size_t>(instruction.code())];
	handler_ = handler;
	op_code_ = handler->op_code;
	const std::int32_t group_index = handler->group_index;
	if (group_index >= 0) {
		encoder_flags_ |= EncoderFlags::MOD_RM;
		mod_rm_ = static_cast<std::uint8_t>(static_cast<std::uint32_t>(group_index) << 3);
	}
	const std::int32_t rm_group_index = handler->rm_group_index;
	if (rm_group_index >= 0) {
		encoder_flags_ |= EncoderFlags::MOD_RM;
		mod_rm_ |= static_cast<std::uint8_t>(static_cast<std::uint32_t>(rm_group_index) | 0xC0);
	}

	switch (handler->enc_flags3 & (EncFlags3::BIT16OR32 | EncFlags3::BIT64)) {
	case EncFlags3::BIT16OR32:
		if (bitness_ == 64)
			EncoderInternal::set_error_message_str(*this, EncoderInternal::ERROR_ONLY_1632_BIT_MODE);
		break;

	case EncFlags3::BIT64:
		if (bitness_ != 64)
			EncoderInternal::set_error_message_str(*this, EncoderInternal::ERROR_ONLY_64_BIT_MODE);
		break;

	default:
		break;
	}

	switch (handler->op_size) {
	case CodeSize::Unknown:
		break;
	case CodeSize::Code16:
		encoder_flags_ |= opsize16_flags_;
		break;
	case CodeSize::Code32:
		encoder_flags_ |= opsize32_flags_;
		break;
	case CodeSize::Code64:
		if ((handler->enc_flags3 & EncFlags3::DEFAULT_OP_SIZE64) == 0)
			encoder_flags_ |= EncoderFlags::W;
		break;
	}

	switch (handler->addr_size) {
	case CodeSize::Unknown:
	case CodeSize::Code64:
		break;
	case CodeSize::Code16:
		encoder_flags_ |= adrsize16_flags_;
		break;
	case CodeSize::Code32:
		encoder_flags_ |= adrsize32_flags_;
		break;
	}

	if (!handler->is_special_instr) {
		const std::uint32_t operands_len = handler->operands_len;
		for (std::uint32_t i = 0; i < operands_len; i++)
			handler->operand(i)->encode(*this, instruction, i);

		if ((handler->enc_flags3 & EncFlags3::FWAIT) != 0)
			EncoderInternal::write_byte_internal(*this, 0x9B);

		internal::OP_CODE_HANDLER_ENCODE_FNS[static_cast<std::size_t>(handler->kind)](handler, *this, instruction);

		const std::uint32_t op_code = op_code_;
		if (!handler->is_2byte_opcode)
			EncoderInternal::write_byte_internal(*this, op_code);
		else {
			EncoderInternal::write_byte_internal(*this, op_code >> 8);
			EncoderInternal::write_byte_internal(*this, op_code);
		}

		if ((encoder_flags_ & (EncoderFlags::MOD_RM | EncoderFlags::DISPL)) != 0)
			EncoderInternal::write_mod_rm(*this);

		if (imm_size_ != ImmSize::None)
			EncoderInternal::write_immediate(*this);
	}
	else
		internal::OP_CODE_HANDLER_ENCODE_FNS[static_cast<std::size_t>(handler->kind)](handler, *this, instruction);

	const std::size_t instr_len = static_cast<std::size_t>(current_rip_) - static_cast<std::size_t>(rip);
	static_assert(IcedConstants::MAX_INSTRUCTION_LENGTH == 15, "");
	if (instr_len > IcedConstants::MAX_INSTRUCTION_LENGTH && !handler->is_special_instr)
		EncoderInternal::set_error_message_str(*this, "Instruction length > 15 bytes");
	if (!error_message_.empty()) {
		IcedError error(std::move(error_message_));
		error_message_.clear();
		return error;
	}
	return instr_len;
}

ConstantOffsets Encoder::get_constant_offsets() const noexcept {
	ConstantOffsets co{};

	switch (displ_size_) {
	case DisplSize::None:
		break;

	case DisplSize::Size1:
		co.displacement_size_ = 1;
		co.displacement_offset_ = static_cast<std::uint8_t>(displ_addr_ - eip_);
		break;

	case DisplSize::Size2:
		co.displacement_size_ = 2;
		co.displacement_offset_ = static_cast<std::uint8_t>(displ_addr_ - eip_);
		break;

	case DisplSize::Size4:
	case DisplSize::RipRelSize4_Target32:
	case DisplSize::RipRelSize4_Target64:
		co.displacement_size_ = 4;
		co.displacement_offset_ = static_cast<std::uint8_t>(displ_addr_ - eip_);
		break;

	case DisplSize::Size8:
		co.displacement_size_ = 8;
		co.displacement_offset_ = static_cast<std::uint8_t>(displ_addr_ - eip_);
		break;
	}

	switch (imm_size_) {
	case ImmSize::None:
	case ImmSize::SizeIbReg:
	case ImmSize::Size1OpCode:
		break;

	case ImmSize::Size1:
	case ImmSize::RipRelSize1_Target16:
	case ImmSize::RipRelSize1_Target32:
	case ImmSize::RipRelSize1_Target64:
		co.immediate_size_ = 1;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		break;

	case ImmSize::Size1_1:
		co.immediate_size_ = 1;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		co.immediate_size2_ = 1;
		co.immediate_offset2_ = static_cast<std::uint8_t>(imm_addr_ - eip_ + 1);
		break;

	case ImmSize::Size2:
	case ImmSize::RipRelSize2_Target16:
	case ImmSize::RipRelSize2_Target32:
	case ImmSize::RipRelSize2_Target64:
		co.immediate_size_ = 2;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		break;

	case ImmSize::Size2_1:
		co.immediate_size_ = 2;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		co.immediate_size2_ = 1;
		co.immediate_offset2_ = static_cast<std::uint8_t>(imm_addr_ - eip_ + 2);
		break;

	case ImmSize::Size2_2:
		co.immediate_size_ = 2;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		co.immediate_size2_ = 2;
		co.immediate_offset2_ = static_cast<std::uint8_t>(imm_addr_ - eip_ + 2);
		break;

	case ImmSize::Size4:
	case ImmSize::RipRelSize4_Target32:
	case ImmSize::RipRelSize4_Target64:
		co.immediate_size_ = 4;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		break;

	case ImmSize::Size4_2:
		co.immediate_size_ = 4;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		co.immediate_size2_ = 2;
		co.immediate_offset2_ = static_cast<std::uint8_t>(imm_addr_ - eip_ + 4);
		break;

	case ImmSize::Size8:
		co.immediate_size_ = 8;
		co.immediate_offset_ = static_cast<std::uint8_t>(imm_addr_ - eip_);
		break;
	}

	return co;
}

namespace internal {

void EncoderInternal::write_byte_grow(Encoder& e, std::uint32_t value) {
	e.buffer_.push_back(static_cast<std::uint8_t>(value));
}

void EncoderInternal::verify_op_kind_failed(Encoder& e, std::uint32_t operand, OpKind expected, OpKind actual) {
	std::string msg = op_str(operand);
#ifndef NDEBUG
	msg += "Expected: ";
	msg += op_kind_str(expected);
	msg += ", actual: ";
	msg += op_kind_str(actual);
#else
	msg += "Expected: OpKind value ";
	msg += std::to_string(static_cast<std::uint32_t>(expected));
	msg += ", actual: OpKind value ";
	msg += std::to_string(static_cast<std::uint32_t>(actual));
#endif
	set_error_message(e, std::move(msg));
}

void EncoderInternal::verify_register_failed(Encoder& e, std::uint32_t operand, Register expected, Register actual) {
	std::string msg = op_str(operand);
	msg += "Expected: ";
	append_reg(msg, expected);
	msg += ", actual: ";
	append_reg(msg, actual);
	set_error_message(e, std::move(msg));
}

void EncoderInternal::verify_register_range_failed(Encoder& e, std::uint32_t operand, Register register_, Register reg_lo, Register reg_hi) {
	std::string msg = op_str(operand);
	msg += "Register ";
#ifndef NDEBUG
	msg += register_str(register_);
	msg += " is not between ";
	msg += register_str(reg_lo);
	msg += " and ";
	msg += register_str(reg_hi);
#else
	msg += std::to_string(static_cast<std::uint32_t>(register_));
	msg += " is not between ";
	msg += std::to_string(static_cast<std::uint32_t>(reg_lo));
	msg += " and ";
	msg += std::to_string(static_cast<std::uint32_t>(reg_hi));
#endif
	msg += " (inclusive)";
	set_error_message(e, std::move(msg));
}

void EncoderInternal::add_branch(Encoder& e, OpKind op_kind, std::uint32_t imm_size, const Instruction& instruction, std::uint32_t operand) {
	if (!verify_op_kind(e, operand, op_kind, instruction.op_kind(operand)))
		return;

	std::uint64_t target;
	switch (imm_size) {
	case 1:
		switch (op_kind) {
		case OpKind::NearBranch16:
			e.encoder_flags_ |= e.opsize16_flags_;
			e.imm_size_ = ImmSize::RipRelSize1_Target16;
			e.immediate_ = instruction.near_branch16();
			break;

		case OpKind::NearBranch32:
			e.encoder_flags_ |= e.opsize32_flags_;
			e.imm_size_ = ImmSize::RipRelSize1_Target32;
			e.immediate_ = instruction.near_branch32();
			break;

		case OpKind::NearBranch64:
			e.imm_size_ = ImmSize::RipRelSize1_Target64;
			target = instruction.near_branch64();
			e.immediate_ = static_cast<std::uint32_t>(target);
			e.immediate_hi_ = static_cast<std::uint32_t>(target >> 32);
			break;

		default:
			ICED_UNREACHABLE();
		}
		break;

	case 2:
		switch (op_kind) {
		case OpKind::NearBranch16:
			e.encoder_flags_ |= e.opsize16_flags_;
			e.imm_size_ = ImmSize::RipRelSize2_Target16;
			e.immediate_ = instruction.near_branch16();
			break;

		default:
			ICED_UNREACHABLE();
		}
		break;

	case 4:
		switch (op_kind) {
		case OpKind::NearBranch32:
			e.encoder_flags_ |= e.opsize32_flags_;
			e.imm_size_ = ImmSize::RipRelSize4_Target32;
			e.immediate_ = instruction.near_branch32();
			break;

		case OpKind::NearBranch64:
			e.imm_size_ = ImmSize::RipRelSize4_Target64;
			target = instruction.near_branch64();
			e.immediate_ = static_cast<std::uint32_t>(target);
			e.immediate_hi_ = static_cast<std::uint32_t>(target >> 32);
			break;

		default:
			ICED_UNREACHABLE();
		}
		break;

	default:
		ICED_UNREACHABLE();
	}
}

void EncoderInternal::add_branch_x(Encoder& e, std::uint32_t imm_size, const Instruction& instruction, std::uint32_t operand) {
	if (e.bitness_ == 64) {
		if (!verify_op_kind(e, operand, OpKind::NearBranch64, instruction.op_kind(operand)))
			return;

		const std::uint64_t target = instruction.near_branch64();
		switch (imm_size) {
		case 2:
			e.encoder_flags_ |= EncoderFlags::P66;
			e.imm_size_ = ImmSize::RipRelSize2_Target64;
			e.immediate_ = static_cast<std::uint32_t>(target);
			e.immediate_hi_ = static_cast<std::uint32_t>(target >> 32);
			break;

		case 4:
			e.imm_size_ = ImmSize::RipRelSize4_Target64;
			e.immediate_ = static_cast<std::uint32_t>(target);
			e.immediate_hi_ = static_cast<std::uint32_t>(target >> 32);
			break;

		default:
			ICED_UNREACHABLE();
		}
	}
	else {
		if (!verify_op_kind(e, operand, OpKind::NearBranch32, instruction.op_kind(operand)))
			return;

		switch (imm_size) {
		case 2:
			static_assert(EncoderFlags::P66 == 0x80, "");
			e.encoder_flags_ |= (e.bitness_ & 0x20) << 2;
			e.imm_size_ = ImmSize::RipRelSize2_Target32;
			e.immediate_ = instruction.near_branch32();
			break;

		case 4:
			static_assert(EncoderFlags::P66 == 0x80, "");
			e.encoder_flags_ |= (e.bitness_ & 0x10) << 3;
			e.imm_size_ = ImmSize::RipRelSize4_Target32;
			e.immediate_ = instruction.near_branch32();
			break;

		default:
			ICED_UNREACHABLE();
		}
	}
}

void EncoderInternal::add_branch_disp(Encoder& e, std::uint32_t displ_size, const Instruction& instruction, std::uint32_t operand) {
	ICED_DEBUG_ASSERT(displ_size == 2 || displ_size == 4);
	OpKind op_kind;
	switch (displ_size) {
	case 2:
		op_kind = OpKind::NearBranch16;
		e.imm_size_ = ImmSize::Size2;
		e.immediate_ = instruction.near_branch16();
		break;

	case 4:
		op_kind = OpKind::NearBranch32;
		e.imm_size_ = ImmSize::Size4;
		e.immediate_ = instruction.near_branch32();
		break;

	default:
		ICED_UNREACHABLE();
	}
	(void)verify_op_kind(e, operand, op_kind, instruction.op_kind(operand));
}

void EncoderInternal::add_far_branch(Encoder& e, const Instruction& instruction, std::uint32_t operand, std::uint32_t size) {
	if (size == 2) {
		if (!verify_op_kind(e, operand, OpKind::FarBranch16, instruction.op_kind(operand)))
			return;
		e.imm_size_ = ImmSize::Size2_2;
		e.immediate_ = instruction.far_branch16();
		e.immediate_hi_ = instruction.far_branch_selector();
	}
	else {
		ICED_DEBUG_ASSERT(size == 4);
		if (!verify_op_kind(e, operand, OpKind::FarBranch32, instruction.op_kind(operand)))
			return;
		e.imm_size_ = ImmSize::Size4_2;
		e.immediate_ = instruction.far_branch32();
		e.immediate_hi_ = instruction.far_branch_selector();
	}
	if (e.bitness_ != size * 8)
		e.encoder_flags_ |= EncoderFlags::P66;
}

void EncoderInternal::set_addr_size(Encoder& e, std::uint32_t reg_size) {
	ICED_DEBUG_ASSERT(reg_size == 2 || reg_size == 4 || reg_size == 8);
	if (e.bitness_ == 64) {
		if (reg_size == 2)
			set_invalid_reg_size_error(e, reg_size, ", must be 32-bit or 64-bit");
		else if (reg_size == 4)
			e.encoder_flags_ |= EncoderFlags::P67;
	}
	else {
		if (reg_size == 8)
			set_invalid_reg_size_error(e, reg_size, ", must be 16-bit or 32-bit");
		else if (e.bitness_ == 16) {
			if (reg_size == 4)
				e.encoder_flags_ |= EncoderFlags::P67;
		}
		else {
			ICED_DEBUG_ASSERT(e.bitness_ == 32);
			if (reg_size == 2)
				e.encoder_flags_ |= EncoderFlags::P67;
		}
	}
}

void EncoderInternal::add_abs_mem(Encoder& e, const Instruction& instruction, std::uint32_t operand) {
	e.encoder_flags_ |= EncoderFlags::DISPL;
	const OpKind op_kind = instruction.op_kind(operand);
	if (op_kind == OpKind::Memory) {
		if (instruction.memory_base() != Register::None || instruction.memory_index() != Register::None) {
			set_op_error(e, operand, "Absolute addresses can't have base and/or index regs");
			return;
		}
		if (instruction.memory_index_scale() != 1) {
			set_op_error(e, operand, "Absolute addresses must have scale == *1");
			return;
		}
		switch (instruction.memory_displ_size()) {
		case 2:
			if (e.bitness_ == 64) {
				set_op_error(e, operand, "16-bit abs addresses can't be used in 64-bit mode");
				return;
			}
			if (e.bitness_ == 32)
				e.encoder_flags_ |= EncoderFlags::P67;
			e.displ_size_ = DisplSize::Size2;
			if (instruction.memory_displacement64() > std::numeric_limits<std::uint16_t>::max()) {
				set_op_error(e, operand, "Displacement must fit in a u16");
				return;
			}
			e.displ_ = instruction.memory_displacement32();
			break;

		case 4:
			e.encoder_flags_ |= e.adrsize32_flags_;
			e.displ_size_ = DisplSize::Size4;
			if (instruction.memory_displacement64() > std::numeric_limits<std::uint32_t>::max()) {
				set_op_error(e, operand, "Displacement must fit in a u32");
				return;
			}
			e.displ_ = instruction.memory_displacement32();
			break;

		case 8: {
			if (e.bitness_ != 64) {
				set_op_error(e, operand, "64-bit abs address is only available in 64-bit mode");
				return;
			}
			e.displ_size_ = DisplSize::Size8;
			const std::uint64_t addr = instruction.memory_displacement64();
			e.displ_ = static_cast<std::uint32_t>(addr);
			e.displ_hi_ = static_cast<std::uint32_t>(addr >> 32);
			break;
		}

		default:
			set_op_error(e, operand, "Instruction::memory_displ_size() must be initialized to 2 (16-bit), 4 (32-bit) or 8 (64-bit)");
			break;
		}
	}
	else {
		set_op_error_op_kind(e, operand, "Expected OpKind::Memory, actual: ", op_kind, true);
	}
}

void EncoderInternal::add_mod_rm_register(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi) {
	if (!verify_op_kind(e, operand, OpKind::Register, instruction.op_kind(operand)))
		return;
	const Register reg = instruction.op_register(operand);
	if (!verify_register_range(e, operand, reg, reg_lo, reg_hi))
		return;
	std::uint32_t reg_num = static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(reg_lo);
	if (reg_lo == Register::AL) {
		if (reg >= Register::SPL) {
			reg_num -= 4;
			e.encoder_flags_ |= EncoderFlags::REX;
		}
		else if (reg >= Register::AH)
			e.encoder_flags_ |= EncoderFlags::HIGH_LEGACY_8_BIT_REGS;
	}
	ICED_DEBUG_ASSERT(reg_num <= 31);
	e.mod_rm_ |= static_cast<std::uint8_t>((reg_num & 7) << 3);
	e.encoder_flags_ |= EncoderFlags::MOD_RM;
	static_assert(EncoderFlags::R == 4, "");
	e.encoder_flags_ |= (reg_num & 8) >> 1;
	static_assert(EncoderFlags::R2 == 0x200, "");
	e.encoder_flags_ |= (reg_num & 0x10) << (9 - 4);
}

void EncoderInternal::add_reg(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi) {
	if (!verify_op_kind(e, operand, OpKind::Register, instruction.op_kind(operand)))
		return;
	const Register reg = instruction.op_register(operand);
	if (!verify_register_range(e, operand, reg, reg_lo, reg_hi))
		return;
	std::uint32_t reg_num = static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(reg_lo);
	if (reg_lo == Register::AL) {
		if (reg >= Register::SPL) {
			reg_num -= 4;
			e.encoder_flags_ |= EncoderFlags::REX;
		}
		else if (reg >= Register::AH)
			e.encoder_flags_ |= EncoderFlags::HIGH_LEGACY_8_BIT_REGS;
	}
	ICED_DEBUG_ASSERT(reg_num <= 15);
	e.op_code_ |= reg_num & 7;
	static_assert(EncoderFlags::B == 1, "");
	e.encoder_flags_ |= reg_num >> 3; // reg_num <= 15, so no need to mask out anything
}

void EncoderInternal::add_reg_or_mem_full(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi,
	Register vsib_index_reg_lo, Register vsib_index_reg_hi, bool allow_mem_op, bool allow_reg_op) {
	const OpKind op_kind = instruction.op_kind(operand);
	e.encoder_flags_ |= EncoderFlags::MOD_RM;
	if (op_kind == OpKind::Register) {
		if (!allow_reg_op) {
			set_op_error(e, operand, "register operand is not allowed");
			return;
		}
		const Register reg = instruction.op_register(operand);
		if (!verify_register_range(e, operand, reg, reg_lo, reg_hi))
			return;
		std::uint32_t reg_num = static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(reg_lo);
		if (reg_lo == Register::AL) {
			if (reg >= Register::R8L)
				reg_num -= 4;
			else if (reg >= Register::SPL) {
				reg_num -= 4;
				e.encoder_flags_ |= EncoderFlags::REX;
			}
			else if (reg >= Register::AH)
				e.encoder_flags_ |= EncoderFlags::HIGH_LEGACY_8_BIT_REGS;
		}
		e.mod_rm_ |= static_cast<std::uint8_t>(reg_num & 7);
		e.mod_rm_ |= 0xC0;
		static_assert(EncoderFlags::B == 1, "");
		static_assert(EncoderFlags::X == 2, "");
		e.encoder_flags_ |= (reg_num >> 3) & 3;
		ICED_DEBUG_ASSERT(reg_num <= 31);
	}
	else if (op_kind == OpKind::Memory) {
		if (!allow_mem_op) {
			set_op_error(e, operand, "memory operand is not allowed");
			return;
		}
		if (memory_size_ext::is_broadcast(instruction.memory_size()))
			e.encoder_flags_ |= EncoderFlags::BROADCAST;

		CodeSize code_size = instruction.code_size();
		if (code_size == CodeSize::Unknown) {
			if (e.bitness_ == 64)
				code_size = CodeSize::Code64;
			else if (e.bitness_ == 32)
				code_size = CodeSize::Code32;
			else {
				ICED_DEBUG_ASSERT(e.bitness_ == 16);
				code_size = CodeSize::Code16;
			}
		}
		const std::uint32_t addr_size = InstructionInternal::get_address_size_in_bytes(instruction.memory_base(), instruction.memory_index(),
											instruction.memory_displ_size(), code_size) *
			8;
		if (addr_size != e.bitness_)
			e.encoder_flags_ |= EncoderFlags::P67;
		if ((e.encoder_flags_ & EncoderFlags::REG_IS_MEMORY) != 0) {
			const std::uint32_t reg_size = get_register_op_size(instruction);
			if (reg_size != addr_size) {
				set_op_error(e, operand, "Register operand size must equal memory addressing mode (16/32/64)");
				return;
			}
		}
		if (addr_size == 16) {
			if (vsib_index_reg_lo != Register::None) {
				set_op_error(e, operand, "VSIB operands can't use 16-bit addressing. It must be 32-bit or 64-bit addressing");
				return;
			}
			add_mem_op16(e, instruction, operand);
		}
		else
			add_mem_op(e, instruction, operand, addr_size, vsib_index_reg_lo, vsib_index_reg_hi);
	}
	else {
		set_op_error_op_kind(e, operand, "Expected a register or memory operand, but op_kind is ", op_kind, false);
	}
}

std::uint32_t EncoderInternal::get_register_op_size(const Instruction& instruction) {
	ICED_DEBUG_ASSERT(instruction.op0_kind() == OpKind::Register);
	if (instruction.op0_kind() == OpKind::Register) {
		const Register reg = instruction.op0_register();
		if (register_ext::is_gpr64(reg))
			return 64;
		if (register_ext::is_gpr32(reg))
			return 32;
		if (register_ext::is_gpr16(reg))
			return 16;
		return 0;
	}
	return 0;
}

std::optional<std::int8_t> EncoderInternal::try_convert_to_disp8n(Encoder& e, const Instruction& instruction, std::int32_t displ) {
	const EncOpCodeHandler* handler = e.handler_;
	switch (handler->kind) {
	case OpCodeHandlerKind::EVEX:
		return evex_try_convert_to_disp8n(handler, e, instruction, displ);
	case OpCodeHandlerKind::MVEX:
		return mvex_try_convert_to_disp8n(handler, e, instruction, displ);
	default:
		break;
	}
	if (std::numeric_limits<std::int8_t>::min() <= displ && displ <= std::numeric_limits<std::int8_t>::max())
		return static_cast<std::int8_t>(displ);
	return std::nullopt;
}

void EncoderInternal::add_mem_op16(Encoder& e, const Instruction& instruction, std::uint32_t operand) {
	if (e.bitness_ == 64) {
		set_op_error(e, operand, "16-bit addressing can't be used by 64-bit code");
		return;
	}
	const Register base = instruction.memory_base();
	const Register index = instruction.memory_index();
	std::uint32_t displ_size = instruction.memory_displ_size();
	if (base == Register::BX && index == Register::SI) {
		// Nothing
	}
	else if (base == Register::BX && index == Register::DI)
		e.mod_rm_ |= 1;
	else if (base == Register::BP && index == Register::SI)
		e.mod_rm_ |= 2;
	else if (base == Register::BP && index == Register::DI)
		e.mod_rm_ |= 3;
	else if (base == Register::SI && index == Register::None)
		e.mod_rm_ |= 4;
	else if (base == Register::DI && index == Register::None)
		e.mod_rm_ |= 5;
	else if (base == Register::BP && index == Register::None)
		e.mod_rm_ |= 6;
	else if (base == Register::BX && index == Register::None)
		e.mod_rm_ |= 7;
	else if (base == Register::None && index == Register::None) {
		e.mod_rm_ |= 6;
		e.displ_size_ = DisplSize::Size2;
		if (instruction.memory_displacement64() > std::numeric_limits<std::uint16_t>::max()) {
			set_op_error(e, operand, "Displacement must fit in a u16");
			return;
		}
		e.displ_ = instruction.memory_displacement32();
	}
	else {
		set_invalid_16bit_regs_error(e, operand, base, index);
		return;
	}

	if (base != Register::None || index != Register::None) {
		const std::int64_t displ64 = static_cast<std::int64_t>(instruction.memory_displacement64());
		if (displ64 < std::numeric_limits<std::int16_t>::min() || displ64 > std::numeric_limits<std::uint16_t>::max()) {
			set_op_error(e, operand, "Displacement must fit in an i16 or a u16");
			return;
		}
		e.displ_ = instruction.memory_displacement32();
		// [bp] => [bp+00]
		if (displ_size == 0 && base == Register::BP && index == Register::None) {
			displ_size = 1;
			if (e.displ_ != 0) {
				set_op_error(e, operand, "Displacement must be 0 if displ_size == 0");
				return;
			}
		}
		if (displ_size == 1) {
			if (auto compressed_value =
					try_convert_to_disp8n(e, instruction, static_cast<std::int32_t>(static_cast<std::int16_t>(static_cast<std::uint16_t>(e.displ_)))))
				e.displ_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(*compressed_value));
			else
				displ_size = 2;
		}
		if (displ_size == 0) {
			if (e.displ_ != 0) {
				set_op_error(e, operand, "Displacement must be 0 if displ_size == 0");
				return;
			}
		}
		else if (displ_size == 1) {
			// This if check should never be true when we're here
			if (static_cast<std::int32_t>(e.displ_) < std::numeric_limits<std::int8_t>::min() ||
				static_cast<std::int32_t>(e.displ_) > std::numeric_limits<std::int8_t>::max()) {
				set_op_error(e, operand, "Displacement must fit in an i8");
				return;
			}
			e.mod_rm_ |= 0x40;
			e.displ_size_ = DisplSize::Size1;
		}
		else if (displ_size == 2) {
			e.mod_rm_ |= 0x80;
			e.displ_size_ = DisplSize::Size2;
		}
		else
			set_op_error_num(e, operand, "Invalid displacement size: ", displ_size, ", must be 0, 1, or 2");
	}
}

void EncoderInternal::add_mem_op(Encoder& e, const Instruction& instruction, std::uint32_t operand, std::uint32_t addr_size, Register vsib_index_reg_lo,
	Register vsib_index_reg_hi) {
	ICED_DEBUG_ASSERT(addr_size == 32 || addr_size == 64);
	if (e.bitness_ != 64 && addr_size == 64) {
		set_op_error(e, operand, "64-bit addressing can only be used in 64-bit mode");
		return;
	}

	const Register base = instruction.memory_base();
	const Register index = instruction.memory_index();
	std::uint32_t displ_size = instruction.memory_displ_size();

	Register base_lo;
	Register base_hi;
	Register index_lo;
	Register index_hi;
	if (addr_size == 64) {
		base_lo = Register::RAX;
		base_hi = Register::R15;
	}
	else {
		ICED_DEBUG_ASSERT(addr_size == 32);
		base_lo = Register::EAX;
		base_hi = Register::R15D;
	}
	if (vsib_index_reg_lo != Register::None) {
		index_lo = vsib_index_reg_lo;
		index_hi = vsib_index_reg_hi;
	}
	else {
		index_lo = base_lo;
		index_hi = base_hi;
	}
	if (base != Register::None && base != Register::RIP && base != Register::EIP && !verify_register_range(e, operand, base, base_lo, base_hi))
		return;
	if (index != Register::None && !verify_register_range(e, operand, index, index_lo, index_hi))
		return;

	if (displ_size != 0 && displ_size != 1 && displ_size != 4 && displ_size != 8) {
		set_op_error_num(e, operand, "Invalid displ size: ", displ_size, ", must be 0, 1, 4, 8");
		return;
	}
	if (base == Register::RIP || base == Register::EIP) {
		if (index != Register::None) {
			set_op_error(e, operand, "RIP relative addressing can't use an index register");
			return;
		}
		if (InstructionInternal::internal_get_memory_index_scale(instruction) != 0) {
			set_op_error(e, operand, "RIP relative addressing must use scale *1");
			return;
		}
		if (e.bitness_ != 64) {
			set_op_error(e, operand, "RIP/EIP relative addressing is only available in 64-bit mode");
			return;
		}
		if ((e.encoder_flags_ & EncoderFlags::MUST_USE_SIB) != 0) {
			set_op_error(e, operand, "RIP/EIP relative addressing isn't supported");
			return;
		}
		e.mod_rm_ |= 5;
		const std::uint64_t target = instruction.memory_displacement64();
		if (base == Register::RIP) {
			e.displ_size_ = DisplSize::RipRelSize4_Target64;
			e.displ_ = static_cast<std::uint32_t>(target);
			e.displ_hi_ = static_cast<std::uint32_t>(target >> 32);
		}
		else {
			e.displ_size_ = DisplSize::RipRelSize4_Target32;
			if (target > std::numeric_limits<std::uint32_t>::max()) {
				set_op_error_hex(e, operand, "Target address doesn't fit in 32 bits: 0x", target);
				return;
			}
			e.displ_ = static_cast<std::uint32_t>(target);
		}
		return;
	}
	const std::uint32_t scale = InstructionInternal::internal_get_memory_index_scale(instruction);
	e.displ_ = instruction.memory_displacement32();
	const std::int64_t displ64 = static_cast<std::int64_t>(instruction.memory_displacement64());
	if (addr_size == 64) {
		if (displ64 < std::numeric_limits<std::int32_t>::min() || displ64 > std::numeric_limits<std::int32_t>::max()) {
			set_op_error(e, operand, "Displacement must fit in an i32");
			return;
		}
	}
	else {
		ICED_DEBUG_ASSERT(addr_size == 32);
		if (displ64 < std::numeric_limits<std::int32_t>::min() || displ64 > static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max())) {
			set_op_error(e, operand, "Displacement must fit in an i32 or a u32");
			return;
		}
	}
	if (base == Register::None && index == Register::None) {
		if (vsib_index_reg_lo != Register::None) {
			set_op_error(e, operand, "VSIB addressing can't use an offset-only address");
			return;
		}
		if (e.bitness_ == 64 || scale != 0 || (e.encoder_flags_ & EncoderFlags::MUST_USE_SIB) != 0) {
			e.mod_rm_ |= 4;
			e.displ_size_ = DisplSize::Size4;
			e.encoder_flags_ |= EncoderFlags::SIB;
			e.sib_ = static_cast<std::uint8_t>(0x25 | (scale << 6));
			return;
		}
		e.mod_rm_ |= 5;
		e.displ_size_ = DisplSize::Size4;
		return;
	}

	const std::int32_t base_num = base == Register::None ? -1 : static_cast<std::int32_t>(base) - static_cast<std::int32_t>(base_lo);
	const std::int32_t index_num = index == Register::None ? -1 : static_cast<std::int32_t>(index) - static_cast<std::int32_t>(index_lo);

	// [ebp]/[ebp+index*scale] => [ebp+00]/[ebp+index*scale+00]
	if (displ_size == 0 && (base_num & 7) == 5) {
		displ_size = 1;
		if (e.displ_ != 0) {
			set_op_error(e, operand, "Displacement must be 0 if displ_size == 0");
			return;
		}
	}

	if (displ_size == 1) {
		if (auto compressed_value = try_convert_to_disp8n(e, instruction, static_cast<std::int32_t>(e.displ_)))
			e.displ_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(*compressed_value));
		else
			displ_size = addr_size / 8;
	}

	if (base == Register::None) {
		// Tested earlier in the method
		ICED_DEBUG_ASSERT(index != Register::None);
		e.displ_size_ = DisplSize::Size4;
	}
	else if (displ_size == 1) {
		// This if check should never be true when we're here
		if (static_cast<std::int32_t>(e.displ_) < std::numeric_limits<std::int8_t>::min() ||
			static_cast<std::int32_t>(e.displ_) > std::numeric_limits<std::int8_t>::max()) {
			set_op_error(e, operand, "Displacement must fit in an i8");
			return;
		}
		e.mod_rm_ |= 0x40;
		e.displ_size_ = DisplSize::Size1;
	}
	else if (displ_size == addr_size / 8) {
		e.mod_rm_ |= 0x80;
		e.displ_size_ = DisplSize::Size4;
	}
	else if (displ_size == 0) {
		if (e.displ_ != 0) {
			set_op_error(e, operand, "Displacement must be 0 if displ_size == 0");
			return;
		}
	}
	else
		set_error_message_str(e, "Invalid memory_displ_size() value");

	if (index == Register::None && (base_num & 7) != 4 && scale == 0 && (e.encoder_flags_ & EncoderFlags::MUST_USE_SIB) == 0) {
		// Tested earlier in the method
		ICED_DEBUG_ASSERT(base != Register::None);
		e.mod_rm_ |= static_cast<std::uint8_t>(base_num & 7);
	}
	else {
		e.encoder_flags_ |= EncoderFlags::SIB;
		e.sib_ = static_cast<std::uint8_t>(scale << 6);
		e.mod_rm_ |= 4;
		if (index == Register::RSP || index == Register::ESP) {
			set_op_error(e, operand, "ESP/RSP can't be used as an index register");
			return;
		}
		if (base_num < 0)
			e.sib_ |= 5;
		else
			e.sib_ |= static_cast<std::uint8_t>(base_num & 7);
		if (index_num < 0)
			e.sib_ |= 0x20;
		else
			e.sib_ |= static_cast<std::uint8_t>((index_num & 7) << 3);
	}

	if (base_num >= 0) {
		static_assert(EncoderFlags::B == 1, "");
		ICED_DEBUG_ASSERT(base_num <= 15); // No '& 1' required below
		e.encoder_flags_ |= static_cast<std::uint32_t>(base_num) >> 3;
	}
	if (index_num >= 0) {
		static_assert(EncoderFlags::X == 2, "");
		e.encoder_flags_ |= (static_cast<std::uint32_t>(index_num) >> 2) & 2;
		e.encoder_flags_ |= (static_cast<std::uint32_t>(index_num) & 0x10) << EncoderFlags::VVVVV_SHIFT;
		ICED_DEBUG_ASSERT(index_num <= 31);
	}
}

void EncoderInternal::write_prefixes(Encoder& e, const Instruction& instruction, bool can_write_f3) {
	ICED_DEBUG_ASSERT(!e.handler_->is_special_instr);
	const Register seg = instruction.segment_prefix();
	if (seg != Register::None) {
		static constexpr std::uint8_t SEGMENT_OVERRIDES[6] = {0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65};
		static_assert(static_cast<std::uint32_t>(Register::ES) + 1 == static_cast<std::uint32_t>(Register::CS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 2 == static_cast<std::uint32_t>(Register::SS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 3 == static_cast<std::uint32_t>(Register::DS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 4 == static_cast<std::uint32_t>(Register::FS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 5 == static_cast<std::uint32_t>(Register::GS), "");
		const std::size_t index = static_cast<std::size_t>(seg) - static_cast<std::size_t>(Register::ES);
		ICED_ASSERT(index < sizeof(SEGMENT_OVERRIDES));
		write_byte_internal(e, SEGMENT_OVERRIDES[index]);
	}
	if ((e.encoder_flags_ & EncoderFlags::PF0) != 0 || instruction.has_lock_prefix())
		write_byte_internal(e, 0xF0);
	if ((e.encoder_flags_ & EncoderFlags::P66) != 0)
		write_byte_internal(e, 0x66);
	if ((e.encoder_flags_ & EncoderFlags::P67) != 0)
		write_byte_internal(e, 0x67);
	if (can_write_f3 && instruction.has_repe_prefix())
		write_byte_internal(e, 0xF3);
	if (instruction.has_repne_prefix())
		write_byte_internal(e, 0xF2);
}

void EncoderInternal::write_mod_rm(Encoder& e) {
	ICED_DEBUG_ASSERT(!e.handler_->is_special_instr);
	ICED_DEBUG_ASSERT((e.encoder_flags_ & (EncoderFlags::MOD_RM | EncoderFlags::DISPL)) != 0);
	if ((e.encoder_flags_ & EncoderFlags::MOD_RM) != 0) {
		write_byte_internal(e, e.mod_rm_);
		if ((e.encoder_flags_ & EncoderFlags::SIB) != 0)
			write_byte_internal(e, e.sib_);
	}

	std::uint32_t diff4;
	e.displ_addr_ = static_cast<std::uint32_t>(e.current_rip_);
	switch (e.displ_size_) {
	case DisplSize::None:
		break;

	case DisplSize::Size1:
		write_byte_internal(e, e.displ_);
		break;

	case DisplSize::Size2:
		diff4 = e.displ_;
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		break;

	case DisplSize::Size4:
		diff4 = e.displ_;
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		write_byte_internal(e, diff4 >> 16);
		write_byte_internal(e, diff4 >> 24);
		break;

	case DisplSize::Size8:
		diff4 = e.displ_;
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		write_byte_internal(e, diff4 >> 16);
		write_byte_internal(e, diff4 >> 24);
		diff4 = e.displ_hi_;
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		write_byte_internal(e, diff4 >> 16);
		write_byte_internal(e, diff4 >> 24);
		break;

	case DisplSize::RipRelSize4_Target32: {
		const std::uint32_t eip = static_cast<std::uint32_t>(e.current_rip_) + 4 + IMM_SIZES[static_cast<std::size_t>(e.imm_size_)];
		diff4 = e.displ_ - eip;
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		write_byte_internal(e, diff4 >> 16);
		write_byte_internal(e, diff4 >> 24);
		break;
	}

	case DisplSize::RipRelSize4_Target64: {
		const std::uint64_t rip = e.current_rip_ + 4 + IMM_SIZES[static_cast<std::size_t>(e.imm_size_)];
		const std::uint64_t target = (static_cast<std::uint64_t>(e.displ_hi_) << 32) | e.displ_;
		const std::int64_t diff8 = static_cast<std::int64_t>(target - rip);
		if (diff8 < std::numeric_limits<std::int32_t>::min() || diff8 > std::numeric_limits<std::int32_t>::max()) {
			set_distance_error(e, "RIP relative distance", rip, 16, target, 8, diff8, "i32");
		}
		diff4 = static_cast<std::uint32_t>(diff8);
		write_byte_internal(e, diff4);
		write_byte_internal(e, diff4 >> 8);
		write_byte_internal(e, diff4 >> 16);
		write_byte_internal(e, diff4 >> 24);
		break;
	}
	}
}

void EncoderInternal::write_immediate(Encoder& e) {
	ICED_DEBUG_ASSERT(!e.handler_->is_special_instr);
	std::uint32_t value;
	e.imm_addr_ = static_cast<std::uint32_t>(e.current_rip_);
	switch (e.imm_size_) {
	case ImmSize::None:
		break;

	case ImmSize::Size1:
	case ImmSize::SizeIbReg:
	case ImmSize::Size1OpCode:
		write_byte_internal(e, e.immediate_);
		break;

	case ImmSize::Size2:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;

	case ImmSize::Size4:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		break;

	case ImmSize::Size8:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		value = e.immediate_hi_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		break;

	case ImmSize::Size2_1:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, e.immediate_hi_);
		break;

	case ImmSize::Size1_1:
		write_byte_internal(e, e.immediate_);
		write_byte_internal(e, e.immediate_hi_);
		break;

	case ImmSize::Size2_2:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		value = e.immediate_hi_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;

	case ImmSize::Size4_2:
		value = e.immediate_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		value = e.immediate_hi_;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;

	case ImmSize::RipRelSize1_Target16: {
		const std::uint16_t ip = static_cast<std::uint16_t>(static_cast<std::uint32_t>(e.current_rip_) + 1);
		const std::int16_t diff2 = static_cast<std::int16_t>(static_cast<std::uint16_t>(e.immediate_) - ip);
		if (diff2 < std::numeric_limits<std::int8_t>::min() || diff2 > std::numeric_limits<std::int8_t>::max()) {
			set_distance_error(e, "Branch distance", ip, 4, static_cast<std::uint16_t>(e.immediate_), 4, diff2, "i8");
		}
		write_byte_internal(e, static_cast<std::uint32_t>(static_cast<std::int32_t>(diff2)));
		break;
	}

	case ImmSize::RipRelSize1_Target32: {
		const std::uint32_t eip = static_cast<std::uint32_t>(e.current_rip_) + 1;
		const std::int32_t diff4 = static_cast<std::int32_t>(e.immediate_ - eip);
		if (diff4 < std::numeric_limits<std::int8_t>::min() || diff4 > std::numeric_limits<std::int8_t>::max()) {
			set_distance_error(e, "Branch distance", eip, 8, e.immediate_, 8, diff4, "i8");
		}
		write_byte_internal(e, static_cast<std::uint32_t>(diff4));
		break;
	}

	case ImmSize::RipRelSize1_Target64: {
		const std::uint64_t rip = e.current_rip_ + 1;
		const std::uint64_t target = (static_cast<std::uint64_t>(e.immediate_hi_) << 32) | e.immediate_;
		const std::int64_t diff8 = static_cast<std::int64_t>(target - rip);
		if (diff8 < std::numeric_limits<std::int8_t>::min() || diff8 > std::numeric_limits<std::int8_t>::max()) {
			set_distance_error(e, "Branch distance", rip, 16, target, 16, diff8, "i8");
		}
		write_byte_internal(e, static_cast<std::uint32_t>(diff8));
		break;
	}

	case ImmSize::RipRelSize2_Target16: {
		const std::uint32_t eip = static_cast<std::uint32_t>(e.current_rip_) + 2;
		value = e.immediate_ - eip;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;
	}

	case ImmSize::RipRelSize2_Target32: {
		const std::uint32_t eip = static_cast<std::uint32_t>(e.current_rip_) + 2;
		const std::int32_t diff4 = static_cast<std::int32_t>(e.immediate_ - eip);
		if (diff4 < std::numeric_limits<std::int16_t>::min() || diff4 > std::numeric_limits<std::int16_t>::max()) {
			set_distance_error(e, "Branch distance", eip, 8, e.immediate_, 8, diff4, "i16");
		}
		value = static_cast<std::uint32_t>(diff4);
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;
	}

	case ImmSize::RipRelSize2_Target64: {
		const std::uint64_t rip = e.current_rip_ + 2;
		const std::uint64_t target = (static_cast<std::uint64_t>(e.immediate_hi_) << 32) | e.immediate_;
		const std::int64_t diff8 = static_cast<std::int64_t>(target - rip);
		if (diff8 < std::numeric_limits<std::int16_t>::min() || diff8 > std::numeric_limits<std::int16_t>::max()) {
			set_distance_error(e, "Branch distance", rip, 16, target, 16, diff8, "i16");
		}
		value = static_cast<std::uint32_t>(diff8);
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		break;
	}

	case ImmSize::RipRelSize4_Target32: {
		const std::uint32_t eip = static_cast<std::uint32_t>(e.current_rip_) + 4;
		value = e.immediate_ - eip;
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		break;
	}

	case ImmSize::RipRelSize4_Target64: {
		const std::uint64_t rip = e.current_rip_ + 4;
		const std::uint64_t target = (static_cast<std::uint64_t>(e.immediate_hi_) << 32) | e.immediate_;
		const std::int64_t diff8 = static_cast<std::int64_t>(target - rip);
		if (diff8 < std::numeric_limits<std::int32_t>::min() || diff8 > std::numeric_limits<std::int32_t>::max()) {
			set_distance_error(e, "Branch distance", rip, 16, target, 16, diff8, "i32");
		}
		value = static_cast<std::uint32_t>(diff8);
		write_byte_internal(e, value);
		write_byte_internal(e, value >> 8);
		write_byte_internal(e, value >> 16);
		write_byte_internal(e, value >> 24);
		break;
	}
	}
}

} // namespace internal
} // namespace iced_x86
