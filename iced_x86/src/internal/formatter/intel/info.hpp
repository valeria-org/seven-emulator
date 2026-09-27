// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/intel/info.rs (InstrOpInfo, InstrInfo) and formatter/intel/fmt_tbl.rs (ALL_INFOS)

#pragma once

#include <array>
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
#include "internal/formatter/intel/instr_op_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal::intel {

constexpr std::int8_t OP_ACCESS_INVALID = -1;

struct InstrInfoConstants {
	static constexpr std::int8_t OP_ACCESS_NONE = static_cast<std::int8_t>(-(static_cast<std::int32_t>(OpAccess::None) + 2));
	static constexpr std::int8_t OP_ACCESS_READ = static_cast<std::int8_t>(-(static_cast<std::int32_t>(OpAccess::Read) + 2));
	static constexpr std::int8_t OP_ACCESS_WRITE = static_cast<std::int8_t>(-(static_cast<std::int32_t>(OpAccess::Write) + 2));
	static constexpr std::int8_t OP_ACCESS_READ_WRITE = static_cast<std::int8_t>(-(static_cast<std::int32_t>(OpAccess::ReadWrite) + 2));
};

/// Operands, mnemonic and flags of an instruction
struct InstrOpInfo {
	FormatterString mnemonic;
	std::uint16_t flags; // InstrOpInfoFlags
	std::uint8_t op_count;
	std::array<InstrOpKind, IcedConstants::MAX_OP_COUNT> op_kinds;
	std::array<Register, IcedConstants::MAX_OP_COUNT> op_registers;
	std::array<std::int8_t, IcedConstants::MAX_OP_COUNT> op_indexes;

	/// Rust: `InstrOpInfo::default()`
	explicit InstrOpInfo(FormatterString mnemonic_) noexcept
		: mnemonic(mnemonic_), flags(0), op_count(0), op_kinds{}, op_registers{}, op_indexes{} {}

	/// Rust: `InstrOpInfo::new()`
	InstrOpInfo(FormatterString mnemonic_, const Instruction& instruction, std::uint32_t flags_) noexcept;

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
		ICED_DEBUG_ASSERT(op_kinds[0] == InstrOpKind::DeclareByte || op_kinds[0] == InstrOpKind::DeclareWord ||
						  op_kinds[0] == InstrOpKind::DeclareDword || op_kinds[0] == InstrOpKind::DeclareQword);
		return op_kinds[0];
	}

	std::optional<std::uint32_t> instruction_index(std::uint32_t operand) const noexcept {
		std::int8_t instruction_operand;
		if (operand < IcedConstants::MAX_OP_COUNT)
			instruction_operand = op_indexes[operand];
		else {
			ICED_DEBUG_ASSERT(op_kinds[0] == InstrOpKind::DeclareByte || op_kinds[0] == InstrOpKind::DeclareWord ||
							  op_kinds[0] == InstrOpKind::DeclareDword || op_kinds[0] == InstrOpKind::DeclareQword);
			instruction_operand = -1;
		}
		// Read the result from a table: if it's created with `std::optional(value)`, some compilers (eg. GCC) write the
		// value and the flag with 2 stores and then read it with one 8-byte load which causes a store forwarding stall.
		static constexpr std::optional<std::uint32_t> INSTRUCTION_OPERANDS[IcedConstants::MAX_OP_COUNT + 1] = {std::nullopt, 0, 1, 2, 3, 4};
		if (instruction_operand < 0)
			return INSTRUCTION_OPERANDS[0];
		ICED_ASSERT(instruction_operand < static_cast<std::int8_t>(IcedConstants::MAX_OP_COUNT));
		return INSTRUCTION_OPERANDS[static_cast<std::size_t>(instruction_operand) + 1];
	}

	std::optional<OpAccess> op_access(std::uint32_t operand) const noexcept {
		std::int8_t instruction_operand;
		if (operand < IcedConstants::MAX_OP_COUNT)
			instruction_operand = op_indexes[operand];
		else {
			ICED_DEBUG_ASSERT(op_kinds[0] == InstrOpKind::DeclareByte || op_kinds[0] == InstrOpKind::DeclareWord ||
							  op_kinds[0] == InstrOpKind::DeclareDword || op_kinds[0] == InstrOpKind::DeclareQword);
			instruction_operand = op_indexes[0];
		}
		if (instruction_operand < OP_ACCESS_INVALID)
			return static_cast<OpAccess>(-instruction_operand - 2);
		return std::nullopt;
	}

	std::optional<std::uint32_t> operand_index(std::uint32_t instruction_operand) const noexcept {
		std::int32_t index;
		if (instruction_operand == static_cast<std::uint32_t>(op_indexes[0]))
			index = 0;
		else if (instruction_operand == static_cast<std::uint32_t>(op_indexes[1]))
			index = 1;
		else if (instruction_operand == static_cast<std::uint32_t>(op_indexes[2]))
			index = 2;
		else if (instruction_operand == static_cast<std::uint32_t>(op_indexes[3]))
			index = 3;
		else if (instruction_operand == static_cast<std::uint32_t>(op_indexes[4]))
			index = 4;
		else
			index = -1;
		if (static_cast<std::uint32_t>(index) < op_count)
			return static_cast<std::uint32_t>(index);
		return std::nullopt;
	}
};

/// Creates the `InstrOpInfo` of an instruction (Rust: `ALL_INFOS[code].op_info()`). The instruction infos are constant data
/// (`INSTR_INFOS` in the generated fmt_data.cpp).
InstrOpInfo get_op_info(const FormatterOptions& options, const Instruction& instruction) noexcept;

} // namespace iced_x86::internal::intel
