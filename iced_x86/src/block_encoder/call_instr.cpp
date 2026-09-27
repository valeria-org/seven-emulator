// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include <algorithm>

#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

void CallInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	Instruction instr_copy = instruction;
	instr_copy.set_near_branch64(0);
	const auto orig_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));
	bool use_orig_instruction = false;
	if (!block_encoder.fix_branches()) {
		use_orig_instruction = true;
		base.size = orig_instruction_size;
	} else if (block_encoder.bitness == 64) {
		// Make sure it's not shorter than the real instruction. It can happen if there are extra prefixes.
		base.size = std::max<std::uint32_t>(orig_instruction_size, InstrUtils::CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64);
	} else {
		base.size = orig_instruction_size;
	}
	self.type = InstrType::Call;
	self.bitness = static_cast<std::uint8_t>(block_encoder.bitness);
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.pointer_data = NO_POINTER_DATA;
	self.u.call = CallInstrData{orig_instruction_size, use_orig_instruction};
}

bool CallInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.call;
	if (base.done || d.use_orig_instruction) {
		base.done = true;
		return false;
	}

	// If it's in the same block, we assume the target is at most 2GB away.
	bool use_short = self.bitness != 64 || self.target_instr.is_in_block(ctx.block);
	if (!use_short) {
		const std::uint64_t target_address = self.target_instr.address(ctx);
		const std::uint64_t next_rip = ctx.ip + d.orig_instruction_size;
		auto diff = static_cast<std::int64_t>(target_address - next_rip);
		diff = correct_diff(self.target_instr.is_in_block(ctx.block), diff, gained);
		use_short = INT32_MIN <= diff && diff <= INT32_MAX;
	}

	if (use_short) {
		if (self.pointer_data != NO_POINTER_DATA)
			ctx.block.data(self.pointer_data).is_valid = false;
		base.size = d.orig_instruction_size;
		d.use_orig_instruction = true;
		base.done = true;
		return true;
	}

	if (self.pointer_data == NO_POINTER_DATA)
		self.pointer_data = ctx.block.alloc_pointer_location();
	return false;
}

Result<InstrEncodeResult> CallInstr::encode(Instr& self, InstrBase& base, InstrContext& ctx) {
	if (self.u.call.use_orig_instruction) {
		self.instruction.set_near_branch64(self.target_instr.address(ctx));
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	} else {
		ICED_DEBUG_ASSERT(self.pointer_data != NO_POINTER_DATA);
		if (self.pointer_data == NO_POINTER_DATA)
			return IcedError("Internal error");
		ctx.block.data(self.pointer_data).data = self.target_instr.address(ctx);
		auto result = InstrUtils::encode_branch_to_pointer_data(ctx.block, true, ctx.ip, self.pointer_data, base.size);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ConstantOffsets{}, false};
	}
}

} // namespace iced_x86::internal
