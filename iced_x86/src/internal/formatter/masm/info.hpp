// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Masm formatter instruction infos (Rust: formatter/masm/info.rs, fmt_tbl.rs)

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "iced_x86/formatter_options.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/op_access.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "internal/formatter/formatter_string.hpp"
#include "internal/formatter/masm/instr_op_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal::masm {

constexpr std::int8_t OP_ACCESS_INVALID = -1;

struct InstrInfoConstants {
	static constexpr std::int8_t OP_ACCESS_NONE = -(static_cast<std::int8_t>(OpAccess::None) + 2);
	static constexpr std::int8_t OP_ACCESS_READ = -(static_cast<std::int8_t>(OpAccess::Read) + 2);
	static constexpr std::int8_t OP_ACCESS_COND_READ = -(static_cast<std::int8_t>(OpAccess::CondRead) + 2);
	static constexpr std::int8_t OP_ACCESS_READ_WRITE = -(static_cast<std::int8_t>(OpAccess::ReadWrite) + 2);
};

/// Formatter operands of an instruction (Rust: `InstrOpInfo`)
struct InstrOpInfo {
	FormatterString mnemonic;
	// It's a u16 in Rust but GCC can load 32 bits after a 16-bit store (store forwarding stall)
	std::uint32_t flags; // InstrOpInfoFlags
	std::uint8_t op_count;
	InstrOpKind op_kinds[IcedConstants::MAX_OP_COUNT];
	Register op_registers[IcedConstants::MAX_OP_COUNT];
	std::int8_t op_indexes[IcedConstants::MAX_OP_COUNT];

	static InstrOpKind to_instr_op_kind(OpKind op_kind) noexcept {
		// All OpKind values are valid InstrOpKind values
		return static_cast<InstrOpKind>(op_kind);
	}

	Register op_register(std::uint32_t operand) const noexcept {
		ICED_ASSERT(operand < IcedConstants::MAX_OP_COUNT);
		return op_registers[operand];
	}

	InstrOpKind op_kind(std::uint32_t operand) const noexcept {
		if (operand < IcedConstants::MAX_OP_COUNT)
			return op_kinds[operand];
		ICED_DEBUG_ASSERT(is_declare_data());
		return op_kinds[0];
	}

	std::optional<std::uint32_t> instruction_index(std::uint32_t operand) const noexcept {
		std::int8_t instruction_operand;
		if (operand < IcedConstants::MAX_OP_COUNT)
			instruction_operand = op_indexes[operand];
		else {
			ICED_DEBUG_ASSERT(is_declare_data());
			instruction_operand = -1;
		}
		// A table is used because GCC creates the `std::optional` on the stack with two stores (value + has_value), and the
		// following 8-byte load (it's passed in a register) can't be forwarded from the stores (a ~15 cycle stall)
		if (instruction_operand < 0)
			return INSTRUCTION_INDEXES[NO_INSTRUCTION_INDEX];
		ICED_DEBUG_ASSERT(static_cast<std::uint32_t>(instruction_operand) < IcedConstants::MAX_OP_COUNT);
		return INSTRUCTION_INDEXES[static_cast<std::uint32_t>(instruction_operand)];
	}

	std::optional<OpAccess> op_access(std::uint32_t operand) const noexcept {
		std::int8_t instruction_operand;
		if (operand < IcedConstants::MAX_OP_COUNT)
			instruction_operand = op_indexes[operand];
		else {
			ICED_DEBUG_ASSERT(is_declare_data());
			instruction_operand = op_indexes[0];
		}
		if (instruction_operand < OP_ACCESS_INVALID)
			return static_cast<OpAccess>(-instruction_operand - 2);
		return std::nullopt;
	}

	std::optional<std::uint32_t> operand_index(std::uint32_t instruction_operand) const noexcept {
		std::uint32_t index;
		if (instruction_operand == static_cast<std::uint32_t>(static_cast<std::int32_t>(op_indexes[0])))
			index = 0;
		else if (instruction_operand == static_cast<std::uint32_t>(static_cast<std::int32_t>(op_indexes[1])))
			index = 1;
		else if (instruction_operand == static_cast<std::uint32_t>(static_cast<std::int32_t>(op_indexes[2])))
			index = 2;
		else if (instruction_operand == static_cast<std::uint32_t>(static_cast<std::int32_t>(op_indexes[3])))
			index = 3;
		else if (instruction_operand == static_cast<std::uint32_t>(static_cast<std::int32_t>(op_indexes[4])))
			index = 4;
		else
			index = 0xFFFF'FFFF;
		if (index < op_count)
			return index;
		return std::nullopt;
	}

