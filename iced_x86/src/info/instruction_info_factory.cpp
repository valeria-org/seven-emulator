// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/instruction_info.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/register_ext.hpp"
#include "internal/iced_assert.hpp"
#include "internal/info/implied_access.hpp"
#include "internal/info/info_flags1.hpp"
#include "internal/info/info_flags2.hpp"
#include "internal/info/info_tables.hpp"
#include "internal/info/instr_info_constants.hpp"
#include "internal/info/op_accesses.hpp"
#include "internal/info/op_info0.hpp"
#include "internal/info/op_info1.hpp"
#include "internal/info/op_info2.hpp"
#include "internal/instruction_internal.hpp"
#include <cstddef>
#include <cstdint>

namespace iced_x86 {
namespace internal {

namespace {

struct XspInfo {
	Register xsp;
	CodeSize code_size;
	std::uint64_t mask;
};

// Index = CodeSize
constexpr XspInfo XSP_TABLE[4] = {
	{Register::RSP, CodeSize::Code64, UINT64_MAX},
	{Register::SP, CodeSize::Code16, UINT16_MAX},
	{Register::ESP, CodeSize::Code32, UINT32_MAX},
	{Register::RSP, CodeSize::Code64, UINT64_MAX},
};
static_assert(static_cast<std::uint32_t>(CodeSize::Unknown) == 0, "");
static_assert(static_cast<std::uint32_t>(CodeSize::Code16) == 1, "");
static_assert(static_cast<std::uint32_t>(CodeSize::Code32) == 2, "");
static_assert(static_cast<std::uint32_t>(CodeSize::Code64) == 3, "");
static_assert(IcedConstants::CODE_SIZE_ENUM_COUNT == 4, "");

struct Flags {
	static constexpr std::uint32_t NO_MEMORY_USAGE = 0x0000'0001;
	static constexpr std::uint32_t NO_REGISTER_USAGE = 0x0000'0002;
	static constexpr std::uint32_t IS_64BIT = 0x0000'0004;
	static constexpr std::uint32_t ZERO_EXT_VEC_REGS = 0x0000'0008;
};
static_assert(InstructionInfoOptions::NO_MEMORY_USAGE == Flags::NO_MEMORY_USAGE, "");
static_assert(InstructionInfoOptions::NO_REGISTER_USAGE == Flags::NO_REGISTER_USAGE, "");

constexpr Register reg_add(Register reg, std::uint32_t value) noexcept {
	return static_cast<Register>(static_cast<std::uint32_t>(reg) + value);
}

} // namespace

struct InstructionInfoFactoryImpl {
	static const InstructionInfo& create(InstructionInfo& info, const Instruction& instruction, std::uint32_t options);

private:
	static const XspInfo& get_xsp(CodeSize code_size) noexcept { return XSP_TABLE[static_cast<std::size_t>(code_size)]; }

	static void add_implied_accesses(ImpliedAccess implied_access, const Instruction& instruction, InstructionInfo& info, std::uint32_t flags);

	static Register get_a_rdi(const Instruction& instruction) noexcept {
		switch (instruction.op0_kind()) {
		case OpKind::MemorySegDI:
			return Register::DI;
		case OpKind::MemorySegEDI:
			return Register::EDI;
		default:
			return Register::RDI;
		}
	}

	static Register get_seg_default_ds(const Instruction& instruction) noexcept {
		Register seg = instruction.segment_prefix();
		return seg == Register::None ? Register::DS : seg;
	}

	static MemorySize get_stack_memory_size(std::uint32_t op_size) noexcept {
		if (op_size == 8)
			return MemorySize::UInt64;
		if (op_size == 4)
			return MemorySize::UInt32;
		ICED_DEBUG_ASSERT(op_size == 2);
		return MemorySize::UInt16;
	}

