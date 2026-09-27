// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_asm/code_label.hpp"
#include "iced_x86/code_asm/mem.hpp"
#include "iced_x86/code_asm/op_state.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/memory_operand.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace iced_x86::code_asm {

class CodeAssembler;

namespace internal {

/// Base class of `CodeAssembler`: its state and the options/error methods.
///
/// The instruction methods (thousands of overloads) are declared in the generated classes `CodeAssemblerFns0`,
/// `CodeAssemblerFns1`, etc (code_assembler_fns.hpp). `CodeAssembler` derives from the last one:
/// `CodeAssemblerBase <- CodeAssemblerFns0 <- ... <- CodeAssemblerFnsN <- CodeAssembler`. All overloads of a mnemonic
/// are declared in the same class. Splitting them keeps the classes small which is much faster to compile (some
/// compilers are slow when a class has thousands of members).
///
/// This class is an implementation detail, use `CodeAssembler`.
class CodeAssemblerBase {
public:
	/// `true` if an error has been detected by one of the methods that add instructions, labels or data.
	/// The error is sticky: all later calls that add instructions, labels or data are ignored until `clear_error()`
	/// (or `reset()`) is called. `assemble()` fails with this error.
	[[nodiscard]] bool has_error() const noexcept { return error_.has_value(); }

	/// Gets the first detected error or `nullptr` if there's no error (see `has_error()`)
	[[nodiscard]] const IcedError* error() const noexcept { return error_ ? &*error_ : nullptr; }

	/// Clears the error (see `has_error()`). Pending prefixes and labels aren't cleared.
	void clear_error() noexcept { error_.reset(); }

	/// Gets the bitness (16, 32 or 64)
	[[nodiscard]] std::uint32_t bitness() const noexcept { return bitness_; }

	/// `true` (default value) to use `VEX` encoding intead of `EVEX` encoding if we must pick one of the encodings.
	/// See also `vex()` and `evex()`
	[[nodiscard]] bool prefer_vex() const noexcept { return (options_ & CodeAssemblerOptions::PREFER_VEX) != 0; }

	/// `true` (default value) to use `VEX` encoding intead of `EVEX` encoding if we must pick one of the encodings.
	/// See also `vex()` and `evex()`
	///
	/// @param new_value New value
	void set_prefer_vex(bool new_value) noexcept {
		if (new_value)
			options_ |= CodeAssemblerOptions::PREFER_VEX;
		else
			options_ &= static_cast<std::uint8_t>(~CodeAssemblerOptions::PREFER_VEX);
	}

	/// `true` (default value) to create short branches, `false` to create near branches.
	[[nodiscard]] bool prefer_short_branch() const noexcept { return (options_ & CodeAssemblerOptions::PREFER_SHORT_BRANCH) != 0; }

	/// `true` (default value) to create short branches, `false` to create near branches.
	///
	/// @param new_value New value
	void set_prefer_short_branch(bool new_value) noexcept {
		if (new_value)
			options_ |= CodeAssemblerOptions::PREFER_SHORT_BRANCH;
		else
			options_ &= static_cast<std::uint8_t>(~CodeAssemblerOptions::PREFER_SHORT_BRANCH);
	}

protected:
	struct PrefixFlags {
		static constexpr std::uint8_t NONE = 0x00;
		static constexpr std::uint8_t LOCK = 0x01;
		static constexpr std::uint8_t REPE = 0x02;
		static constexpr std::uint8_t REPNE = 0x04;
		static constexpr std::uint8_t NOTRACK = 0x08;
		static constexpr std::uint8_t PREFER_VEX = 0x10;
		static constexpr std::uint8_t PREFER_EVEX = 0x20;
	};

	struct CodeAssemblerOptions {
		static constexpr std::uint8_t PREFER_VEX = 0x01;
		static constexpr std::uint8_t PREFER_SHORT_BRANCH = 0x02;
	};

	explicit CodeAssemblerBase(std::uint32_t bitness);

	/// Gets the derived `CodeAssembler` (all instances are `CodeAssembler`s)
	CodeAssembler& self() noexcept;

	bool instruction_prefer_vex() const noexcept {
		if ((prefix_flags_ & (PrefixFlags::PREFER_VEX | PrefixFlags::PREFER_EVEX)) != 0)
			return (prefix_flags_ & PrefixFlags::PREFER_VEX) != 0;
		return (options_ & CodeAssemblerOptions::PREFER_VEX) != 0;
	}

	MemoryOperand to_memory_operand(const AsmMemoryOperand& mem) const noexcept {
		const std::uint32_t displ_size = mem.is_displacement_only() ? bitness_ / 8 : (mem.displacement() != 0 ? 1 : 0);
		return MemoryOperand(mem.base(), mem.index(), mem.scale(), mem.displacement(), displ_size, mem.is_broadcast(), mem.segment());
	}

	CodeAssembler& add_instr(Instruction instruction);
	CodeAssembler& add_instr(const Result<Instruction>& instruction);
	CodeAssembler& add_instr_with_state(const Result<Instruction>& instruction, CodeAsmOpState state);
	CodeAssembler& set_error(const char* message);
	CodeAssembler& set_error(const IcedError& error);

	std::uint32_t bitness_;
	std::vector<Instruction> instructions_;
	std::uint64_t current_label_id_;
	CodeLabel current_label_;
	CodeLabel current_anon_label_;
	CodeLabel next_anon_label_;
	bool defined_anon_label_;
	std::uint8_t prefix_flags_;
	std::uint8_t options_;
	std::optional<IcedError> error_;
};

} // namespace internal
} // namespace iced_x86::code_asm
