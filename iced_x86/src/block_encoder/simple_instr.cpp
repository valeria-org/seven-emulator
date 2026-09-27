// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

void SimpleInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	base.done = true;
	base.size = block_encoder.get_instruction_size(instruction, instruction.ip());
	self.type = InstrType::Simple;
	self.instruction = instruction;
}

Result<InstrEncodeResult> SimpleInstr::encode(Instr& self, InstrBase& /*base*/, InstrContext& ctx) {
	auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
	if (result.is_err())
		return InstrUtils::create_error_message(result.error(), self.instruction);
	return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
}

} // namespace iced_x86::internal
