// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include <algorithm>

#include "iced_x86/iced_constants.hpp"
#include "iced_x86/op_kind.hpp"
#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

static Code as_native_branch_code(Code code, std::uint32_t bitness) noexcept {
	Code c16, c32, c64;
	switch (code) {
	case Code::Loopne_rel8_16_CX:
	case Code::Loopne_rel8_32_CX:
		c16 = Code::Loopne_rel8_16_CX;
		c32 = Code::Loopne_rel8_32_CX;
		c64 = Code::INVALID;
		break;
	case Code::Loopne_rel8_16_ECX:
	case Code::Loopne_rel8_32_ECX:
	case Code::Loopne_rel8_64_ECX:
		c16 = Code::Loopne_rel8_16_ECX;
		c32 = Code::Loopne_rel8_32_ECX;
		c64 = Code::Loopne_rel8_64_ECX;
		break;
	case Code::Loopne_rel8_16_RCX:
	case Code::Loopne_rel8_64_RCX:
		c16 = Code::Loopne_rel8_16_RCX;
		c32 = Code::INVALID;
		c64 = Code::Loopne_rel8_64_RCX;
		break;
	case Code::Loope_rel8_16_CX:
	case Code::Loope_rel8_32_CX:
		c16 = Code::Loope_rel8_16_CX;
		c32 = Code::Loope_rel8_32_CX;
		c64 = Code::INVALID;
		break;
	case Code::Loope_rel8_16_ECX:
	case Code::Loope_rel8_32_ECX:
	case Code::Loope_rel8_64_ECX:
		c16 = Code::Loope_rel8_16_ECX;
		c32 = Code::Loope_rel8_32_ECX;
		c64 = Code::Loope_rel8_64_ECX;
		break;
	case Code::Loope_rel8_16_RCX:
	case Code::Loope_rel8_64_RCX:
		c16 = Code::Loope_rel8_16_RCX;
		c32 = Code::INVALID;
		c64 = Code::Loope_rel8_64_RCX;
		break;
	case Code::Loop_rel8_16_CX:
	case Code::Loop_rel8_32_CX:
		c16 = Code::Loop_rel8_16_CX;
		c32 = Code::Loop_rel8_32_CX;
		c64 = Code::INVALID;
		break;
	case Code::Loop_rel8_16_ECX:
	case Code::Loop_rel8_32_ECX:
	case Code::Loop_rel8_64_ECX:
		c16 = Code::Loop_rel8_16_ECX;
		c32 = Code::Loop_rel8_32_ECX;
		c64 = Code::Loop_rel8_64_ECX;
		break;
	case Code::Loop_rel8_16_RCX:
	case Code::Loop_rel8_64_RCX:
		c16 = Code::Loop_rel8_16_RCX;
		c32 = Code::INVALID;
		c64 = Code::Loop_rel8_64_RCX;
		break;
	case Code::Jcxz_rel8_16:
	case Code::Jcxz_rel8_32:
		c16 = Code::Jcxz_rel8_16;
		c32 = Code::Jcxz_rel8_32;
		c64 = Code::INVALID;
		break;
	case Code::Jecxz_rel8_16:
	case Code::Jecxz_rel8_32:
	case Code::Jecxz_rel8_64:
		c16 = Code::Jecxz_rel8_16;
		c32 = Code::Jecxz_rel8_32;
		c64 = Code::Jecxz_rel8_64;
		break;
	case Code::Jrcxz_rel8_16:
	case Code::Jrcxz_rel8_64:
		c16 = Code::Jrcxz_rel8_16;
		c32 = Code::INVALID;
		c64 = Code::Jrcxz_rel8_64;
		break;
	default:
		ICED_UNREACHABLE();
	}

	switch (bitness) {
	case 16:
		return c16;
	case 32:
		return c32;
	case 64:
		return c64;
	default:
		ICED_UNREACHABLE();
	}
}

void SimpleBranchInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	auto instr_kind = SimpleBranchInstrKind::Uninitialized;
	Instruction instr_copy;
	Code native_code;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	std::uint8_t long_instruction_size;
	std::uint8_t native_instruction_size;
	if (!block_encoder.fix_branches()) {
		instr_kind = SimpleBranchInstrKind::Unchanged;
		instr_copy = instruction;
		instr_copy.set_near_branch64(0);
		base.size = block_encoder.get_instruction_size(instr_copy, 0);
		native_code = Code::INVALID;
		short_instruction_size = 0;
		near_instruction_size = 0;
		long_instruction_size = 0;
		native_instruction_size = 0;
	} else {
		instr_copy = instruction;
		instr_copy.set_near_branch64(0);
		short_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));

		native_code = as_native_branch_code(instruction.code(), block_encoder.bitness);
		if (native_code == instruction.code())
			native_instruction_size = short_instruction_size;
		else {
			instr_copy = instruction;
			instr_copy.set_code(native_code);
			instr_copy.set_near_branch64(0);
			native_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));
		}

		switch (block_encoder.bitness) {
		case 16:
			near_instruction_size = static_cast<std::uint8_t>(native_instruction_size + 2 + 3);
			break;
		case 32:
		case 64:
			near_instruction_size = static_cast<std::uint8_t>(native_instruction_size + 2 + 5);
			break;
		default:
			ICED_UNREACHABLE();
		}

		if (block_encoder.bitness == 64) {
			long_instruction_size =
				static_cast<std::uint8_t>(native_instruction_size + 2 + InstrUtils::CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64);
			base.size = std::max(std::max(short_instruction_size, near_instruction_size), long_instruction_size);
		} else {
			long_instruction_size = 0;
			base.size = std::max(short_instruction_size, near_instruction_size);
		}
	}
	self.type = InstrType::SimpleBranch;
	self.bitness = static_cast<std::uint8_t>(block_encoder.bitness);
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.pointer_data = NO_POINTER_DATA;
	self.u.simple_br = SimpleBranchInstrData{
		instr_kind, short_instruction_size, near_instruction_size, long_instruction_size, native_instruction_size, native_code,
	};
}

bool SimpleBranchInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.simple_br;
	if (d.instr_kind == SimpleBranchInstrKind::Unchanged || d.instr_kind == SimpleBranchInstrKind::Short) {
		base.done = true;
		return false;
	}

	std::uint64_t target_address = self.target_instr.address(ctx);
	std::uint64_t next_rip = ctx.ip + d.short_instruction_size;
	auto diff = static_cast<std::int64_t>(target_address - next_rip);
	diff = InstrUtils::convert_diff_to_bitness_diff(self.bitness, correct_diff(self.target_instr.is_in_block(ctx.block), diff, gained));
	if (INT8_MIN <= diff && diff <= INT8_MAX) {
		if (self.pointer_data != NO_POINTER_DATA)
			ctx.block.data(self.pointer_data).is_valid = false;
		d.instr_kind = SimpleBranchInstrKind::Short;
		base.size = d.short_instruction_size;
		base.done = true;
		return true;
	}

	// If it's in the same block, we assume the target is at most 2GB away.
	bool use_near = self.bitness != 64 || self.target_instr.is_in_block(ctx.block);
	if (!use_near) {
		target_address = self.target_instr.address(ctx);
		next_rip = ctx.ip + d.near_instruction_size;
		diff = static_cast<std::int64_t>(target_address - next_rip);
		diff = InstrUtils::convert_diff_to_bitness_diff(self.bitness, correct_diff(self.target_instr.is_in_block(ctx.block), diff, gained));
		use_near = INT32_MIN <= diff && diff <= INT32_MAX;
	}
	if (use_near) {
		if (self.pointer_data != NO_POINTER_DATA)
			ctx.block.data(self.pointer_data).is_valid = false;
		if (diff < static_cast<std::int64_t>(IcedConstants::MAX_INSTRUCTION_LENGTH) * INT8_MIN ||
			diff > static_cast<std::int64_t>(IcedConstants::MAX_INSTRUCTION_LENGTH) * INT8_MAX)
			base.done = true;
		d.instr_kind = SimpleBranchInstrKind::Near;
		base.size = d.near_instruction_size;
		return true;
	}

	if (self.pointer_data == NO_POINTER_DATA)
		self.pointer_data = ctx.block.alloc_pointer_location();
	d.instr_kind = SimpleBranchInstrKind::Long;
	return false;
}

