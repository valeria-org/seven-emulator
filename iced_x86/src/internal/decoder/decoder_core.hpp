// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Internal decoder header: the hot inline `DecoderCore` methods and the `OpCodeHandler` base struct.
// Included by all decoder .cpp files.

#pragma once

#include "iced_x86/decoder.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/tuple_type.hpp"
#include "internal/decoder/handler_flags.hpp"
#include "internal/decoder/op_size.hpp"
#include "internal/decoder/state_flags.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instr_scale.hpp"
#include "internal/instruction_internal.hpp"
#include "internal/tuple_type_tbl.hpp"
#include "internal/vector_length.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace iced_x86::internal {

static_assert(DecoderCore::SF_IP_REL64 == StateFlags::IP_REL64, "");
static_assert(DecoderCore::SF_IP_REL32 == StateFlags::IP_REL32, "");
static_assert(DecoderCore::SF_HAS_REX == StateFlags::HAS_REX, "");
static_assert(DecoderCore::SF_IS_INVALID == StateFlags::IS_INVALID, "");
static_assert(DecoderCore::SF_W == StateFlags::W, "");
static_assert(DecoderCore::SF_LOCK == StateFlags::LOCK, "");
static_assert(DecoderCore::SF_NO_MORE_BYTES == StateFlags::NO_MORE_BYTES, "");
static_assert(DecoderCore::OP_SIZE64 == static_cast<std::uint8_t>(OpSize::Size64), "");

ICED_FORCE_INLINE constexpr HandlerEntry to_handler_entry(const OpCodeHandler* handler) noexcept { return HandlerEntry{handler->decode, handler}; }

// Casts `self_ptr` (the handler passed to its decode fn) to the real handler type
template <typename T>
ICED_FORCE_INLINE const T& handler_this(const OpCodeHandler* self_ptr) noexcept {
	return *static_cast<const T*>(self_ptr);
}

ICED_FORCE_INLINE void write_op0_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_op0_register(static_cast<Register>(reg));
}

ICED_FORCE_INLINE void write_op1_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_op1_register(static_cast<Register>(reg));
}

ICED_FORCE_INLINE void write_op2_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_op2_register(static_cast<Register>(reg));
}

ICED_FORCE_INLINE void write_op3_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_op3_register(static_cast<Register>(reg));
}

ICED_FORCE_INLINE void write_base_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_memory_base(static_cast<Register>(reg));
}

ICED_FORCE_INLINE void write_index_reg(Instruction& instruction, std::uint32_t reg) noexcept {
	ICED_DEBUG_ASSERT(reg < IcedConstants::REGISTER_ENUM_COUNT);
	instruction.set_memory_index(static_cast<Register>(reg));
}

ICED_FORCE_INLINE constexpr std::uint32_t reg_u32(Register reg) noexcept { return static_cast<std::uint32_t>(reg); }

static_assert(static_cast<std::uint32_t>(OpSize::Size16) == 0, "");
static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
static_assert(static_cast<std::uint32_t>(VectorLength::Unknown) == 3, "");

// ---------------------------------------------------------------------------------------------------------------------
// DecoderState

ICED_FORCE_INLINE std::uint32_t DecoderState::encoding() const noexcept {
#ifndef NDEBUG
	return (flags >> StateFlags::ENCODING_SHIFT) & StateFlags::ENCODING_MASK;
#else
	return static_cast<std::uint32_t>(EncodingKind::Legacy);
#endif
}

ICED_FORCE_INLINE std::uint32_t DecoderState::sss() const noexcept { return (flags >> StateFlags::MVEX_SSS_SHIFT) & StateFlags::MVEX_SSS_MASK; }

// ---------------------------------------------------------------------------------------------------------------------
// Readers

namespace decoder_detail {
template <typename T>
ICED_FORCE_INLINE T read_le(std::uintptr_t ptr) noexcept {
	T value;
	std::memcpy(&value, reinterpret_cast<const void*>(ptr), sizeof(T));
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	if constexpr (sizeof(T) == 2)
		value = static_cast<T>(__builtin_bswap16(value));
	else if constexpr (sizeof(T) == 4)
		value = static_cast<T>(__builtin_bswap32(value));
	else if constexpr (sizeof(T) == 8)
		value = static_cast<T>(__builtin_bswap64(value));
#endif
	return value;
}
} // namespace decoder_detail

