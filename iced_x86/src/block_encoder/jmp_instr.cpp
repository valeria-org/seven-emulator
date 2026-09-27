// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include <algorithm>

#include "iced_x86/code_ext.hpp"
#include "iced_x86/iced_constants.hpp"
#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

void JmpInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	auto instr_kind = JmpInstrKind::Uninitialized;
	Instruction instr_copy;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	if (!block_encoder.fix_branches()) {
		instr_kind = JmpInstrKind::Unchanged;
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
			base.size = std::max<std::uint32_t>(near_instruction_size, InstrUtils::CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64);
		} else
			base.size = near_instruction_size;
	}
	self.type = InstrType::Jmp;
	self.bitness = static_cast<std::uint8_t>(block_encoder.bitness);
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.pointer_data = NO_POINTER_DATA;
	self.u.jmp = JmpInstrData{instr_kind, short_instruction_size, near_instruction_size};
}

bool JmpInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.jmp;
	if (d.instr_kind == JmpInstrKind::Unchanged || d.instr_kind == JmpInstrKind::Short) {
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
		d.instr_kind = JmpInstrKind::Short;
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
		d.instr_kind = JmpInstrKind::Near;
		base.size = d.near_instruction_size;
		return true;
	}

	if (self.pointer_data == NO_POINTER_DATA)
		self.pointer_data = ctx.block.alloc_pointer_location();
	d.instr_kind = JmpInstrKind::Long;
	return false;
}

Result<InstrEncodeResult> JmpInstr::encode(Instr& self, InstrBase& base, InstrContext& ctx) {
	auto& d = self.u.jmp;
	switch (d.instr_kind) {
	case JmpInstrKind::Unchanged:
	case JmpInstrKind::Short:
	case JmpInstrKind::Near: {
		if (d.instr_kind == JmpInstrKind::Unchanged) {
			// nothing
		} else if (d.instr_kind == JmpInstrKind::Short) {
			self.instruction.set_code(code_ext::as_short_branch(self.instruction.code()));
		} else {
			ICED_DEBUG_ASSERT(d.instr_kind == JmpInstrKind::Near);
			self.instruction.set_code(code_ext::as_near_branch(self.instruction.code()));
		}
		self.instruction.set_near_branch64(self.target_instr.address(ctx));
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	}

	case JmpInstrKind::Long: {
		ICED_DEBUG_ASSERT(self.pointer_data != NO_POINTER_DATA);
		if (self.pointer_data == NO_POINTER_DATA)
			return IcedError("Internal error");
		ctx.block.data(self.pointer_data).data = self.target_instr.address(ctx);
		auto result = InstrUtils::encode_branch_to_pointer_data(ctx.block, false, ctx.ip, self.pointer_data, base.size);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ConstantOffsets{}, false};
	}

	case JmpInstrKind::Uninitialized:
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace iced_x86::internal
