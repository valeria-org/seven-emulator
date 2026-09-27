// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/register.hpp"
#include "internal/block_encoder/instr.hpp"

namespace iced_x86::internal {

void IpRelMemOpInstr::create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction) {
	ICED_DEBUG_ASSERT(instruction.is_ip_rel_memory_operand());

	Instruction instr_copy = instruction;
	instr_copy.set_memory_base(Register::RIP);
	instr_copy.set_memory_displacement64(0);
	const auto rip_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, instr_copy.ip_rel_memory_address()));

	instr_copy.set_memory_base(Register::EIP);
	const auto eip_instruction_size = static_cast<std::uint8_t>(block_encoder.get_instruction_size(instr_copy, instr_copy.ip_rel_memory_address()));

	base.size = eip_instruction_size;
	ICED_DEBUG_ASSERT(eip_instruction_size >= rip_instruction_size);
	self.type = InstrType::IpRelMemOp;
	self.instruction = instruction;
	self.target_instr = TargetInstr();
	self.u.ip_rel = IpRelMemOpInstrData{IpRelMemOpInstrKind::Uninitialized, eip_instruction_size, rip_instruction_size};
}

bool IpRelMemOpInstr::optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained) {
	auto& d = self.u.ip_rel;
	if (d.instr_kind == IpRelMemOpInstrKind::Unchanged || d.instr_kind == IpRelMemOpInstrKind::Rip || d.instr_kind == IpRelMemOpInstrKind::Eip) {
		base.done = true;
		return false;
	}

	// If it's in the same block, we assume the target is at most 2GB away.
	bool use_rip = self.target_instr.is_in_block(ctx.block);
	const std::uint64_t target_address = self.target_instr.address(ctx);
	if (!use_rip) {
		const std::uint64_t next_rip = ctx.ip + d.rip_instruction_size;
		auto diff = static_cast<std::int64_t>(target_address - next_rip);
		diff = correct_diff(self.target_instr.is_in_block(ctx.block), diff, gained);
		use_rip = INT32_MIN <= diff && diff <= INT32_MAX;
	}

	if (use_rip) {
		base.size = d.rip_instruction_size;
		d.instr_kind = IpRelMemOpInstrKind::Rip;
		base.done = true;
		return true;
	}

	// If it's in the low 4GB we can use EIP relative addressing
	if (target_address <= UINT32_MAX) {
		base.size = d.eip_instruction_size;
		d.instr_kind = IpRelMemOpInstrKind::Eip;
		base.done = true;
		return true;
	}

	d.instr_kind = IpRelMemOpInstrKind::Long;
	return false;
}

Result<InstrEncodeResult> IpRelMemOpInstr::encode(Instr& self, InstrBase& /*base*/, InstrContext& ctx) {
	auto& d = self.u.ip_rel;
	switch (d.instr_kind) {
	case IpRelMemOpInstrKind::Unchanged:
	case IpRelMemOpInstrKind::Rip:
	case IpRelMemOpInstrKind::Eip: {
		if (d.instr_kind == IpRelMemOpInstrKind::Rip)
			self.instruction.set_memory_base(Register::RIP);
		else if (d.instr_kind == IpRelMemOpInstrKind::Eip)
			self.instruction.set_memory_base(Register::EIP);
		else
			ICED_DEBUG_ASSERT(d.instr_kind == IpRelMemOpInstrKind::Unchanged);

		const std::uint64_t target_address = self.target_instr.address(ctx);
		self.instruction.set_memory_displacement64(target_address);
		auto result = ctx.block.encoder.encode(self.instruction, ctx.ip);
		if (result.is_err())
			return InstrUtils::create_error_message(result.error(), self.instruction);
		const std::uint64_t expected_rip =
			self.instruction.memory_base() == Register::EIP ? static_cast<std::uint64_t>(static_cast<std::uint32_t>(target_address)) : target_address;
		if (self.instruction.ip_rel_memory_address() != expected_rip)
			return InstrUtils::create_error_message("Invalid IP relative address", self.instruction);
		return InstrEncodeResult{ctx.block.encoder.get_constant_offsets(), true};
	}

	case IpRelMemOpInstrKind::Long:
		return IcedError("IP relative memory operand is too far away and isn't currently supported. "
						 "Try to allocate memory close to the original instruction (+/-2GB).");

	case IpRelMemOpInstrKind::Uninitialized:
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace iced_x86::internal