// Same as Rust's `mk_read_xx!()`: returns false if there's not enough bytes left (and doesn't update any state)
#define ICED_DECODER_TRY_READ(self, mem_ty, result_ty, value) \
	do { \
		constexpr std::size_t ICED_SIZE = sizeof(mem_ty); \
		static_assert(ICED_SIZE >= 1 && ICED_SIZE <= DecoderCore::MAX_READ_SIZE, ""); \
		const std::uintptr_t ICED_DATA_PTR = (self).data_ptr; \
		/* This doesn't overflow data_ptr (verified in ctor since SIZE <= MAX_READ_SIZE) */ \
		if (ICED_LIKELY(ICED_DATA_PTR + ICED_SIZE - 1 < (self).max_data_ptr)) { \
			(value) = static_cast<result_ty>(decoder_detail::read_le<mem_ty>(ICED_DATA_PTR)); \
			(self).data_ptr = ICED_DATA_PTR + ICED_SIZE; \
			return true; \
		} \
		return false; \
	} while (0)

ICED_FORCE_INLINE bool DecoderCore::try_read_u8(std::size_t& value) noexcept { ICED_DECODER_TRY_READ(*this, std::uint8_t, std::size_t, value); }
ICED_FORCE_INLINE bool DecoderCore::try_read_u16(std::size_t& value) noexcept { ICED_DECODER_TRY_READ(*this, std::uint16_t, std::size_t, value); }
ICED_FORCE_INLINE bool DecoderCore::try_read_u32(std::size_t& value) noexcept { ICED_DECODER_TRY_READ(*this, std::uint32_t, std::size_t, value); }

#undef ICED_DECODER_TRY_READ

// Same as Rust's `mk_read_xx_fn_body!()`
#define ICED_DECODER_READ(mem_ty, result_ty) \
	constexpr std::size_t ICED_SIZE = sizeof(mem_ty); \
	static_assert(ICED_SIZE >= 1 && ICED_SIZE <= DecoderCore::MAX_READ_SIZE, ""); \
	const std::uintptr_t ICED_DATA_PTR = data_ptr; \
	/* This doesn't overflow data_ptr (verified in ctor since SIZE <= MAX_READ_SIZE) */ \
	if (ICED_LIKELY(ICED_DATA_PTR + ICED_SIZE - 1 < max_data_ptr)) { \
		result_ty result = static_cast<result_ty>(decoder_detail::read_le<mem_ty>(ICED_DATA_PTR)); \
		data_ptr = ICED_DATA_PTR + ICED_SIZE; \
		return result; \
	} \
	state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES; \
	return 0

ICED_FORCE_INLINE std::size_t DecoderCore::read_u16() noexcept { ICED_DECODER_READ(std::uint16_t, std::size_t); }
ICED_FORCE_INLINE std::size_t DecoderCore::read_u32() noexcept { ICED_DECODER_READ(std::uint32_t, std::size_t); }
ICED_FORCE_INLINE std::uint64_t DecoderCore::read_u64() noexcept { ICED_DECODER_READ(std::uint64_t, std::uint64_t); }

#undef ICED_DECODER_READ

// ---------------------------------------------------------------------------------------------------------------------
// Misc

ICED_FORCE_INLINE void DecoderCore::reset_rex_prefix_state() noexcept {
	state.flags &= ~(StateFlags::HAS_REX | StateFlags::W);
	if ((state.flags & StateFlags::HAS66) == 0)
		state.operand_size = default_operand_size;
	else
		state.operand_size = default_inverted_operand_size;
	state.extra_register_base = 0;
	state.extra_index_register_base = 0;
	state.extra_base_register_base = 0;
}

ICED_FORCE_INLINE void DecoderCore::call_opcode_handlers_map0_table(Instruction& instruction) noexcept {
	std::size_t b = read_u8();
	decode_table2(handlers_map0[b], instruction);
}

ICED_FORCE_INLINE std::uint32_t DecoderCore::current_ip32() const noexcept {
	ICED_DEBUG_ASSERT(instr_start_data_ptr <= data_ptr);
	ICED_DEBUG_ASSERT(data_ptr - instr_start_data_ptr <= IcedConstants::MAX_INSTRUCTION_LENGTH);
	return static_cast<std::uint32_t>(data_ptr - instr_start_data_ptr) + static_cast<std::uint32_t>(ip);
}

