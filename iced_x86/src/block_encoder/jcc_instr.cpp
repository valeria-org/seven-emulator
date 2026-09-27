// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include <algorithm>

#include "iced_x86/code_ext.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/op_kind.hpp"
#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

static std::uint32_t long_instruction_size64(const Instruction& instruction) noexcept {
	// Check if JKZD/JKNZD
	if (instruction.op_count() == 2)
		return 5 + InstrUtils::CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64;
	// Code:
	//		!jcc short skip		; negated jcc opcode
	//		jmp qword ptr [rip+mem]
	//	skip:
	return 2 + InstrUtils::CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64;
}

static Code short_br_to_native_br(Code code, std::uint32_t bitness) noexcept {
	Code c16, c32, c64;
	switch (code) {
	case Code::Jo_rel8_16:
	case Code::Jo_rel8_32:
	case Code::Jo_rel8_64:
		c16 = Code::Jo_rel8_16;
		c32 = Code::Jo_rel8_32;
		c64 = Code::Jo_rel8_64;
		break;
	case Code::Jno_rel8_16:
	case Code::Jno_rel8_32:
	case Code::Jno_rel8_64:
		c16 = Code::Jno_rel8_16;
		c32 = Code::Jno_rel8_32;
		c64 = Code::Jno_rel8_64;
		break;
	case Code::Jb_rel8_16:
	case Code::Jb_rel8_32:
	case Code::Jb_rel8_64:
		c16 = Code::Jb_rel8_16;
		c32 = Code::Jb_rel8_32;
		c64 = Code::Jb_rel8_64;
		break;
	case Code::Jae_rel8_16:
	case Code::Jae_rel8_32:
	case Code::Jae_rel8_64:
		c16 = Code::Jae_rel8_16;
		c32 = Code::Jae_rel8_32;
		c64 = Code::Jae_rel8_64;
		break;
	case Code::Je_rel8_16:
	case Code::Je_rel8_32:
	case Code::Je_rel8_64:
		c16 = Code::Je_rel8_16;
		c32 = Code::Je_rel8_32;
		c64 = Code::Je_rel8_64;
		break;
	case Code::Jne_rel8_16:
	case Code::Jne_rel8_32:
	case Code::Jne_rel8_64:
		c16 = Code::Jne_rel8_16;
		c32 = Code::Jne_rel8_32;
		c64 = Code::Jne_rel8_64;
		break;
	case Code::Jbe_rel8_16:
	case Code::Jbe_rel8_32:
	case Code::Jbe_rel8_64:
		c16 = Code::Jbe_rel8_16;
		c32 = Code::Jbe_rel8_32;
		c64 = Code::Jbe_rel8_64;
		break;
	case Code::Ja_rel8_16:
	case Code::Ja_rel8_32:
	case Code::Ja_rel8_64:
		c16 = Code::Ja_rel8_16;
		c32 = Code::Ja_rel8_32;
		c64 = Code::Ja_rel8_64;
		break;
	case Code::Js_rel8_16:
	case Code::Js_rel8_32:
	case Code::Js_rel8_64:
		c16 = Code::Js_rel8_16;
		c32 = Code::Js_rel8_32;
		c64 = Code::Js_rel8_64;
		break;
	case Code::Jns_rel8_16:
	case Code::Jns_rel8_32:
	case Code::Jns_rel8_64:
		c16 = Code::Jns_rel8_16;
		c32 = Code::Jns_rel8_32;
		c64 = Code::Jns_rel8_64;
		break;
	case Code::Jp_rel8_16:
	case Code::Jp_rel8_32:
	case Code::Jp_rel8_64:
		c16 = Code::Jp_rel8_16;
		c32 = Code::Jp_rel8_32;
		c64 = Code::Jp_rel8_64;
		break;
	case Code::Jnp_rel8_16:
	case Code::Jnp_rel8_32:
	case Code::Jnp_rel8_64:
		c16 = Code::Jnp_rel8_16;
		c32 = Code::Jnp_rel8_32;
		c64 = Code::Jnp_rel8_64;
		break;
	case Code::Jl_rel8_16:
	case Code::Jl_rel8_32:
	case Code::Jl_rel8_64:
		c16 = Code::Jl_rel8_16;
		c32 = Code::Jl_rel8_32;
		c64 = Code::Jl_rel8_64;
		break;
	case Code::Jge_rel8_16:
	case Code::Jge_rel8_32:
	case Code::Jge_rel8_64:
		c16 = Code::Jge_rel8_16;
		c32 = Code::Jge_rel8_32;
		c64 = Code::Jge_rel8_64;
		break;
	case Code::Jle_rel8_16:
	case Code::Jle_rel8_32:
	case Code::Jle_rel8_64:
		c16 = Code::Jle_rel8_16;
		c32 = Code::Jle_rel8_32;
		c64 = Code::Jle_rel8_64;
		break;
	case Code::Jg_rel8_16:
	case Code::Jg_rel8_32:
	case Code::Jg_rel8_64:
		c16 = Code::Jg_rel8_16;
		c32 = Code::Jg_rel8_32;
		c64 = Code::Jg_rel8_64;
		break;
	default:
		if (bitness == 64) {
			switch (code) {
			case Code::VEX_KNC_Jkzd_kr_rel8_64:
			case Code::VEX_KNC_Jknzd_kr_rel8_64:
				return code;
			default:
				break;
			}
		}
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

void JccInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	auto instr_kind = JccInstrKind::Uninitialized;
	Instruction instr_copy;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	const auto long_instruction_size64_ = static_cast<std::uint8_t>(long_instruction_size64(instruction));
	if (!block_encoder.fix_branches()) {
		instr_kind = JccInstrKind::Unchanged;
		instr_copy = instruction;
		instr_copy.set_near_branch64(0);
		base.size = block_encoder.get_instruction_size(instr_copy, 0);
		short_instruction_size = 0;
		near_instruction_size = 0;
	} else {
		instr_copy = instruction;
		instr_copy.set_code(code_ext::as_short_branch(instruction.code()));
		instr_copy.set_near_branch64(0);
		short_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));

		instr_copy = instruction;
		instr_copy.set_code(code_ext::as_near_branch(instruction.code()));
		instr_copy.set_near_branch64(0);
		near_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));

		if (block_encoder.bitness == 64) {
			// Make sure it's not shorter than the real instruction. It can happen if there are extra prefixes.
			base.size = std::max(near_instruction_size, long_instruction_size64_);
		} else
			base.size = near_instruction_size;
	}
	self.type = InstrType::Jcc;
	self.bitness = static_cast<std::uint8_t>(block_encoder.bitness);
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.pointer_data = NO_POINTER_DATA;
	self.u.jcc = JccInstrData{instr_kind, short_instruction_size, near_instruction_size, long_instruction_size64_};
}