// Encodes `brins tmp ; jmp short skip` (the first two instructions of the Near and Long forms) and returns its size.
// `total_size` is the size of the whole Near/Long form (the address of `skip`).
static Result<std::uint32_t> encode_native_br_and_jmp_short(Instr& self, InstrContext& ctx, std::uint32_t total_size) {
	const auto& d = self.u.simple_br;
	Instruction instr = self.instruction;
	instr.set_code(d.native_code);
	instr.set_near_branch64(ctx.ip + d.native_instruction_size + 2);
	auto size_result = ctx.block.encoder.encode(instr, ctx.ip);
	if (size_result.is_err())
		return InstrUtils::create_error_message(size_result.error(), self.instruction);
	auto size = static_cast<std::uint32_t>(size_result.value());

	instr = Instruction();
	instr.set_near_branch64(ctx.ip + total_size);
	switch (ctx.block.encoder.bitness()) {
	case 16:
		instr.set_code(Code::Jmp_rel8_16);
		instr.set_op0_kind(OpKind::NearBranch16);
		break;

	case 32:
		instr.set_code(Code::Jmp_rel8_32);
		instr.set_op0_kind(OpKind::NearBranch32);
		break;

	case 64:
		instr.set_code(Code::Jmp_rel8_64);
		instr.set_op0_kind(OpKind::NearBranch64);
		break;

	default:
		ICED_UNREACHABLE();
	}
	auto instr_len_result = ctx.block.encoder.encode(instr, ctx.ip + size);
	if (instr_len_result.is_err())
		return InstrUtils::create_error_message(instr_len_result.error(), self.instruction);
	size += static_cast<std::uint32_t>(instr_len_result.value());
	return size;
}

Result<InstrEncodeResult> SimpleBranchInstr::encode(Instr& self, InstrBase& base, InstrContext& ctx) {
	auto& d = self.u.simple_br;
	switch (d.instr_kind) {
	case SimpleBranchInstrKind::Unchanged:
	case SimpleBranchInstrKind::Short: {
		self.instruction.set_near_branch64(self.target_instr.address(ctx));
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	}

	case SimpleBranchInstrKind::Near: {
		// Code:
		//		brins tmp		; native_instruction_size
		//		jmp short skip	; 2
		//	tmp:
		//		jmp near target	; 3/5/5
		//	skip:

		auto size_result = encode_native_br_and_jmp_short(self, ctx, d.near_instruction_size);
		if (size_result.is_err())
			return size_result.error();
		const std::uint32_t size = size_result.value();

		Instruction instr;
		instr.set_near_branch64(self.target_instr.address(ctx));
		switch (ctx.block.encoder.bitness()) {
		case 16:
			instr.set_code(Code::Jmp_rel16);
			instr.set_op0_kind(OpKind::NearBranch16);
			break;

		case 32:
			instr.set_code(Code::Jmp_rel32_32);
			instr.set_op0_kind(OpKind::NearBranch32);
			break;

		case 64:
			instr.set_code(Code::Jmp_rel32_64);
			instr.set_op0_kind(OpKind::NearBranch64);
			break;

		default:
			ICED_UNREACHABLE();
		}
		auto result = ctx.block.encoder.encode(instr, ctx.ip + size);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ConstantOffsets{}, false};
	}

	case SimpleBranchInstrKind::Long: {
		ICED_DEBUG_ASSERT(ctx.block.encoder.bitness() == 64);
		ICED_DEBUG_ASSERT(self.pointer_data != NO_POINTER_DATA);
		if (self.pointer_data == NO_POINTER_DATA)
			return IcedError("Internal error");
		ctx.block.data(self.pointer_data).data = self.target_instr.address(ctx);

		// Code:
		//		brins tmp		; native_instruction_size
		//		jmp short skip	; 2
		//	tmp:
		//		jmp [mem_loc]	; 6
		//	skip:

		auto size_result = encode_native_br_and_jmp_short(self, ctx, d.long_instruction_size);
		if (size_result.is_err())
			return size_result.error();
		const std::uint32_t size = size_result.value();

		auto result = InstrUtils::encode_branch_to_pointer_data(ctx.block, false, ctx.ip + size, self.pointer_data, base.size - size);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ConstantOffsets{}, false};
	}

	case SimpleBranchInstrKind::Uninitialized:
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace iced_x86::internal
