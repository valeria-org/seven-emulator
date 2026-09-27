// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/encoder.hpp"
#include "internal/encoder/displ_size.hpp"
#include "internal/encoder/imm_size.hpp"
#include "internal/encoder/op_code_handler.hpp"
#include "internal/iced_assert.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace iced_x86::internal {

// Gives the encoder internals (operand handlers, op code handlers, block encoder) access to the encoder's
// private state. It's the equivalent of Rust's `pub(super)`/`pub(crate)` encoder fields and methods.
struct EncoderInternal {
	static constexpr const char* ERROR_ONLY_1632_BIT_MODE = "The instruction can only be used in 16/32-bit mode";
	static constexpr const char* ERROR_ONLY_64_BIT_MODE = "The instruction can only be used in 64-bit mode";

	// Fields

	static std::uint32_t& encoder_flags(Encoder& e) noexcept { return e.encoder_flags_; }
	static ImmSize& imm_size(Encoder& e) noexcept { return e.imm_size_; }
	static std::uint32_t& immediate(Encoder& e) noexcept { return e.immediate_; }
	static std::uint32_t& immediate_hi(Encoder& e) noexcept { return e.immediate_hi_; }
	static std::uint32_t& op_code(Encoder& e) noexcept { return e.op_code_; }
	static std::uint32_t prevent_vex2(const Encoder& e) noexcept { return e.prevent_vex2_; }
	static std::uint32_t internal_vex_wig_lig(const Encoder& e) noexcept { return e.internal_vex_wig_lig_; }
	static std::uint32_t internal_vex_lig(const Encoder& e) noexcept { return e.internal_vex_lig_; }
	static std::uint32_t internal_evex_wig(const Encoder& e) noexcept { return e.internal_evex_wig_; }
	static std::uint32_t internal_evex_lig(const Encoder& e) noexcept { return e.internal_evex_lig_; }
	static std::uint32_t internal_mvex_wig(const Encoder& e) noexcept { return e.internal_mvex_wig_; }

	// Methods

	static void write_byte_internal(Encoder& e, std::uint32_t value) {
		// Keep the common case (no reallocation) inline, like Rust's `Vec::push()`
		if (ICED_LIKELY(e.buffer_.size() != e.buffer_.capacity()))
			e.buffer_.push_back(static_cast<std::uint8_t>(value));
		else
			write_byte_grow(e, value);
		e.current_rip_++;
	}

	// Used by the block encoder
	static std::size_t position(const Encoder& e) noexcept { return e.buffer_.size(); }
	// Used by the block encoder
	static void clear_buffer(Encoder& e) noexcept { e.buffer_.clear(); }

	static void set_error_message(Encoder& e, std::string message) {
		if (e.error_message_.empty())
			e.error_message_ = std::move(message);
	}

	static void set_error_message_str(Encoder& e, const char* message) {
		if (e.error_message_.empty())
			e.error_message_.append(message);
	}

	static bool verify_op_kind(Encoder& e, std::uint32_t operand, OpKind expected, OpKind actual) {
		if (expected == actual)
			return true;
		verify_op_kind_failed(e, operand, expected, actual);
		return false;
	}

	static bool verify_register(Encoder& e, std::uint32_t operand, Register expected, Register actual) {
		if (expected == actual)
			return true;
		verify_register_failed(e, operand, expected, actual);
		return false;
	}

	static bool verify_register_range(Encoder& e, std::uint32_t operand, Register register_, Register reg_lo, Register reg_hi) {
		// In 16/32-bit mode, only the low 8 regs are used, but callers pass in all 16 (or 32) regs
		// that are valid in 64-bit mode. Update reg_hi so only the first 8 regs can be used.
		if (e.bitness_ != 64 && static_cast<std::uint32_t>(reg_hi) > static_cast<std::uint32_t>(reg_lo) + 7)
			// reg_lo+7 is a valid reg (eg. EAX+7 = EDI) and all groups of regs (eg. gpr64) use consecutive
			// enum values so it's safe to add 7 to get the 8th reg in the same reg group
			reg_hi = static_cast<Register>(static_cast<std::uint32_t>(reg_lo) + 7);
		if (reg_lo <= register_ && register_ <= reg_hi)
			return true;
		verify_register_range_failed(e, operand, register_, reg_lo, reg_hi);
		return false;
	}

	static void add_branch(Encoder& e, OpKind op_kind, std::uint32_t imm_size, const Instruction& instruction, std::uint32_t operand);
	static void add_branch_x(Encoder& e, std::uint32_t imm_size, const Instruction& instruction, std::uint32_t operand);
	static void add_branch_disp(Encoder& e, std::uint32_t displ_size, const Instruction& instruction, std::uint32_t operand);
	static void add_far_branch(Encoder& e, const Instruction& instruction, std::uint32_t operand, std::uint32_t size);
	static void set_addr_size(Encoder& e, std::uint32_t reg_size);
	static void add_abs_mem(Encoder& e, const Instruction& instruction, std::uint32_t operand);
	static void add_mod_rm_register(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi);
	static void add_reg(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi);
	static void add_reg_or_mem(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi, bool allow_mem_op,
		bool allow_reg_op) {
		add_reg_or_mem_full(e, instruction, operand, reg_lo, reg_hi, Register::None, Register::None, allow_mem_op, allow_reg_op);
	}
	static void add_reg_or_mem_full(Encoder& e, const Instruction& instruction, std::uint32_t operand, Register reg_lo, Register reg_hi,
		Register vsib_index_reg_lo, Register vsib_index_reg_hi, bool allow_mem_op, bool allow_reg_op);
	static void write_prefixes(Encoder& e, const Instruction& instruction, bool can_write_f3);

	// Implemented in encoder.cpp, used by `Encoder::encode()`
	static std::uint32_t get_register_op_size(const Instruction& instruction);
	static std::optional<std::int8_t> try_convert_to_disp8n(Encoder& e, const Instruction& instruction, std::int32_t displ);
	static void add_mem_op16(Encoder& e, const Instruction& instruction, std::uint32_t operand);
	static void add_mem_op(Encoder& e, const Instruction& instruction, std::uint32_t operand, std::uint32_t addr_size, Register vsib_index_reg_lo,
		Register vsib_index_reg_hi);
	static void write_mod_rm(Encoder& e);
	static void write_immediate(Encoder& e);

private:
	ICED_NOINLINE static void write_byte_grow(Encoder& e, std::uint32_t value);
	ICED_NOINLINE static void verify_op_kind_failed(Encoder& e, std::uint32_t operand, OpKind expected, OpKind actual);
	ICED_NOINLINE static void verify_register_failed(Encoder& e, std::uint32_t operand, Register expected, Register actual);
	ICED_NOINLINE static void verify_register_range_failed(Encoder& e, std::uint32_t operand, Register register_, Register reg_lo, Register reg_hi);
};

} // namespace iced_x86::internal