	static void command_push(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t count, std::uint32_t op_size) {
		ICED_DEBUG_ASSERT(count > 0);
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp_info.xsp, OpAccess::ReadWrite);
		}
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			MemorySize mem_size = get_stack_memory_size(op_size);
			std::uint64_t offset = 0 - static_cast<std::uint64_t>(op_size);
			for (std::uint32_t i = 0; i < count; i++) {
				add_memory(info, Register::SS, xsp_info.xsp, Register::None, 1, offset & xsp_info.mask, mem_size, OpAccess::Write, xsp_info.code_size, 0);
				offset -= op_size;
			}
		}
	}

	static void command_pop(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t count, std::uint32_t op_size) {
		ICED_DEBUG_ASSERT(count > 0);
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp_info.xsp, OpAccess::ReadWrite);
		}
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			MemorySize mem_size = get_stack_memory_size(op_size);
			std::uint64_t offset = 0;
			for (std::uint32_t i = 0; i < count; i++) {
				add_memory(info, Register::SS, xsp_info.xsp, Register::None, 1, offset, mem_size, OpAccess::Read, xsp_info.code_size, 0);
				offset += op_size;
			}
		}
	}

	static void command_pop_rm(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t op_size) {
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp_info.xsp, OpAccess::ReadWrite);
		}
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			MemorySize memory_size = get_stack_memory_size(op_size);
			if (instruction.op0_kind() == OpKind::Memory) {
				ICED_DEBUG_ASSERT(info.used_memory_locations_.size() == 1);
				if (instruction.memory_base() == Register::RSP || instruction.memory_base() == Register::ESP) {
					UsedMemory& mem = info.used_memory_locations_[0];
					std::uint64_t displ = mem.displacement_ + op_size;
					if (instruction.memory_base() == Register::ESP)
						displ = static_cast<std::uint32_t>(displ);
					mem.displacement_ = displ;
				}
			}
			add_memory(info, Register::SS, xsp_info.xsp, Register::None, 1, 0, memory_size, OpAccess::Read, xsp_info.code_size, 0);
		}
	}

	static void command_pusha(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t op_size) {
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp_info.xsp, OpAccess::ReadWrite);
		}
		std::int64_t displ;
		MemorySize memory_size;
		Register base_register;
		if (op_size == 4) {
			displ = -4;
			memory_size = MemorySize::UInt32;
			base_register = Register::EAX;
		} else {
			ICED_DEBUG_ASSERT(op_size == 2);
			displ = -2;
			memory_size = MemorySize::UInt16;
			base_register = Register::AX;
		}
		for (std::uint32_t i = 0; i < 8; i++) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0)
				add_register(flags, info, reg_add(base_register, i), OpAccess::Read);
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				add_memory(info, Register::SS, xsp_info.xsp, Register::None, 1,
					(static_cast<std::uint64_t>(displ) * static_cast<std::uint64_t>(i + 1)) & xsp_info.mask, memory_size, OpAccess::Write,
					xsp_info.code_size, 0);
			}
		}
	}

	static void command_popa(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t op_size) {
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp_info.xsp, OpAccess::ReadWrite);
		}
		MemorySize memory_size;
		Register base_register;
		if (op_size == 4) {
			memory_size = MemorySize::UInt32;
			base_register = Register::EAX;
		} else {
			ICED_DEBUG_ASSERT(op_size == 2);
			memory_size = MemorySize::UInt16;
			base_register = Register::AX;
		}
		for (std::uint32_t i = 0; i < 8; i++) {
			// Ignore eSP
			if (i != 3) {
				if ((flags & Flags::NO_REGISTER_USAGE) == 0)
					add_register(flags, info, reg_add(base_register, 7 - i), OpAccess::Write);
				if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
					add_memory(info, Register::SS, xsp_info.xsp, Register::None, 1,
						(static_cast<std::uint64_t>(op_size) * static_cast<std::uint64_t>(i)) & xsp_info.mask, memory_size, OpAccess::Read,
						xsp_info.code_size, 0);
				}
			}
		}
	}

	static void command_ins(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rdi, rcx;
		switch (instruction.op0_kind()) {
		case OpKind::MemoryESDI:
			addr_size = CodeSize::Code16;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case OpKind::MemoryESEDI:
			addr_size = CodeSize::Code32;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondWrite;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondWrite, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 1);
				info.used_registers_[0] = UsedRegister(Register::DX, OpAccess::CondRead);
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Write, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_outs(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rsi, rcx;
		switch (instruction.op1_kind()) {
		case OpKind::MemorySegSI:
			addr_size = CodeSize::Code16;
			rsi = Register::SI;
			rcx = Register::CX;
			break;
		case OpKind::MemorySegESI:
			addr_size = CodeSize::Code32;
			rsi = Register::ESI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rsi = Register::RSI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondRead;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 1);
				info.used_registers_[0] = UsedRegister(Register::DX, OpAccess::CondRead);
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::Read);
				add_register(flags, info, rsi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_movs(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rsi, rdi, rcx;
		switch (instruction.op0_kind()) {
		case OpKind::MemoryESDI:
			addr_size = CodeSize::Code16;
			rsi = Register::SI;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case OpKind::MemoryESEDI:
			addr_size = CodeSize::Code32;
			rsi = Register::ESI;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rsi = Register::RSI;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondWrite;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondWrite, addr_size, 0);
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Write, addr_size, 0);
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::Read);
				add_register(flags, info, rsi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_cmps(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rsi, rdi, rcx;
		switch (instruction.op0_kind()) {
		case OpKind::MemorySegSI:
			addr_size = CodeSize::Code16;
			rsi = Register::SI;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case OpKind::MemorySegESI:
			addr_size = CodeSize::Code32;
			rsi = Register::ESI;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rsi = Register::RSI;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondRead;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::Read);
				add_register(flags, info, rsi, OpAccess::ReadWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_stos(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rdi, rcx;
		switch (instruction.op0_kind()) {
		case OpKind::MemoryESDI:
			addr_size = CodeSize::Code16;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case OpKind::MemoryESEDI:
			addr_size = CodeSize::Code32;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondWrite;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondWrite, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 1);
				info.used_registers_[0].access_ = OpAccess::CondRead;
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Write, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_lods(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rsi, rcx;
		switch (instruction.op1_kind()) {
		case OpKind::MemorySegSI:
			addr_size = CodeSize::Code16;
			rsi = Register::SI;
			rcx = Register::CX;
			break;
		case OpKind::MemorySegESI:
			addr_size = CodeSize::Code32;
			rsi = Register::ESI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rsi = Register::RSI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondWrite;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 1);
				info.used_registers_[0].access_ = OpAccess::CondWrite;
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondRead);
				add_register(flags, info, rsi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, instruction.memory_segment(), rsi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_memory_segment_register(flags, info, instruction.memory_segment(), OpAccess::Read);
				add_register(flags, info, rsi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_scas(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		CodeSize addr_size;
		Register rdi, rcx;
		switch (instruction.op1_kind()) {
		case OpKind::MemoryESDI:
			addr_size = CodeSize::Code16;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case OpKind::MemoryESEDI:
			addr_size = CodeSize::Code32;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			info.op_accesses_[0] = OpAccess::CondRead;
			info.op_accesses_[1] = OpAccess::CondRead;
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondRead, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 1);
				info.used_registers_[0].access_ = OpAccess::CondRead;
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Read, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
			}
		}
	}

	static void command_xstore(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t size) {
		CodeSize addr_size;
		Register rdi, rcx;
		switch (size) {
		case 2:
			addr_size = CodeSize::Code16;
			rdi = Register::DI;
			rcx = Register::CX;
			break;
		case 4:
			addr_size = CodeSize::Code32;
			rdi = Register::EDI;
			rcx = Register::ECX;
			break;
		default:
			addr_size = CodeSize::Code64;
			rdi = Register::RDI;
			rcx = Register::RCX;
			break;
		}
		if (InstructionInternal::internal_has_repe_or_repne_prefix(instruction)) {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, MemorySize::Unknown, OpAccess::CondWrite, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.empty());
				add_register(flags, info, rcx, OpAccess::ReadCondWrite);
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondRead);
				add_register(flags, info, rdi, OpAccess::CondWrite);
				add_register(flags, info, Register::EAX, OpAccess::CondWrite);
				add_register(flags, info, Register::EDX, OpAccess::CondRead);
			}
		} else {
			if ((flags & Flags::NO_MEMORY_USAGE) == 0)
				add_memory(info, Register::ES, rdi, Register::None, 1, 0, instruction.memory_size(), OpAccess::Write, addr_size, 0);
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if ((flags & Flags::IS_64BIT) == 0)
					add_register(flags, info, Register::ES, OpAccess::Read);
				add_register(flags, info, rdi, OpAccess::ReadWrite);
				add_register(flags, info, Register::EAX, OpAccess::Write);
				add_register(flags, info, Register::EDX, OpAccess::Read);
			}
		}
	}

	static void command_enter(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t op_size) {
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		const Register xsp = xsp_info.xsp;
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp, OpAccess::ReadWrite);
		}

		MemorySize memory_size;
		Register r_sp;
		if (op_size == 8) {
			memory_size = MemorySize::UInt64;
			r_sp = Register::RSP;
		} else if (op_size == 4) {
			memory_size = MemorySize::UInt32;
			r_sp = Register::ESP;
		} else {
			ICED_DEBUG_ASSERT(op_size == 2);
			memory_size = MemorySize::UInt16;
			r_sp = Register::SP;
		}

		if (r_sp != xsp && (flags & Flags::NO_REGISTER_USAGE) == 0)
			add_register(flags, info, r_sp, OpAccess::ReadWrite);

		const std::uint64_t nesting_level = static_cast<std::uint64_t>(instruction.immediate8_2nd() & 0x1F);
		std::uint64_t xsp_offset = 0;
		// push rBP
		if ((flags & Flags::NO_REGISTER_USAGE) == 0)
			add_register(flags, info, reg_add(r_sp, 1), OpAccess::ReadWrite);
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			xsp_offset -= op_size;
			add_memory(info, Register::SS, xsp, Register::None, 1, xsp_offset & xsp_info.mask, memory_size, OpAccess::Write, xsp_info.code_size, 0);
		}

		if (nesting_level != 0) {
			const Register xbp = reg_add(xsp, 1); // rBP immediately follows rSP
			std::uint64_t xbp_offset = 0;
			for (std::uint32_t i = 1; i < static_cast<std::uint32_t>(nesting_level); i++) {
				if (i == 1 && static_cast<std::uint32_t>(r_sp) + 1 != static_cast<std::uint32_t>(xbp) && (flags & Flags::NO_REGISTER_USAGE) == 0)
					add_register(flags, info, xbp, OpAccess::ReadWrite);
				// push [xbp]
				if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
					xbp_offset -= op_size;
					add_memory(info, Register::SS, xbp, Register::None, 1, xbp_offset & xsp_info.mask, memory_size, OpAccess::Read, xsp_info.code_size, 0);
					xsp_offset -= op_size;
					add_memory(info, Register::SS, xsp, Register::None, 1, xsp_offset & xsp_info.mask, memory_size, OpAccess::Write, xsp_info.code_size, 0);
				}
			}
			// push frameTemp
			if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
				xsp_offset -= op_size;
				add_memory(info, Register::SS, xsp, Register::None, 1, xsp_offset & xsp_info.mask, memory_size, OpAccess::Write, xsp_info.code_size, 0);
			}
		}
	}

	static void command_leave(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, std::uint32_t op_size) {
		const XspInfo& xsp_info = get_xsp(instruction.code_size());
		const Register xsp = xsp_info.xsp;
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0)
				add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, xsp, OpAccess::Write);
		}

		MemorySize memory_size;
		Register r_bp;
		if (op_size == 8) {
			memory_size = MemorySize::UInt64;
			r_bp = Register::RBP;
		} else if (op_size == 4) {
			memory_size = MemorySize::UInt32;
			r_bp = Register::EBP;
		} else {
			ICED_DEBUG_ASSERT(op_size == 2);
			memory_size = MemorySize::UInt16;
			r_bp = Register::BP;
		}
		const Register xbp = reg_add(xsp, 1);
		if ((flags & Flags::NO_MEMORY_USAGE) == 0)
			add_memory(info, Register::SS, xbp, Register::None, 1, 0, memory_size, OpAccess::Read, xsp_info.code_size, 0);
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if (xbp == r_bp)
				add_register(flags, info, r_bp, OpAccess::ReadWrite);
			else {
				add_register(flags, info, xbp, OpAccess::Read);
				add_register(flags, info, r_bp, OpAccess::Write);
			}
		}
	}

	static void command_clear_rflags(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		if (instruction.op0_register() == instruction.op1_register() && instruction.op0_kind() == OpKind::Register &&
			instruction.op1_kind() == OpKind::Register) {
			info.op_accesses_[0] = OpAccess::Write;
			info.op_accesses_[1] = OpAccess::None;
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 2 || info.used_registers_.size() == 3);
				info.used_registers_.clear();
				add_register(flags, info, instruction.op0_register(), OpAccess::Write);
			}
		}
	}

	static bool is_clear_instr(const Instruction& instruction) noexcept {
		MvexRegMemConv conv = instruction.mvex_reg_mem_conv();
		return conv == MvexRegMemConv::None || conv == MvexRegMemConv::RegSwizzleNone;
	}

	static void command_clear_reg_regmem(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		if (instruction.op0_register() == instruction.op1_register() && instruction.op1_kind() == OpKind::Register && is_clear_instr(instruction)) {
			info.op_accesses_[0] = OpAccess::Write;
			info.op_accesses_[1] = OpAccess::None;
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 2 || info.used_registers_.size() == 3);
				info.used_registers_.clear();
				info.used_registers_.push_back(UsedRegister(instruction.op0_register(), OpAccess::Write));
			}
		}
	}

	static void command_clear_reg_reg_regmem(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		if (instruction.op1_register() == instruction.op2_register() && instruction.op2_kind() == OpKind::Register && is_clear_instr(instruction)) {
			info.op_accesses_[1] = OpAccess::None;
			info.op_accesses_[2] = OpAccess::None;
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() == 3 || info.used_registers_.size() == 4);
				ICED_DEBUG_ASSERT(info.used_registers_[info.used_registers_.size() - 2].register_() == instruction.op1_register());
				ICED_DEBUG_ASSERT(info.used_registers_[info.used_registers_.size() - 1].register_() == instruction.op2_register());
				info.used_registers_.resize(info.used_registers_.size() - 2);
			}
		}
	}

	static void command_arpl(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			ICED_DEBUG_ASSERT(!info.used_registers_.empty());
			// Skip memory operand, if any
			std::size_t start_index = instruction.op0_kind() == OpKind::Register ? 0 : info.used_registers_.size() - 1;
			for (std::size_t i = start_index; i < info.used_registers_.size(); i++) {
				UsedRegister& reg_info = info.used_registers_[i];
				std::int32_t index = try_get_gpr_16_32_64_index(reg_info.register_());
				if (index >= 4)
					index += 4; // Skip AH, CH, DH, BH
				if (index >= 0)
					reg_info.register_value_ = reg_add(Register::AL, static_cast<std::uint32_t>(index));
			}
		}
	}

	static void command_last_gpr(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, Register base_reg) {
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			constexpr std::uint32_t N = 1;
			const std::uint32_t op_count = instruction.op_count();
			const std::uint32_t imm_count = instruction.op_kind(op_count - 1) == OpKind::Immediate8 ? 1 : 0;
			const std::uint32_t op_index = op_count - N - imm_count;
			if (instruction.op_kind(op_index) == OpKind::Register) {
				ICED_DEBUG_ASSERT(info.used_registers_.size() >= N);
				ICED_DEBUG_ASSERT(instruction.op_register(op_index) == info.used_registers_[info.used_registers_.size() - N].register_());
				ICED_DEBUG_ASSERT(info.used_registers_[info.used_registers_.size() - N].access() == OpAccess::Read);
				std::int32_t index = try_get_gpr_16_32_64_index(instruction.op_register(op_index));
				if (index >= 4 && base_reg == Register::AL)
					index += 4; // Skip AH, CH, DH, BH
				if (index >= 0) {
					const std::size_t regs_index = info.used_registers_.size() - N;
					info.used_registers_[regs_index] = UsedRegister(reg_add(base_reg, static_cast<std::uint32_t>(index)), OpAccess::Read);
				}
			}
		}
	}

	static void command_lea(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			ICED_DEBUG_ASSERT(!info.used_registers_.empty());
			ICED_DEBUG_ASSERT(instruction.op0_kind() == OpKind::Register);
			const Register reg = instruction.op0_register();
			for (std::size_t i = 1; i < info.used_registers_.size(); i++) {
				UsedRegister& reg_info = info.used_registers_[i];
				if (reg >= Register::EAX && reg <= Register::R15D) {
					if (reg_info.register_() >= Register::RAX && reg_info.register_() <= Register::R15) {
						reg_info.register_value_ = static_cast<Register>(static_cast<std::uint32_t>(reg_info.register_()) -
																		 static_cast<std::uint32_t>(Register::RAX) + static_cast<std::uint32_t>(Register::EAX));
					}
				} else if (reg >= Register::AX && reg <= Register::R15W) {
					if (reg_info.register_() >= Register::EAX && reg_info.register_() <= Register::R15) {
						reg_info.register_value_ = static_cast<Register>(
							((static_cast<std::uint32_t>(reg_info.register_()) - static_cast<std::uint32_t>(Register::EAX)) & 0xF) +
							static_cast<std::uint32_t>(Register::AX));
					}
				} else {
					ICED_DEBUG_ASSERT(reg >= Register::RAX && reg <= Register::R15);
					break;
				}
			}
		}
	}

	static void command_emmi(const Instruction& instruction, InstructionInfo& info, std::uint32_t flags, OpAccess op_access) {
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if (instruction.op0_kind() == OpKind::Register) {
				Register reg = instruction.op0_register();
				if (reg >= Register::MM0 && reg <= Register::MM7) {
					reg = reg_add(Register::MM0, (static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::MM0)) ^ 1);
					add_register(flags, info, reg, op_access);
				}
			}
		}
	}

	static void command_mem_displ(InstructionInfo& info, std::uint32_t flags, std::int32_t displ) {
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			if (info.used_memory_locations_.size() == 1) {
				// Index = CodeSize
				static constexpr std::uint64_t MASK[4] = {UINT64_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX};
				UsedMemory& loc = info.used_memory_locations_[0];
				loc.displacement_ = (loc.displacement_ + static_cast<std::uint64_t>(static_cast<std::int64_t>(displ))) &
									MASK[static_cast<std::size_t>(loc.address_size_)];
			} else
				ICED_DEBUG_ASSERT(false);
		}
	}

	static constexpr std::int32_t try_get_gpr_16_32_64_index(Register reg) noexcept {
		const std::uint32_t r = static_cast<std::uint32_t>(reg);
		std::uint32_t index = r - static_cast<std::uint32_t>(Register::EAX);
		if (index <= 15)
			return static_cast<std::int32_t>(index);
		index = r - static_cast<std::uint32_t>(Register::RAX);
		if (index <= 15)
			return static_cast<std::int32_t>(index);
		index = r - static_cast<std::uint32_t>(Register::AX);
		if (index <= 15)
			return static_cast<std::int32_t>(index);
		return -1;
	}

	static void add_memory(InstructionInfo& info, Register segment_register, Register base_register, Register index_register,
		std::uint32_t scale, std::uint64_t displ, MemorySize memory_size, OpAccess access, CodeSize address_size, std::uint32_t vsib_size) {
		if (address_size == CodeSize::Unknown) {
			const Register reg = base_register != Register::None ? base_register : index_register;
			if (register_ext::is_gpr64(reg))
				address_size = CodeSize::Code64;
			else if (register_ext::is_gpr32(reg))
				address_size = CodeSize::Code32;
			else if (register_ext::is_gpr16(reg))
				address_size = CodeSize::Code16;
		}
		if (access != OpAccess::NoMemAccess) {
			info.used_memory_locations_.push_back(
				UsedMemory(segment_register, base_register, index_register, scale, displ, memory_size, access, address_size, vsib_size));
		}
	}

	static void add_memory_segment_register(std::uint32_t flags, InstructionInfo& info, Register seg, OpAccess access) {
		ICED_DEBUG_ASSERT(Register::ES <= seg && seg <= Register::GS);
		// Ignore es,cs,ss,ds memory operand segment registers in 64-bit mode
		if ((flags & Flags::IS_64BIT) == 0 || seg >= Register::FS)
			add_register(flags, info, seg, access);
	}

	static void add_register(std::uint32_t flags, InstructionInfo& info, Register reg, OpAccess access) {
		ICED_DEBUG_ASSERT((flags & Flags::NO_REGISTER_USAGE) == 0);

		Register write_reg = reg;
		if ((flags & (Flags::IS_64BIT | Flags::ZERO_EXT_VEC_REGS)) != 0) {
			static_assert(static_cast<std::uint32_t>(OpAccess::Write) + 1 == static_cast<std::uint32_t>(OpAccess::CondWrite), "");
			static_assert(static_cast<std::uint32_t>(OpAccess::Write) + 2 == static_cast<std::uint32_t>(OpAccess::ReadWrite), "");
			static_assert(static_cast<std::uint32_t>(OpAccess::Write) + 3 == static_cast<std::uint32_t>(OpAccess::ReadCondWrite), "");
			if (static_cast<std::uint32_t>(access) - static_cast<std::uint32_t>(OpAccess::Write) <= 3) {
				static_assert(IcedConstants::VMM_FIRST == Register::ZMM0, "");
				std::uint32_t index = static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::EAX);
				if ((flags & Flags::IS_64BIT) != 0 && index <= static_cast<std::uint32_t>(Register::R15D) - static_cast<std::uint32_t>(Register::EAX))
					write_reg = reg_add(Register::RAX, index);
				else {
					index = static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::XMM0);
					if ((flags & Flags::ZERO_EXT_VEC_REGS) != 0 &&
						index <= static_cast<std::uint32_t>(IcedConstants::VMM_LAST) - static_cast<std::uint32_t>(Register::XMM0))
						write_reg = reg_add(Register::ZMM0, index % IcedConstants::VMM_COUNT);
				}
				if (access != OpAccess::ReadWrite && access != OpAccess::ReadCondWrite)
					reg = write_reg;
			}
		}

		if (write_reg == reg)
			info.used_registers_.push_back(UsedRegister(reg, access));
		else {
			ICED_DEBUG_ASSERT(access == OpAccess::ReadWrite || access == OpAccess::ReadCondWrite);
			info.used_registers_.push_back(UsedRegister(reg, OpAccess::Read));
			info.used_registers_.push_back(UsedRegister(write_reg, access == OpAccess::ReadWrite ? OpAccess::Write : OpAccess::CondWrite));
		}
	}
};