bool JccInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.jcc;
	if (d.instr_kind == JccInstrKind::Unchanged || d.instr_kind == JccInstrKind::Short) {
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
		d.instr_kind = JccInstrKind::Short;
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
		d.instr_kind = JccInstrKind::Near;
		base.size = d.near_instruction_size;
		return true;
	}

	if (self.pointer_data == NO_POINTER_DATA)
		self.pointer_data = ctx.block.alloc_pointer_location();
	d.instr_kind = JccInstrKind::Long;
	return false;
}

Result<InstrEncodeResult> JccInstr::encode(Instr& self, InstrBase& base, InstrContext& ctx) {
	auto& d = self.u.jcc;
	switch (d.instr_kind) {
	case JccInstrKind::Unchanged:
	case JccInstrKind::Short:
	case JccInstrKind::Near: {
		if (d.instr_kind == JccInstrKind::Unchanged) {
			// nothing
		} else if (d.instr_kind == JccInstrKind::Short) {
			self.instruction.set_code(code_ext::as_short_branch(self.instruction.code()));
		} else {
			ICED_DEBUG_ASSERT(d.instr_kind == JccInstrKind::Near);
			self.instruction.set_code(code_ext::as_near_branch(self.instruction.code()));
		}
		self.instruction.set_near_branch64(self.target_instr.address(ctx));
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	}

	case JccInstrKind::Long: {
		ICED_DEBUG_ASSERT(self.pointer_data != NO_POINTER_DATA);
		if (self.pointer_data == NO_POINTER_DATA)
			return IcedError("Internal error");
		ctx.block.data(self.pointer_data).data = self.target_instr.address(ctx);
		Instruction instr;
		instr.set_code(short_br_to_native_br(code_ext::as_short_branch(code_ext::negate_condition_code(self.instruction.code())),
			ctx.block.encoder.bitness()));
		if (self.instruction.op_count() == 1)
			instr.set_op0_kind(OpKind::NearBranch64);
		else {
			ICED_DEBUG_ASSERT(self.instruction.op_count() == 2);
			instr.set_op0_kind(OpKind::Register);
			instr.set_op0_register(self.instruction.op0_register());
			instr.set_op1_kind(OpKind::NearBranch64);
		}
		ICED_DEBUG_ASSERT(ctx.block.encoder.bitness() == 64);
		ICED_DEBUG_ASSERT(d.long_instruction_size64 <= INT8_MAX);
		instr.set_near_branch64(ctx.ip + d.long_instruction_size64);
		auto instr_len_result = ctx.block.encoder.encode(instr, ctx.ip);
		if (instr_len_result.is_err())
			return InstrUtils::create_error_message(instr_len_result.error(), self.instruction);
		const auto instr_len = static_cast<std::uint32_t>(instr_len_result.value());
		auto result = InstrUtils::encode_branch_to_pointer_data(ctx.block, false, ctx.ip + instr_len, self.pointer_data, base.size - instr_len);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ConstantOffsets{}, false};
	}

	case JccInstrKind::Uninitialized:
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace iced_x86::internal