	/// Rust: `InstrOpInfo::default()`
	static InstrOpInfo with_default(FormatterString mnemonic) noexcept {
		InstrOpInfo res;
		res.mnemonic = mnemonic;
		res.flags = 0;
		res.op_count = 0;
		for (std::size_t i = 0; i < IcedConstants::MAX_OP_COUNT; i++) {
			res.op_kinds[i] = InstrOpKind::Register;
			res.op_registers[i] = Register::None;
			res.op_indexes[i] = 0;
		}
		return res;
	}

	/// Rust: `InstrOpInfo::new()`
	static InstrOpInfo with_instruction(FormatterString mnemonic, const Instruction& instruction, std::uint32_t flags) noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		InstrOpInfo res;
		res.mnemonic = mnemonic;
		res.flags = flags;
		res.op_kinds[0] = to_instr_op_kind(instruction.op0_kind());
		res.op_kinds[1] = to_instr_op_kind(instruction.op1_kind());
		res.op_kinds[2] = to_instr_op_kind(instruction.op2_kind());
		res.op_kinds[3] = to_instr_op_kind(instruction.op3_kind());
		res.op_kinds[4] = to_instr_op_kind(instruction.op4_kind());
		res.op_registers[0] = instruction.op0_register();
		res.op_registers[1] = instruction.op1_register();
		res.op_registers[2] = instruction.op2_register();
		res.op_registers[3] = instruction.op3_register();
		res.op_registers[4] = instruction.op4_register();
		const std::uint32_t op_count = instruction.op_count();
		res.op_count = static_cast<std::uint8_t>(op_count);
		res.op_indexes[0] = 0;
		switch (op_count) {
		case 0:
			res.op_indexes[0] = OP_ACCESS_INVALID;
			res.op_indexes[1] = OP_ACCESS_INVALID;
			res.op_indexes[2] = OP_ACCESS_INVALID;
			res.op_indexes[3] = OP_ACCESS_INVALID;
			res.op_indexes[4] = OP_ACCESS_INVALID;
			break;
		case 1:
			res.op_indexes[1] = OP_ACCESS_INVALID;
			res.op_indexes[2] = OP_ACCESS_INVALID;
			res.op_indexes[3] = OP_ACCESS_INVALID;
			res.op_indexes[4] = OP_ACCESS_INVALID;
			break;
		case 2:
			res.op_indexes[1] = 1;
			res.op_indexes[2] = OP_ACCESS_INVALID;
			res.op_indexes[3] = OP_ACCESS_INVALID;
			res.op_indexes[4] = OP_ACCESS_INVALID;
			break;
		case 3:
			res.op_indexes[1] = 1;
			res.op_indexes[2] = 2;
			res.op_indexes[3] = OP_ACCESS_INVALID;
			res.op_indexes[4] = OP_ACCESS_INVALID;
			break;
		case 4:
			res.op_indexes[1] = 1;
			res.op_indexes[2] = 2;
			res.op_indexes[3] = 3;
			res.op_indexes[4] = OP_ACCESS_INVALID;
			break;
		case 5:
			res.op_indexes[1] = 1;
			res.op_indexes[2] = 2;
			res.op_indexes[3] = 3;
			res.op_indexes[4] = 4;
			break;
		default:
			ICED_UNREACHABLE();
		}
		return res;
	}

private:
	static constexpr std::size_t NO_INSTRUCTION_INDEX = IcedConstants::MAX_OP_COUNT;
	static constexpr std::optional<std::uint32_t> INSTRUCTION_INDEXES[IcedConstants::MAX_OP_COUNT + 1] = {0U, 1U, 2U, 3U, 4U, std::nullopt};
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");

	bool is_declare_data() const noexcept {
		return op_kinds[0] == InstrOpKind::DeclareByte || op_kinds[0] == InstrOpKind::DeclareWord || op_kinds[0] == InstrOpKind::DeclareDword ||
			   op_kinds[0] == InstrOpKind::DeclareQword;
	}
};

/// Creates the formatter operands of an instruction (Rust: `ALL_INFOS[code].op_info()`). The instruction infos are constant
/// data (`INSTR_INFOS` in the generated fmt_data.cpp).
InstrOpInfo get_op_info(const FormatterOptions& options, const Instruction& instruction) noexcept;

} // namespace iced_x86::internal::masm