const InstructionInfo& InstructionInfoFactoryImpl::create(InstructionInfo& info, const Instruction& instruction, std::uint32_t options) {
	info.used_registers_.clear();
	info.used_memory_locations_.clear();

	const InfoTableEntry& entry = INFO_TABLE[static_cast<std::size_t>(instruction.code())];
	const std::uint32_t flags1 = entry.flags1;
	const std::uint32_t flags2 = entry.flags2;

	const CodeSize code_size = instruction.code_size();
	std::uint32_t flags = options & (Flags::NO_MEMORY_USAGE | Flags::NO_REGISTER_USAGE);
	if (code_size == CodeSize::Code64 || code_size == CodeSize::Unknown)
		flags |= Flags::IS_64BIT;
	const std::uint32_t encoding = (flags2 >> InfoFlags2::ENCODING_SHIFT) & InfoFlags2::ENCODING_MASK;
	if (encoding != static_cast<std::uint32_t>(EncodingKind::Legacy))
		flags |= Flags::ZERO_EXT_VEC_REGS;

	const OpInfo0 op0_info = static_cast<OpInfo0>((flags1 >> InfoFlags1::OP_INFO0_SHIFT) & InfoFlags1::OP_INFO0_MASK);
	OpAccess op0_access;
	switch (op0_info) {
	case OpInfo0::None:
		op0_access = OpAccess::None;
		break;
	case OpInfo0::Read:
		op0_access = OpAccess::Read;
		break;

	case OpInfo0::Write:
		if (instruction.has_op_mask() && instruction.merging_masking()) {
			if (instruction.op0_kind() != OpKind::Register)
				op0_access = OpAccess::CondWrite;
			else
				op0_access = OpAccess::ReadWrite;
		} else
			op0_access = OpAccess::Write;
		break;

	case OpInfo0::WriteVmm:
		// If it's opmask+merging ({k1}) and dest is xmm/ymm/zmm, then access is one of:
		//	k1			mem			xmm/ymm		zmm
		//	----------------------------------------
		//	all 1s		write		write		write		all bits are overwritten, upper bits in zmm (if xmm/ymm) are cleared
		//	all 0s		no access	read/write	no access	no elem is written, but xmm/ymm's upper bits (in zmm) are cleared so
		//													treat it as R lower bits + clear upper bits + W full reg
		//	else		cond-write	read/write	r-c-w		some elems are unchanged, the others are overwritten
		// If it's xmm/ymm, use RW, else use RCW. If it's mem, use CW
		if (instruction.has_op_mask() && instruction.merging_masking()) {
			if (instruction.op0_kind() != OpKind::Register)
				op0_access = OpAccess::CondWrite;
			else
				op0_access = OpAccess::ReadCondWrite;
		} else
			op0_access = OpAccess::Write;
		break;

	case OpInfo0::WriteForce:
	case OpInfo0::WriteForceP1:
		op0_access = OpAccess::Write;
		break;

	case OpInfo0::CondWrite:
		op0_access = OpAccess::CondWrite;
		break;

	case OpInfo0::CondWrite32_ReadWrite64:
		if ((flags & Flags::IS_64BIT) != 0)
			op0_access = OpAccess::ReadWrite;
		else
			op0_access = OpAccess::CondWrite;
		break;

	case OpInfo0::ReadWrite:
		op0_access = OpAccess::ReadWrite;
		break;

	case OpInfo0::ReadWriteVmm:
		// If it's opmask+merging ({k1}) and dest is xmm/ymm/zmm, then access is one of:
		//	k1			xmm/ymm		zmm
		//	-------------------------------
		//	all 1s		read/write	read/write	all bits are overwritten, upper bits in zmm (if xmm/ymm) are cleared
		//	all 0s		read/write	no access	no elem is written, but xmm/ymm's upper bits (in zmm) are cleared so
		//										treat it as R lower bits + clear upper bits + W full reg
		//	else		read/write	r-c-w		some elems are unchanged, the others are overwritten
		// If it's xmm/ymm, use RW, else use RCW
		if (instruction.has_op_mask() && instruction.merging_masking())
			op0_access = OpAccess::ReadCondWrite;
		else
			op0_access = OpAccess::ReadWrite;
		break;

	case OpInfo0::ReadCondWrite:
		op0_access = OpAccess::ReadCondWrite;
		break;

	case OpInfo0::NoMemAccess:
		op0_access = OpAccess::NoMemAccess;
		break;

	case OpInfo0::WriteMem_ReadWriteReg:
		if (InstructionInternal::internal_op0_is_not_reg_or_op1_is_not_reg(instruction))
			op0_access = OpAccess::Write;
		else
			op0_access = OpAccess::ReadWrite;
		break;

	default:
		ICED_UNREACHABLE();
	}

	ICED_DEBUG_ASSERT(instruction.op_count() <= IcedConstants::MAX_OP_COUNT);
	info.op_accesses_[0] = op0_access;
	const OpInfo1 op1_info = static_cast<OpInfo1>((flags1 >> InfoFlags1::OP_INFO1_SHIFT) & InfoFlags1::OP_INFO1_MASK);
	info.op_accesses_[1] = OP_ACCESS_1[static_cast<std::size_t>(op1_info)];
	const OpInfo2 op2_info = static_cast<OpInfo2>((flags1 >> InfoFlags1::OP_INFO2_SHIFT) & InfoFlags1::OP_INFO2_MASK);
	info.op_accesses_[2] = OP_ACCESS_2[static_cast<std::size_t>(op2_info)];
	static_assert(InstrInfoConstants::OP_INFO3_COUNT == 2, "");
	info.op_accesses_[3] = (flags1 & (InfoFlags1::OP_INFO3_MASK << InfoFlags1::OP_INFO3_SHIFT)) != 0 ? OpAccess::Read : OpAccess::None;
	static_assert(InstrInfoConstants::OP_INFO4_COUNT == 2, "");
	info.op_accesses_[4] = (flags1 & (InfoFlags1::OP_INFO4_MASK << InfoFlags1::OP_INFO4_SHIFT)) != 0 ? OpAccess::Read : OpAccess::None;
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");

	const std::uint32_t op_count = instruction.op_count();
	for (std::uint32_t i = 0; i < op_count; i++) {
		OpAccess access = info.op_accesses_[i];
		if (access == OpAccess::None)
			continue;

		switch (instruction.op_kind(i)) {
		case OpKind::Register:
			if (access == OpAccess::NoMemAccess) {
				access = OpAccess::Read;
				info.op_accesses_[i] = OpAccess::Read;
			}
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				if (i == 0 && op0_info == OpInfo0::WriteForceP1) {
					const Register reg = instruction.op0_register();
					add_register(flags, info, reg, access);
					if (Register::K0 <= reg && reg <= Register::K7)
						add_register(flags, info, reg_add(Register::K0, (static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(Register::K0)) ^ 1), access);
				} else if (i == 1 && op1_info == OpInfo1::ReadP3) {
					const Register reg = instruction.op1_register();
					if (Register::XMM0 <= reg && reg <= IcedConstants::VMM_LAST) {
						// Creates 4 consecutive vec regs with first one a multiple of 4,
						// eg. XMM5 -> XMM4-XMM7. All vec regs enum values are consecutive from XMM0 - VMM_LAST (ZMM31).
						const std::uint32_t reg_base = static_cast<std::uint32_t>(IcedConstants::VMM_FIRST) +
													   ((static_cast<std::uint32_t>(reg) - static_cast<std::uint32_t>(IcedConstants::VMM_FIRST)) & ~3U);
						for (std::uint32_t j = 0; j < 4; j++)
							add_register(flags, info, static_cast<Register>(reg_base + j), access);
					}
				} else
					add_register(flags, info, instruction.op_register(i), access);
			}
			break;

		case OpKind::Memory: {
			static_assert(InfoFlags1::IGNORES_SEGMENT == 1U << 31, "");
			static_assert(static_cast<std::uint32_t>(Register::None) == 0, "");
			// If IGNORES_SEGMENT is set, the segment register is Register::None
			const Register segment_register =
				static_cast<Register>(static_cast<std::uint32_t>(instruction.memory_segment()) & ((flags1 & InfoFlags1::IGNORES_SEGMENT) != 0 ? 0U : ~0U));
			const Register base_register = instruction.memory_base();
			if (base_register == Register::RIP) {
				if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
					add_memory(info, segment_register, Register::None, Register::None, 1, instruction.memory_displacement64(), instruction.memory_size(),
						access, CodeSize::Code64, 0);
				}
				if ((flags & Flags::NO_REGISTER_USAGE) == 0 && segment_register != Register::None)
					add_memory_segment_register(flags, info, segment_register, OpAccess::Read);
			} else if (base_register == Register::EIP) {
				if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
					add_memory(info, segment_register, Register::None, Register::None, 1, instruction.memory_displacement32(), instruction.memory_size(),
						access, CodeSize::Code32, 0);
				}
				if ((flags & Flags::NO_REGISTER_USAGE) == 0 && segment_register != Register::None)
					add_memory_segment_register(flags, info, segment_register, OpAccess::Read);
			} else {
				Register index_register;
				std::uint32_t scale;
				if ((flags1 & InfoFlags1::IGNORES_INDEX_VA) != 0) {
					const Register index = instruction.memory_index();
					if ((flags & Flags::NO_REGISTER_USAGE) == 0 && index != Register::None)
						add_register(flags, info, index, OpAccess::Read);
					index_register = Register::None;
					scale = 1;
				} else {
					index_register = instruction.memory_index();
					scale = instruction.memory_index_scale();
				}
				if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
					const std::uint32_t addr_size_bytes =
						InstructionInternal::get_address_size_in_bytes(base_register, index_register, instruction.memory_displ_size(), code_size);
					CodeSize addr_size;
					switch (addr_size_bytes) {
					case 8:
						addr_size = CodeSize::Code64;
						break;
					case 4:
						addr_size = CodeSize::Code32;
						break;
					case 2:
						addr_size = CodeSize::Code16;
						break;
					default:
						addr_size = CodeSize::Unknown;
						break;
					}
					std::uint32_t vsib_size = 0;
					if (register_ext::is_vector_register(index_register)) {
						const std::optional<bool> is_vsib64 = instruction.vsib();
						if (is_vsib64.has_value())
							vsib_size = *is_vsib64 ? 8 : 4;
					}
					const std::uint64_t displ = addr_size_bytes == 8 ? instruction.memory_displacement64() : instruction.memory_displacement32();
					add_memory(info, segment_register, base_register, index_register, scale, displ, instruction.memory_size(), access, addr_size,
						vsib_size);
				}
				if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
					if (segment_register != Register::None)
						add_memory_segment_register(flags, info, segment_register, OpAccess::Read);
					if (base_register != Register::None)
						add_register(flags, info, base_register, OpAccess::Read);
					if (index_register != Register::None)
						add_register(flags, info, index_register, OpAccess::Read);
				}
			}
			break;
		}

		default:
			break;
		}
	}

	const ImpliedAccess implied_access = static_cast<ImpliedAccess>((flags1 >> InfoFlags1::IMPLIED_ACCESS_SHIFT) & InfoFlags1::IMPLIED_ACCESS_MASK);
	if (implied_access != ImpliedAccess::None)
		add_implied_accesses(implied_access, instruction, info, flags);

	if (instruction.has_op_mask() && (flags & Flags::NO_REGISTER_USAGE) == 0)
		add_register(flags, info, instruction.op_mask(), (flags1 & InfoFlags1::OP_MASK_READ_WRITE) != 0 ? OpAccess::ReadWrite : OpAccess::Read);
	return info;
}

