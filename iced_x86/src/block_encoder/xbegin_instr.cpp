// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

void XbeginInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	auto instr_kind = XbeginInstrKind::Uninitialized;
	Instruction instr_copy;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	if (!block_encoder.fix_branches()) {
		instr_kind = XbeginInstrKind::Unchanged;
		instr_copy = instruction;
		instr_copy.set_near_branch64(0);
		base.size = block_encoder.get_instruction_size(instr_copy, 0);
		short_instruction_size = 0;
		near_instruction_size = 0;
	} else {
		instr_copy = instruction;
		instr_copy.set_code(Code::Xbegin_rel16);
		instr_copy.set_near_branch64(0);
		short_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));

		instr_copy = instruction;
		instr_copy.set_code(Code::Xbegin_rel32);
		instr_copy.set_near_branch64(0);
		near_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, 0));

		base.size = near_instruction_size;
	}
	self.type = InstrType::Xbegin;
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.u.xbegin = XbeginInstrData{instr_kind, short_instruction_size, near_instruction_size};
}

bool XbeginInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.xbegin;
	if (d.instr_kind == XbeginInstrKind::Unchanged || d.instr_kind == XbeginInstrKind::Rel16) {
		base.done = true;
		return false;
	}

	const std::uint64_t target_address = self.target_instr.address(ctx);
	const std::uint64_t next_rip = ctx.ip + d.short_instruction_size;
	auto diff = static_cast<std::int64_t>(target_address - next_rip);
	diff = correct_diff(self.target_instr.is_in_block(ctx.block), diff, gained);
	if (INT16_MIN <= diff && diff <= INT16_MAX) {
		d.instr_kind = XbeginInstrKind::Rel16;
		base.size = d.short_instruction_size;
		return true;
	} else {
		d.instr_kind = XbeginInstrKind::Rel32;
		base.size = d.near_instruction_size;
		return false;
	}
}

Result<InstrEncodeResult> XbeginInstr::encode(Instr& self, InstrBase& /*base*/, InstrContext& ctx) {
	auto& d = self.u.xbegin;
	switch (d.instr_kind) {
	case XbeginInstrKind::Unchanged:
	case XbeginInstrKind::Rel16:
	case XbeginInstrKind::Rel32: {
		if (d.instr_kind == XbeginInstrKind::Unchanged) {
			// nothing
		} else if (d.instr_kind == XbeginInstrKind::Rel16) {
			self.instruction.set_code(Code::Xbegin_rel16);
		} else {
			ICED_DEBUG_ASSERT(d.instr_kind == XbeginInstrKind::Rel32);
			self.instruction.set_code(Code::Xbegin_rel32);
		}
		self.instruction.set_near_branch64(self.target_instr.address(ctx));
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	}

	case XbeginInstrKind::Uninitialized:
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace iced_x86::internal