ICED_FORCE_INLINE std::uint64_t DecoderCore::current_ip64() const noexcept {
	ICED_DEBUG_ASSERT(instr_start_data_ptr <= data_ptr);
	ICED_DEBUG_ASSERT(data_ptr - instr_start_data_ptr <= IcedConstants::MAX_INSTRUCTION_LENGTH);
	return static_cast<std::uint64_t>(data_ptr - instr_start_data_ptr) + ip;
}

ICED_FORCE_INLINE void DecoderCore::clear_mandatory_prefix(Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	InstructionInternal::internal_clear_has_repe_repne_prefix(instruction);
}

ICED_FORCE_INLINE void DecoderCore::set_xacquire_xrelease(Instruction& instruction, std::uint32_t flags) noexcept {
	if (instruction.has_lock_prefix())
		set_xacquire_xrelease_core(instruction, flags);
}

ICED_FORCE_INLINE void DecoderCore::clear_mandatory_prefix_f3(Instruction& instruction) const noexcept {
	ICED_DEBUG_ASSERT(state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	ICED_DEBUG_ASSERT(state.mandatory_prefix == DecoderMandatoryPrefix::PF3);
	instruction.set_has_repe_prefix(false);
}

ICED_FORCE_INLINE void DecoderCore::clear_mandatory_prefix_f2(Instruction& instruction) const noexcept {
	ICED_DEBUG_ASSERT(state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	ICED_DEBUG_ASSERT(state.mandatory_prefix == DecoderMandatoryPrefix::PF2);
	instruction.set_has_repne_prefix(false);
}

ICED_FORCE_INLINE void DecoderCore::set_invalid_instruction() noexcept { state.flags |= StateFlags::IS_INVALID; }

ICED_FORCE_INLINE std::uint32_t DecoderCore::read_op_seg_reg() noexcept {
	std::uint32_t reg = state.reg;
	static_assert(reg_u32(Register::ES) + 1 == reg_u32(Register::CS), "");
	static_assert(reg_u32(Register::ES) + 2 == reg_u32(Register::SS), "");
	static_assert(reg_u32(Register::ES) + 3 == reg_u32(Register::DS), "");
	static_assert(reg_u32(Register::ES) + 4 == reg_u32(Register::FS), "");
	static_assert(reg_u32(Register::ES) + 5 == reg_u32(Register::GS), "");
	if (reg < 6)
		return reg_u32(Register::ES) + reg;
	set_invalid_instruction();
	return reg_u32(Register::None);
}

ICED_FORCE_INLINE std::uint32_t DecoderCore::disp8n(TupleType tuple_type) const noexcept {
	return get_disp8n(tuple_type, (state.flags & StateFlags::B) != 0);
}

// ---------------------------------------------------------------------------------------------------------------------
// Memory operands

using ReadOpMemFn = bool (*)(Instruction& instruction, DecoderCore& self) noexcept;
using ReadOpMemVsibFn = bool (*)(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;

inline constexpr ReadOpMemFn READ_OP_MEM_FNS[0x18] = {
	DecoderCore::read_op_mem_0,
	DecoderCore::read_op_mem_0,
	DecoderCore::read_op_mem_0,
	DecoderCore::read_op_mem_0,
	DecoderCore::read_op_mem_0_4,
	DecoderCore::read_op_mem_0_5,
	DecoderCore::read_op_mem_0,
	DecoderCore::read_op_mem_0,

	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1_4,
	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1,
	DecoderCore::read_op_mem_1,

	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2_4,
	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2,
	DecoderCore::read_op_mem_2,
};

inline constexpr ReadOpMemVsibFn READ_OP_MEM_VSIB_FNS[0x18] = {
	DecoderCore::read_op_mem_vsib_0,
	DecoderCore::read_op_mem_vsib_0,
	DecoderCore::read_op_mem_vsib_0,
	DecoderCore::read_op_mem_vsib_0,
	DecoderCore::read_op_mem_vsib_0_4,
	DecoderCore::read_op_mem_vsib_0_5,
	DecoderCore::read_op_mem_vsib_0,
	DecoderCore::read_op_mem_vsib_0,

	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1_4,
	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1,
	DecoderCore::read_op_mem_vsib_1,

	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2_4,
	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2,
	DecoderCore::read_op_mem_vsib_2,
};

// Returns `true` if the SIB byte was read
// This is a specialized version of read_op_mem_32_or_64_vsib() which takes less arguments. Keep them in sync.
ICED_FORCE_INLINE bool DecoderCore::read_op_mem_32_or_64(Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() != static_cast<std::uint32_t>(EncodingKind::EVEX) &&
					  state.encoding() != static_cast<std::uint32_t>(EncodingKind::MVEX));
	std::size_t index = state.mem_index;
	// index is valid because modrm.mod = 0-2 (never 3 if we're here) so index will always be 0-10_111 (17h)
	ICED_DEBUG_ASSERT(index < sizeof(READ_OP_MEM_FNS) / sizeof(READ_OP_MEM_FNS[0]));
	return READ_OP_MEM_FNS[index](instruction, *this);
}

// Same as Rust's `read_op_mem_stmt!()`
ICED_FORCE_INLINE void DecoderCore::read_op_mem(Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() != static_cast<std::uint32_t>(EncodingKind::EVEX) &&
					  state.encoding() != static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (state.address_size != OpSize::Size16)
		(void)read_op_mem_32_or_64(instruction);
	else
		read_op_mem_16(instruction, TupleType::N1);
}

ICED_FORCE_INLINE void DecoderCore::read_op_mem_sib(Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() != static_cast<std::uint32_t>(EncodingKind::EVEX) &&
					  state.encoding() != static_cast<std::uint32_t>(EncodingKind::MVEX));
	bool is_valid;
	if (state.address_size != OpSize::Size16)
		is_valid = read_op_mem_32_or_64(instruction);
	else {
		read_op_mem_16(instruction, TupleType::N1);
		is_valid = false;
	}
	if (invalid_check_mask != 0 && !is_valid)
		set_invalid_instruction();
}

// All MPX instructions in 64-bit mode force 64-bit addressing, and
// all MPX instructions in 16/32-bit mode require 32-bit addressing
// (see SDM Vol 1, 17.5.1 Intel MPX and Operating Modes)
ICED_FORCE_INLINE void DecoderCore::read_op_mem_mpx(Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() != static_cast<std::uint32_t>(EncodingKind::EVEX) &&
					  state.encoding() != static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (is64b_mode) {
		state.address_size = OpSize::Size64;
		(void)read_op_mem_32_or_64(instruction);
	}
	else if (state.address_size != OpSize::Size16)
		(void)read_op_mem_32_or_64(instruction);
	else {
		read_op_mem_16(instruction, TupleType::N1);
		if (invalid_check_mask != 0)
			set_invalid_instruction();
	}
}

// Returns `true` if the SIB byte was read
// Same as read_op_mem_32_or_64() except it works with vsib memory operands. Keep them in sync.
ICED_FORCE_INLINE bool DecoderCore::read_op_mem_32_or_64_vsib(Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	ICED_DEBUG_ASSERT(state.address_size == OpSize::Size32 || state.address_size == OpSize::Size64);
	std::size_t index = state.mem_index;
	// index is valid because modrm.mod = 0-2 (never 3 if we're here) so index will always be 0-10_111 (17h)
	ICED_DEBUG_ASSERT(index < sizeof(READ_OP_MEM_VSIB_FNS) / sizeof(READ_OP_MEM_VSIB_FNS[0]));
	return READ_OP_MEM_VSIB_FNS[index](*this, instruction, index_reg, tuple_type, is_vsib);
}

ICED_FORCE_INLINE void DecoderCore::read_op_mem_tuple_type(Instruction& instruction, TupleType tuple_type) noexcept {
	ICED_DEBUG_ASSERT(state.encoding() == static_cast<std::uint32_t>(EncodingKind::EVEX) ||
					  state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (state.address_size != OpSize::Size16) {
		Register index_reg = state.address_size == OpSize::Size64 ? Register::RAX : Register::EAX;
		(void)read_op_mem_32_or_64_vsib(instruction, index_reg, tuple_type, false);
	}
	else
		read_op_mem_16(instruction, tuple_type);
}

ICED_FORCE_INLINE void DecoderCore::read_op_mem_vsib(Instruction& instruction, Register vsib_index, TupleType tuple_type) noexcept {
	bool is_valid;
	if (state.address_size != OpSize::Size16)
		is_valid = read_op_mem_32_or_64_vsib(instruction, vsib_index, tuple_type, true);
	else {
		read_op_mem_16(instruction, tuple_type);
		is_valid = false;
	}
	if (invalid_check_mask != 0 && !is_valid)
		set_invalid_instruction();
}

} // namespace iced_x86::internal