void InstructionInfoFactoryImpl::add_implied_accesses(ImpliedAccess implied_access, const Instruction& instruction, InstructionInfo& info, std::uint32_t flags) {
	ICED_DEBUG_ASSERT(implied_access != ImpliedAccess::None);
	switch (implied_access) {
	// GENERATOR-BEGIN: ImpliedAccessHandler
	// ⚠️This was generated by GENERATOR!🦹‍♂️
	case ImpliedAccess::None:
		break;
	case ImpliedAccess::Shift_Ib_MASK1FMOD9:
		break;
	case ImpliedAccess::Shift_Ib_MASK1FMOD11:
		break;
	case ImpliedAccess::Shift_Ib_MASK1F:
		break;
	case ImpliedAccess::Shift_Ib_MASK3F:
		break;
	case ImpliedAccess::Clear_rflags:
		command_clear_rflags(instruction, info, flags);
		break;
	case ImpliedAccess::t_push1x2:
		command_push(instruction, info, flags, 1, 2);
		break;
	case ImpliedAccess::t_push1x4:
		command_push(instruction, info, flags, 1, 4);
		break;
	case ImpliedAccess::t_pop1x2:
		command_pop(instruction, info, flags, 1, 2);
		break;
	case ImpliedAccess::t_pop1x4:
		command_pop(instruction, info, flags, 1, 4);
		break;
	case ImpliedAccess::t_RWal:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AL, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_push1x8:
		command_push(instruction, info, flags, 1, 8);
		break;
	case ImpliedAccess::t_pop1x8:
		command_pop(instruction, info, flags, 1, 8);
		break;
	case ImpliedAccess::t_pusha2:
		command_pusha(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_pusha4:
		command_pusha(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_popa2:
		command_popa(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_popa4:
		command_popa(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_arpl:
		command_arpl(instruction, info, flags);
		break;
	case ImpliedAccess::t_ins:
		command_ins(instruction, info, flags);
		break;
	case ImpliedAccess::t_outs:
		command_outs(instruction, info, flags);
		break;
	case ImpliedAccess::t_lea:
		command_lea(instruction, info, flags);
		break;
	case ImpliedAccess::t_gpr16:
		command_last_gpr(instruction, info, flags, Register::AX);
		break;
	case ImpliedAccess::t_poprm2:
		command_pop_rm(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_poprm4:
		command_pop_rm(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_poprm8:
		command_pop_rm(instruction, info, flags, 8);
		break;
	case ImpliedAccess::t_Ral_Wah:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AL, OpAccess::Read);
			add_register(flags, info, Register::AH, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rax_Weax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_RWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rax_Wdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::DX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Wedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rrax_Wrdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::RDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_push2x2:
		command_push(instruction, info, flags, 2, 2);
		break;
	case ImpliedAccess::t_push2x4:
		command_push(instruction, info, flags, 2, 4);
		break;
	case ImpliedAccess::t_Rah:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AH, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wah:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AH, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_movs:
		command_movs(instruction, info, flags);
		break;
	case ImpliedAccess::t_cmps:
		command_cmps(instruction, info, flags);
		break;
	case ImpliedAccess::t_stos:
		command_stos(instruction, info, flags);
		break;
	case ImpliedAccess::t_lods:
		command_lods(instruction, info, flags);
		break;
	case ImpliedAccess::t_scas:
		command_scas(instruction, info, flags);
		break;
	case ImpliedAccess::t_Wes:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ES, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wds:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::DS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_CWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::CondWrite);
		}
		break;
	case ImpliedAccess::t_enter2:
		command_enter(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_enter4:
		command_enter(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_enter8:
		command_enter(instruction, info, flags, 8);
		break;
	case ImpliedAccess::t_leave2:
		command_leave(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_leave4:
		command_leave(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_leave8:
		command_leave(instruction, info, flags, 8);
		break;
	case ImpliedAccess::t_pop2x2:
		command_pop(instruction, info, flags, 2, 2);
		break;
	case ImpliedAccess::t_pop2x4:
		command_pop(instruction, info, flags, 2, 4);
		break;
	case ImpliedAccess::t_pop2x8:
		command_pop(instruction, info, flags, 2, 8);
		break;
	case ImpliedAccess::b64_t_Wss_pop5x2_f_pop3x2:
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::SS, OpAccess::Write);
			}
			command_pop(instruction, info, flags, 5, 2);
		} else {
			command_pop(instruction, info, flags, 3, 2);
		}
		break;
	case ImpliedAccess::b64_t_Wss_pop5x4_f_pop3x4:
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::SS, OpAccess::Write);
			}
			command_pop(instruction, info, flags, 5, 4);
		} else {
			command_pop(instruction, info, flags, 3, 4);
		}
		break;
	case ImpliedAccess::t_Wss_pop5x8:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		command_pop(instruction, info, flags, 5, 8);
		break;
	case ImpliedAccess::t_Ral_Wax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AL, OpAccess::Read);
			add_register(flags, info, Register::AX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wal:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AL, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_RWst0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST0, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rst0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST0, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rst0_RWst1:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST0, OpAccess::Read);
			add_register(flags, info, Register::ST1, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RCWst0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST0, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_Rst1_RWst0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST1, OpAccess::Read);
			add_register(flags, info, Register::ST0, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rst0_Rst1:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ST0, OpAccess::Read);
			add_register(flags, info, Register::ST1, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wst0TOst7_Wmm0TOmm7:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::ST0); reg_num <= static_cast<std::uint32_t>(Register::ST7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::MM0); reg_num <= static_cast<std::uint32_t>(Register::MM7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rst0TOst7_Rmm0TOmm7:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::ST0); reg_num <= static_cast<std::uint32_t>(Register::ST7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Read);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::MM0); reg_num <= static_cast<std::uint32_t>(Register::MM7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_RWcx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWecx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWrcx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rcx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rrcx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wdx_RWax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::DX, OpAccess::Write);
			add_register(flags, info, Register::AX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Wedx_RWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EDX, OpAccess::Write);
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Wrdx_RWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RDX, OpAccess::Write);
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWax_RWdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::ReadWrite);
			add_register(flags, info, Register::DX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWeax_RWedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
			add_register(flags, info, Register::EDX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWrax_RWrdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
			add_register(flags, info, Register::RDX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_push2x8:
		command_push(instruction, info, flags, 2, 8);
		break;
	case ImpliedAccess::t_Rcr0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CR0, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_RWcr0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CR0, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_gpr16_RWcr0:
		command_last_gpr(instruction, info, flags, Register::AX);
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CR0, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RCWeax_b64_t_CRrcx_CRrdx_CRrbx_CWrcx_CWrdx_CWrbx_f_CRecx_CRedx_CRebx_CRds_CWecx_CWedx_CWebx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::ReadCondWrite);
		}
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::RCX, OpAccess::CondRead);
				add_register(flags, info, Register::RDX, OpAccess::CondRead);
				add_register(flags, info, Register::RBX, OpAccess::CondRead);
				add_register(flags, info, Register::RCX, OpAccess::CondWrite);
				add_register(flags, info, Register::RDX, OpAccess::CondWrite);
				add_register(flags, info, Register::RBX, OpAccess::CondWrite);
			}
		} else {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::ECX, OpAccess::CondRead);
				add_register(flags, info, Register::EDX, OpAccess::CondRead);
				add_register(flags, info, Register::EBX, OpAccess::CondRead);
				add_register(flags, info, Register::DS, OpAccess::CondRead);
				add_register(flags, info, Register::ECX, OpAccess::CondWrite);
				add_register(flags, info, Register::EDX, OpAccess::CondWrite);
				add_register(flags, info, Register::EBX, OpAccess::CondWrite);
			}
		}
		break;
	case ImpliedAccess::t_CWecx_CWedx_CWebx_RWeax_b64_t_CRrcx_CRrdx_CRrbx_f_CRecx_CRedx_CRebx_CRds:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::CondWrite);
			add_register(flags, info, Register::EDX, OpAccess::CondWrite);
			add_register(flags, info, Register::EBX, OpAccess::CondWrite);
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
		}
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::RCX, OpAccess::CondRead);
				add_register(flags, info, Register::RDX, OpAccess::CondRead);
				add_register(flags, info, Register::RBX, OpAccess::CondRead);
			}
		} else {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::ECX, OpAccess::CondRead);
				add_register(flags, info, Register::EDX, OpAccess::CondRead);
				add_register(flags, info, Register::EBX, OpAccess::CondRead);
				add_register(flags, info, Register::DS, OpAccess::CondRead);
			}
		}
		break;
	case ImpliedAccess::t_Rax_Recx_Redx_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax_Recx_Redx_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx_Redx_Rrax_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax_Recx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx_Weax_Wedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Write);
			add_register(flags, info, Register::EDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Recx_Redx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rax_Wfs_Wgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Write);
			add_register(flags, info, Register::GS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Wfs_Wgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Write);
			add_register(flags, info, Register::GS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rrax_Wfs_Wgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Write);
			add_register(flags, info, Register::GS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rax_Rfs_Rgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Read);
			add_register(flags, info, Register::GS, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax_Rfs_Rgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Read);
			add_register(flags, info, Register::GS, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rrax_Rfs_Rgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::FS, OpAccess::Read);
			add_register(flags, info, Register::GS, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax_Wcr0_Wdr6_Wdr7_WesTOgs_Wcr2TOcr4_Wdr0TOdr3_b64_t_WraxTOr15_f_WeaxTOedi:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::CR0, OpAccess::Write);
			add_register(flags, info, Register::DR6, OpAccess::Write);
			add_register(flags, info, Register::DR7, OpAccess::Write);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::ES); reg_num <= static_cast<std::uint32_t>(Register::GS); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::CR2); reg_num <= static_cast<std::uint32_t>(Register::CR4); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::DR0); reg_num <= static_cast<std::uint32_t>(Register::DR3); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
		}
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::RAX); reg_num <= static_cast<std::uint32_t>(Register::R15); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			}
		} else {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::EAX); reg_num <= static_cast<std::uint32_t>(Register::EDI); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			}
		}
		break;
	case ImpliedAccess::t_Rax_Recx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx_Rrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Weax_Wecx_Wedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Write);
			add_register(flags, info, Register::ECX, OpAccess::Write);
			add_register(flags, info, Register::EDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Recx_CRebx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
		}
		break;
	case ImpliedAccess::t_Rax_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Reax_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rrax_Rseg:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wecx_b64_t_Wr11:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Write);
		}
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::R11, OpAccess::Write);
			}
		}
		break;
	case ImpliedAccess::t_Redi_Res:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EDI, OpAccess::Read);
			add_register(flags, info, Register::ES, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx_Wcs_Wss_b64_t_Rr11d:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::CS, OpAccess::Write);
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				add_register(flags, info, Register::R11D, OpAccess::Read);
			}
		}
		break;
	case ImpliedAccess::t_Rr11d_Rrcx_Wcs_Wss:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::R11D, OpAccess::Read);
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::CS, OpAccess::Write);
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Weax_Wedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Write);
			add_register(flags, info, Register::EDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wesp:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ESP, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Recx_Redx_Wesp_Wcs_Wss:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::ESP, OpAccess::Write);
			add_register(flags, info, Register::CS, OpAccess::Write);
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rrcx_Rrdx_Wrsp_Wcs_Wss:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::RDX, OpAccess::Read);
			add_register(flags, info, Register::RSP, OpAccess::Write);
			add_register(flags, info, Register::CS, OpAccess::Write);
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_zrrm:
		command_clear_reg_regmem(instruction, info, flags);
		break;
	case ImpliedAccess::t_zrrrm:
		command_clear_reg_reg_regmem(instruction, info, flags);
		break;
	case ImpliedAccess::b64_t_RWxmm0TOxmm15_f_RWxmm0TOxmm7:
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::XMM0); reg_num <= static_cast<std::uint32_t>(Register::XMM15); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::ReadWrite);
			}
		} else {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::XMM0); reg_num <= static_cast<std::uint32_t>(Register::XMM7); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::ReadWrite);
			}
		}
		break;
	case ImpliedAccess::b64_t_Wzmm0TOzmm15_f_Wzmm0TOzmm7:
		if ((flags & Flags::IS_64BIT) != 0) {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::ZMM0); reg_num <= static_cast<std::uint32_t>(Register::ZMM15); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			}
		} else {
			if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
				for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::ZMM0); reg_num <= static_cast<std::uint32_t>(Register::ZMM7); reg_num++)
					add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
			}
		}
		break;
	case ImpliedAccess::t_CRecx_Wecx_Wedx_Webx_RWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::CondRead);
			add_register(flags, info, Register::ECX, OpAccess::Write);
			add_register(flags, info, Register::EDX, OpAccess::Write);
			add_register(flags, info, Register::EBX, OpAccess::Write);
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRsi_CReax_CRes_CWeax_CWedx_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::EAX, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::EAX, OpAccess::CondWrite);
			add_register(flags, info, Register::EDX, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CReax_CResi_CRes_CWeax_CWedx_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::CondRead);
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::EAX, OpAccess::CondWrite);
			add_register(flags, info, Register::EDX, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CReax_CRrsi_CRes_CWeax_CWedx_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::CondRead);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::EAX, OpAccess::CondWrite);
			add_register(flags, info, Register::EDX, OpAccess::CondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRsi_CRdi_CRes_CWsi_RCWax_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::AX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CResi_CRedi_CRes_CWesi_RCWeax_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::EAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRrsi_CRrdi_CRes_CWrsi_RCWrax_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_Rcl_Rax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CL, OpAccess::Read);
			add_register(flags, info, Register::AX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rcl_Reax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CL, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_xstore2:
		command_xstore(instruction, info, flags, 2);
		break;
	case ImpliedAccess::t_xstore4:
		command_xstore(instruction, info, flags, 4);
		break;
	case ImpliedAccess::t_xstore8:
		command_xstore(instruction, info, flags, 8);
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CRdx_CRbx_CRsi_CRdi_CRes_CWsi_CWdi_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::DX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::BX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::DX, OpAccess::CondRead);
			add_register(flags, info, Register::BX, OpAccess::CondRead);
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::DI, OpAccess::CondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CRedx_CRebx_CResi_CRedi_CRes_CWesi_CWedi_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::EDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EDX, OpAccess::CondRead);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::EDI, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CRrdx_CRrbx_CRrsi_CRrdi_CRes_CWrsi_CWrdi_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RDX, OpAccess::CondRead);
			add_register(flags, info, Register::RBX, OpAccess::CondRead);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RDI, OpAccess::CondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CRmem_CWmem_CWmem_CRax_CRdx_CRbx_CRsi_CRdi_CRes_CWsi_CWdi_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::AX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::BX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::AX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::CondRead);
			add_register(flags, info, Register::DX, OpAccess::CondRead);
			add_register(flags, info, Register::BX, OpAccess::CondRead);
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::DI, OpAccess::CondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CRmem_CWmem_CWmem_CReax_CRedx_CRebx_CResi_CRedi_CRes_CWesi_CWedi_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::EAX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EAX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::CondRead);
			add_register(flags, info, Register::EDX, OpAccess::CondRead);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::EDI, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CRmem_CWmem_CWmem_CRrax_CRrdx_CRrbx_CRrsi_CRrdi_CRes_CWrsi_CWrdi_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RAX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RAX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::CondRead);
			add_register(flags, info, Register::RDX, OpAccess::CondRead);
			add_register(flags, info, Register::RBX, OpAccess::CondRead);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RDI, OpAccess::CondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_RCWal:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AL, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_RCWax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_RCWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_Reax_Redx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_gpr8:
		command_last_gpr(instruction, info, flags, Register::AL);
		break;
	case ImpliedAccess::t_gpr32_Reax_Redx:
		command_last_gpr(instruction, info, flags, Register::EAX);
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rmem_Rseg:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, get_seg_default_ds(instruction), instruction.op0_register(), Register::None, 1, 0x0, MemorySize::UInt8, OpAccess::Read, CodeSize::Unknown, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_RCWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_Wss:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wfs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::FS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wgs:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::GS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_CRecx_CRebx_RCWeax_RCWedx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::CondRead);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::EAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::EDX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRrcx_CRrbx_RCWrax_RCWrdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::CondRead);
			add_register(flags, info, Register::RBX, OpAccess::CondRead);
			add_register(flags, info, Register::RAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::RDX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_Wmem_RarDI_Rseg:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, get_seg_default_ds(instruction), get_a_rdi(instruction), Register::None, 1, 0x0, instruction.memory_size(), OpAccess::Write, CodeSize::Unknown, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, get_a_rdi(instruction), OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rxmm0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::XMM0, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Redx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rrdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wmem_Res:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, instruction.op0_register(), Register::None, 1, 0x0, instruction.memory_size(), OpAccess::Write, CodeSize::Unknown, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::Read);
			}
		}
		break;
	case ImpliedAccess::t_Reax_Redx_Wxmm0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::XMM0, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rrax_Rrdx_Wxmm0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::RDX, OpAccess::Read);
			add_register(flags, info, Register::XMM0, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Redx_Wecx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rrax_Rrdx_Wecx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::RDX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wxmm0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::XMM0, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wecx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rmem_Rds:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::DS, instruction.op0_register(), Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::Read, CodeSize::Unknown, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::DS, OpAccess::Read);
			}
		}
		break;
	case ImpliedAccess::t_Rrcx_Rrdx_RWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::RDX, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rmem_Rrcx_Rseg_RWrax:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, get_seg_default_ds(instruction), Register::RCX, Register::None, 1, 0x0, MemorySize::UInt128, OpAccess::Read, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_memory_segment_register(flags, info, get_seg_default_ds(instruction), OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rax_Recx_Redx_Weax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Recx_Redx_RWeax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Recx_Redx_RWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Rax_Recx_Redx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Recx_Redx_Rrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wtmm0TOtmm7:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::TMM0); reg_num <= static_cast<std::uint32_t>(Register::TMM7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Rebx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::EBX, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Rebx_Weax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EBX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_emmiW:
		command_emmi(instruction, info, flags, OpAccess::Write);
		break;
	case ImpliedAccess::t_emmiRW:
		command_emmi(instruction, info, flags, OpAccess::ReadWrite);
		break;
	case ImpliedAccess::t_emmiR:
		command_emmi(instruction, info, flags, OpAccess::Read);
		break;
	case ImpliedAccess::t_CRrcx_CRrdx_CRr8_CRr9_RWrax:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::CondRead);
			add_register(flags, info, Register::RDX, OpAccess::CondRead);
			add_register(flags, info, Register::R8, OpAccess::CondRead);
			add_register(flags, info, Register::R9, OpAccess::CondRead);
			add_register(flags, info, Register::RAX, OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_RWxmm0TOxmm7:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::XMM0); reg_num <= static_cast<std::uint32_t>(Register::XMM7); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::ReadWrite);
		}
		break;
	case ImpliedAccess::t_Reax_Rxmm0:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::XMM0, OpAccess::Read);
		}
		break;
	case ImpliedAccess::t_Wxmm1_Wxmm2_RWxmm0_Wxmm4TOxmm6:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::XMM1, OpAccess::Write);
			add_register(flags, info, Register::XMM2, OpAccess::Write);
			add_register(flags, info, Register::XMM0, OpAccess::ReadWrite);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::XMM4); reg_num <= static_cast<std::uint32_t>(Register::XMM6); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_RWxmm0_RWxmm1_Wxmm2TOxmm6:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::XMM0, OpAccess::ReadWrite);
			add_register(flags, info, Register::XMM1, OpAccess::ReadWrite);
			for (std::uint32_t reg_num = static_cast<std::uint32_t>(Register::XMM2); reg_num <= static_cast<std::uint32_t>(Register::XMM6); reg_num++)
				add_register(flags, info, static_cast<Register>(reg_num), OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_pop3x8:
		command_pop(instruction, info, flags, 3, 8);
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRbx_CRsi_CRdi_CRes_CWsi_RCWax_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::BX, OpAccess::CondRead);
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::AX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRebx_CResi_CRedi_CRes_CWesi_RCWeax_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::EAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRrbx_CRrsi_CRrdi_CRes_CWrsi_RCWrax_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RBX, OpAccess::CondRead);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RAX, OpAccess::ReadCondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CRax_CRdx_CRbx_CRsi_CRdi_CRes_CWsi_CWdi_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::DX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::BX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::AX, OpAccess::CondRead);
			add_register(flags, info, Register::DX, OpAccess::CondRead);
			add_register(flags, info, Register::BX, OpAccess::CondRead);
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::DI, OpAccess::CondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CReax_CRedx_CRebx_CResi_CRedi_CRes_CWesi_CWedi_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::EDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::CondRead);
			add_register(flags, info, Register::EDX, OpAccess::CondRead);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::EDI, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CRmem_CWmem_CRrax_CRrdx_CRrbx_CRrsi_CRrdi_CRes_CWrsi_CWrdi_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RDX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RAX, OpAccess::CondRead);
			add_register(flags, info, Register::RDX, OpAccess::CondRead);
			add_register(flags, info, Register::RBX, OpAccess::CondRead);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RDI, OpAccess::CondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_gpr16_Wgs:
		command_last_gpr(instruction, info, flags, Register::AX);
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::GS, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Wrsp_Wcs_Wss_pop6x8:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RSP, OpAccess::Write);
			add_register(flags, info, Register::CS, OpAccess::Write);
			add_register(flags, info, Register::SS, OpAccess::Write);
		}
		command_pop(instruction, info, flags, 6, 8);
		break;
	case ImpliedAccess::t_Rcs_Rss_Wrsp_pop6x8:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::CS, OpAccess::Read);
			add_register(flags, info, Register::SS, OpAccess::Read);
			add_register(flags, info, Register::RSP, OpAccess::Write);
		}
		command_pop(instruction, info, flags, 6, 8);
		break;
	case ImpliedAccess::t_Reax_Recx_Wedx_Webx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Write);
			add_register(flags, info, Register::EBX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Reax_Recx_Redx_CRebx_CWedx_CWebx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::EAX, OpAccess::Read);
			add_register(flags, info, Register::ECX, OpAccess::Read);
			add_register(flags, info, Register::EDX, OpAccess::Read);
			add_register(flags, info, Register::EBX, OpAccess::CondRead);
			add_register(flags, info, Register::EDX, OpAccess::CondWrite);
			add_register(flags, info, Register::EBX, OpAccess::CondWrite);
		}
		break;
	case ImpliedAccess::t_memdisplm64:
		command_mem_displ(info, flags, -64);
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRsi_CRdi_CRes_CWsi_RCWcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::SI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code16, 0);
			add_memory(info, Register::ES, Register::DI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code16, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::SI, OpAccess::CondRead);
			add_register(flags, info, Register::DI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::SI, OpAccess::CondWrite);
			add_register(flags, info, Register::CX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CResi_CRedi_CRes_CWesi_RCWecx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::ESI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code32, 0);
			add_memory(info, Register::ES, Register::EDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code32, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::ESI, OpAccess::CondRead);
			add_register(flags, info, Register::EDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::ESI, OpAccess::CondWrite);
			add_register(flags, info, Register::ECX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_CWmem_CRrsi_CRrdi_CRes_CWrsi_RCWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RSI, OpAccess::CondWrite);
			add_register(flags, info, Register::RCX, OpAccess::ReadCondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CRmem_Rrcx_CRrsi_CRrdi_CRes_CRds_CWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::DS, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::DS, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RCX, OpAccess::CondWrite);
		}
		break;
	case ImpliedAccess::t_CRmem_CWmem_Rrcx_CRrsi_CRrdi_CRes_CRds_CWrcx:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::DS, Register::RSI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondRead, CodeSize::Code64, 0);
			add_memory(info, Register::ES, Register::RDI, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::CondWrite, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::RSI, OpAccess::CondRead);
			add_register(flags, info, Register::RDI, OpAccess::CondRead);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::ES, OpAccess::CondRead);
			}
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::DS, OpAccess::CondRead);
			}
			add_register(flags, info, Register::RCX, OpAccess::CondWrite);
		}
		break;
	case ImpliedAccess::t_Rdl_Rrax_Weax_Wrcx_Wrdx:
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::DL, OpAccess::Read);
			add_register(flags, info, Register::RAX, OpAccess::Read);
			add_register(flags, info, Register::EAX, OpAccess::Write);
			add_register(flags, info, Register::RCX, OpAccess::Write);
			add_register(flags, info, Register::RDX, OpAccess::Write);
		}
		break;
	case ImpliedAccess::t_Rmem_Wmem_Rrcx_Rrbx_Rds_Weax:
		if ((flags & Flags::NO_MEMORY_USAGE) == 0) {
			add_memory(info, Register::DS, Register::RBX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::Read, CodeSize::Code64, 0);
			add_memory(info, Register::DS, Register::RCX, Register::None, 1, 0x0, MemorySize::Unknown, OpAccess::Write, CodeSize::Code64, 0);
		}
		if ((flags & Flags::NO_REGISTER_USAGE) == 0) {
			add_register(flags, info, Register::RCX, OpAccess::Read);
			add_register(flags, info, Register::RBX, OpAccess::Read);
			if ((flags & Flags::IS_64BIT) == 0) {
				add_register(flags, info, Register::DS, OpAccess::Read);
			}
			add_register(flags, info, Register::EAX, OpAccess::Write);
		}
		break;
	// GENERATOR-END: ImpliedAccessHandler
	default:
		ICED_UNREACHABLE();
	}
}

} // namespace internal

InstructionInfo::InstructionInfo(std::uint32_t options) : op_accesses_{} {
	if ((options & InstructionInfoOptions::NO_REGISTER_USAGE) == 0)
		used_registers_.reserve(internal::InstrInfoConstants::DEFAULT_USED_REGISTER_COLL_CAPACITY);
	if ((options & InstructionInfoOptions::NO_MEMORY_USAGE) == 0)
		used_memory_locations_.reserve(internal::InstrInfoConstants::DEFAULT_USED_MEMORY_COLL_CAPACITY);
}

InstructionInfoFactory::InstructionInfoFactory() : info_(0) {}

const InstructionInfo& InstructionInfoFactory::info(const Instruction& instruction) {
	return internal::InstructionInfoFactoryImpl::create(info_, instruction, InstructionInfoOptions::NONE);
}

const InstructionInfo& InstructionInfoFactory::info_options(const Instruction& instruction, std::uint32_t options) {
	return internal::InstructionInfoFactoryImpl::create(info_, instruction, options);
}

} // namespace iced_x86
