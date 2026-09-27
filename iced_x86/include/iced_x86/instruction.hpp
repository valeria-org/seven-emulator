// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "iced_x86/code.hpp"
#include "iced_x86/code_ext.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/condition_code.hpp"
#include "iced_x86/cpuid_feature.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/flow_control.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/internal/int_arg.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/mnemonic.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/rep_prefix_kind.hpp"
#include "iced_x86/rounding_control.hpp"
#include "iced_x86/slice.hpp"

namespace iced_x86 {

class OpCodeInfo;
class MemoryOperand;

namespace internal {
struct InstructionInternal;
class DecoderCore;
extern const std::uint8_t OP_COUNT[IcedConstants::CODE_ENUM_COUNT];
extern const MemorySize SIZES_NORMAL[IcedConstants::CODE_ENUM_COUNT];
extern const MemorySize SIZES_BCST[IcedConstants::CODE_ENUM_COUNT];
MemorySize get_mvex_memory_size(Code code, MvexRegMemConv reg_mem_conv) noexcept;
} // namespace internal

/// Contains the FPU `TOP` increment, whether it's conditional and whether the instruction writes to `TOP`
class FpuStackIncrementInfo {
public:
	/// Creates a default instance (`increment() == 0`, `conditional() == false`, `writes_top() == false`)
	constexpr FpuStackIncrementInfo() noexcept : increment_(0), conditional_(false), writes_top_(false) {}

	/// Constructor
	constexpr FpuStackIncrementInfo(std::int32_t increment, bool conditional, bool writes_top) noexcept
		: increment_(increment), conditional_(conditional), writes_top_(writes_top) {}

	/// Used if `writes_top()` is `true`:
	///
	/// Value added to `TOP`.
	///
	/// This is negative if it pushes one or more values and positive if it pops one or more values
	/// and `0` if it writes to `TOP` (eg. `FLDENV`, etc) without pushing/popping anything.
	constexpr std::int32_t increment() const noexcept { return increment_; }

	/// `true` if it's a conditional push/pop (eg. `FPTAN` or `FSINCOS`)
	constexpr bool conditional() const noexcept { return conditional_; }

	/// `true` if `TOP` is written (it's a conditional/unconditional push/pop, `FNSAVE`, `FLDENV`, etc)
	constexpr bool writes_top() const noexcept { return writes_top_; }

	/// Compares all fields
	constexpr bool operator==(const FpuStackIncrementInfo& other) const noexcept {
		return increment_ == other.increment_ && conditional_ == other.conditional_ && writes_top_ == other.writes_top_;
	}
	/// Compares all fields
	constexpr bool operator!=(const FpuStackIncrementInfo& other) const noexcept { return !(*this == other); }

private:
	std::int32_t increment_;
	bool conditional_;
	bool writes_top_;
};

/// A 16/32/64-bit x86 instruction. Created by `Decoder`, by `code_asm::CodeAssembler` or by `Instruction::with*()` methods.
///
/// It has the same size (40 bytes) and layout as the Rust `Instruction`. It's trivially copyable.
///
/// `operator==` ignores some fields (the IP, the length and the code size), use `eq_all_bits()` to compare all bits.
///
/// Methods that are implemented by other components (they're only linked if they're used):
/// - instruction info: `stack_pointer_increment()`, `fpu_stack_increment_info()`, `rflags_read()`, `rflags_written()`,
///   `rflags_cleared()`, `rflags_set()`, `rflags_undefined()`, `rflags_modified()`, and the `code_ext` functions used by
///   `encoding()`, `cpuid_features()`, `flow_control()`, `is_privileged()`, `is_stack_instruction()`, `is_save_restore_instruction()`
/// - encoder: `op_code()` (`code_ext::op_code()`) and the `with*()` factory methods (`Instruction::with()`, `Instruction::with1()`, ...)
/// - formatters: `to_string(const Instruction&)`
class Instruction {
	friend struct internal::InstructionInternal;
	// The inline decoder code in decoder.hpp can't use InstructionInternal
	friend class internal::DecoderCore;

public:
	/// All op kinds of an instruction (`op_count()` values), see `Instruction::op_kinds()`
	class OpKinds {
	public:
		/// Iterator type
		using const_iterator = const OpKind*;
		/// Gets the first element
		const OpKind* begin() const noexcept { return kinds_; }
		/// Gets the end iterator
		const OpKind* end() const noexcept { return kinds_ + count_; }
		/// Gets the number of op kinds (same as `Instruction::op_count()`)
		std::size_t size() const noexcept { return count_; }
		/// `true` if there are no operands
		bool empty() const noexcept { return count_ == 0; }
		/// Gets an op kind, `index` must be less than `size()`
		OpKind operator[](std::size_t index) const noexcept { return kinds_[index]; }

	private:
		friend class Instruction;
		OpKind kinds_[IcedConstants::MAX_OP_COUNT];
		std::uint8_t count_;
	};

	/// Function that returns the value of a register or the base address of a segment register, or `std::nullopt` for unsupported registers.
	/// See `virtual_address()`.
	///
	/// # Arguments
	///
	/// * `context`: The user context passed to `virtual_address()`
	/// * `register_`: Register (GPR8, GPR16, GPR32, GPR64, XMM, YMM, ZMM, seg). If it's a segment register, the call-back function should return the segment's base address, not the segment's register value.
	/// * `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * `element_size`: Only used if it's a vsib memory operand. Size in bytes of elements in vector index register (4 or 8).
	using GetRegisterValueFn = std::optional<std::uint64_t> (*)(void* context, Register register_, std::size_t element_index, std::size_t element_size);

	/// Creates an empty `Instruction` (all fields are cleared). See also the `with_*()` constructor methods.
	constexpr Instruction() noexcept = default;

	/// Checks if two instructions are equal, comparing all bits, not ignoring anything. `==` ignores some fields.
	bool eq_all_bits(const Instruction& other) const noexcept { return std::memcmp(this, &other, sizeof(Instruction)) == 0; }

	/// Gets the 16-bit IP of the instruction, see also `next_ip16()`
	constexpr std::uint16_t ip16() const noexcept { return static_cast<std::uint16_t>(static_cast<std::uint16_t>(next_rip_) - static_cast<std::uint16_t>(len_)); }

	/// Sets the 16-bit IP of the instruction, see also `set_next_ip16()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_ip16(std::uint16_t new_value) noexcept { next_rip_ = static_cast<std::uint64_t>(new_value) + len_; }

	/// Gets the 32-bit IP of the instruction, see also `next_ip32()`
	constexpr std::uint32_t ip32() const noexcept { return static_cast<std::uint32_t>(next_rip_) - static_cast<std::uint32_t>(len_); }

	/// Sets the 32-bit IP of the instruction, see also `set_next_ip32()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_ip32(std::uint32_t new_value) noexcept { next_rip_ = static_cast<std::uint64_t>(new_value) + len_; }

	/// Gets the 64-bit IP of the instruction, see also `next_ip()`
	constexpr std::uint64_t ip() const noexcept { return next_rip_ - len_; }

	/// Sets the 64-bit IP of the instruction, see also `set_next_ip()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_ip(std::uint64_t new_value) noexcept { next_rip_ = new_value + len_; }

	/// Gets the 16-bit IP of the next instruction, see also `ip16()`
	constexpr std::uint16_t next_ip16() const noexcept { return static_cast<std::uint16_t>(next_rip_); }

	/// Sets the 16-bit IP of the next instruction, see also `set_ip16()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_next_ip16(std::uint16_t new_value) noexcept { next_rip_ = new_value; }

	/// Gets the 32-bit IP of the next instruction, see also `ip32()`
	constexpr std::uint32_t next_ip32() const noexcept { return static_cast<std::uint32_t>(next_rip_); }

	/// Sets the 32-bit IP of the next instruction, see also `set_ip32()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_next_ip32(std::uint32_t new_value) noexcept { next_rip_ = new_value; }

	/// Gets the 64-bit IP of the next instruction, see also `ip()`
	constexpr std::uint64_t next_ip() const noexcept { return next_rip_; }

	/// Sets the 64-bit IP of the next instruction, see also `set_ip()`
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_next_ip(std::uint64_t new_value) noexcept { next_rip_ = new_value; }

	/// Gets the code size when the instruction was decoded. This value is informational and can
	/// be used by a formatter.
	constexpr CodeSize code_size() const noexcept { return static_cast<CodeSize>((flags1_ >> F1_CODE_SIZE_SHIFT) & F1_CODE_SIZE_MASK); }

	/// Sets the code size when the instruction was decoded. This value is informational and can
	/// be used by a formatter.
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_code_size(CodeSize new_value) noexcept {
		flags1_ = (flags1_ & ~(F1_CODE_SIZE_MASK << F1_CODE_SIZE_SHIFT)) | ((static_cast<std::uint32_t>(new_value) & F1_CODE_SIZE_MASK) << F1_CODE_SIZE_SHIFT);
	}

	/// Checks if it's an invalid instruction (`code()` == `Code::INVALID`)
	constexpr bool is_invalid() const noexcept { return code_ == Code::INVALID; }

	/// Gets the instruction code, see also `mnemonic()`
	constexpr Code code() const noexcept { return code_; }

	/// Sets the instruction code
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_code(Code new_value) noexcept { code_ = new_value; }

	/// Gets the mnemonic, see also `code()`
	Mnemonic mnemonic() const noexcept { return code_ext::mnemonic(code_); }

	/// Gets the operand count. An instruction can have 0-5 operands.
	std::uint32_t op_count() const noexcept { return internal::OP_COUNT[static_cast<std::size_t>(code_)]; }

	/// Gets the length of the instruction, 0-15 bytes. This is just informational. If you modify the instruction
	/// or create a new one, this method could return the wrong value.
	constexpr std::size_t len() const noexcept { return len_; }

	/// Sets the length of the instruction, 0-15 bytes. This is just informational. If you modify the instruction
	/// or create a new one, this method could return the wrong value.
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_len(std::size_t new_value) noexcept { len_ = static_cast<std::uint8_t>(new_value); }

	/// `true` if the instruction has the `XACQUIRE` prefix (`F2`)
	bool has_xacquire_prefix() const noexcept { return (flags1_ & F1_REPNE_PREFIX) != 0 && is_xacquire_instr(); }

	/// `true` if the instruction has the `XACQUIRE` prefix (`F2`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_xacquire_prefix(bool new_value) noexcept { set_flags1_bit(F1_REPNE_PREFIX, new_value); }

	/// `true` if the instruction has the `XRELEASE` prefix (`F3`)
	bool has_xrelease_prefix() const noexcept { return (flags1_ & F1_REPE_PREFIX) != 0 && is_xrelease_instr(); }

	/// `true` if the instruction has the `XRELEASE` prefix (`F3`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_xrelease_prefix(bool new_value) noexcept { set_flags1_bit(F1_REPE_PREFIX, new_value); }

	/// `true` if the instruction has the `REPE` or `REP` prefix (`F3`)
	constexpr bool has_rep_prefix() const noexcept { return (flags1_ & F1_REPE_PREFIX) != 0; }

	/// `true` if the instruction has the `REPE` or `REP` prefix (`F3`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_rep_prefix(bool new_value) noexcept { set_flags1_bit(F1_REPE_PREFIX, new_value); }

	/// `true` if the instruction has the `REPE` or `REP` prefix (`F3`)
	constexpr bool has_repe_prefix() const noexcept { return (flags1_ & F1_REPE_PREFIX) != 0; }

	/// `true` if the instruction has the `REPE` or `REP` prefix (`F3`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_repe_prefix(bool new_value) noexcept { set_flags1_bit(F1_REPE_PREFIX, new_value); }

	/// `true` if the instruction has the `REPNE` prefix (`F2`)
	constexpr bool has_repne_prefix() const noexcept { return (flags1_ & F1_REPNE_PREFIX) != 0; }

	/// `true` if the instruction has the `REPNE` prefix (`F2`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_repne_prefix(bool new_value) noexcept { set_flags1_bit(F1_REPNE_PREFIX, new_value); }

	/// `true` if the instruction has the `LOCK` prefix (`F0`)
	constexpr bool has_lock_prefix() const noexcept { return (flags1_ & F1_LOCK_PREFIX) != 0; }

	/// `true` if the instruction has the `LOCK` prefix (`F0`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_has_lock_prefix(bool new_value) noexcept { set_flags1_bit(F1_LOCK_PREFIX, new_value); }

	/// Gets operand #0's kind if the operand exists (see `op_count()` and `try_op_kind()`)
	constexpr OpKind op0_kind() const noexcept { return op_kinds_[0]; }

	/// Sets operand #0's kind if the operand exists (see `op_count()` and `try_set_op_kind()`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_op0_kind(OpKind new_value) noexcept { op_kinds_[0] = new_value; }

	/// Gets operand #1's kind if the operand exists (see `op_count()` and `try_op_kind()`)
	constexpr OpKind op1_kind() const noexcept { return op_kinds_[1]; }

	/// Sets operand #1's kind if the operand exists (see `op_count()` and `try_set_op_kind()`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_op1_kind(OpKind new_value) noexcept { op_kinds_[1] = new_value; }

	/// Gets operand #2's kind if the operand exists (see `op_count()` and `try_op_kind()`)
	constexpr OpKind op2_kind() const noexcept { return op_kinds_[2]; }

	/// Sets operand #2's kind if the operand exists (see `op_count()` and `try_set_op_kind()`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_op2_kind(OpKind new_value) noexcept { op_kinds_[2] = new_value; }

	/// Gets operand #3's kind if the operand exists (see `op_count()` and `try_op_kind()`)
	constexpr OpKind op3_kind() const noexcept { return op_kinds_[3]; }

	/// Sets operand #3's kind if the operand exists (see `op_count()` and `try_set_op_kind()`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_op3_kind(OpKind new_value) noexcept { op_kinds_[3] = new_value; }

	/// Gets operand #4's kind if the operand exists (see `op_count()` and `try_op_kind()`)
	constexpr OpKind op4_kind() const noexcept { return OpKind::Immediate8; }

	/// Sets operand #4's kind if the operand exists (see `op_count()` and `try_set_op_kind()`)
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_op4_kind(OpKind new_value) noexcept {
		(void)new_value;
		assert(new_value == OpKind::Immediate8);
	}

	/// Sets operand #4's kind. Returns an error if `new_value` isn't `OpKind::Immediate8` (the only valid value).
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	Result<void> try_set_op4_kind(OpKind new_value) noexcept;

	/// Gets all op kinds (`op_count()` values)
	OpKinds op_kinds() const noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		OpKinds result;
		result.kinds_[0] = op_kinds_[0];
		result.kinds_[1] = op_kinds_[1];
		result.kinds_[2] = op_kinds_[2];
		result.kinds_[3] = op_kinds_[3];
		result.kinds_[4] = op4_kind();
		result.count_ = static_cast<std::uint8_t>(op_count());
		return result;
	}

	/// Gets an operand's kind if it exists (see `op_count()`)
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	OpKind op_kind(std::uint32_t operand) const noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand < 4)
			return op_kinds_[operand];
		if (operand == 4)
			return op4_kind();
		assert(false && "Invalid operand");
		return OpKind::Register;
	}

	/// Gets an operand's kind if it exists (see `op_count()`). Returns an error if `operand` is invalid.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Result<OpKind> try_op_kind(std::uint32_t operand) const noexcept;

	/// Sets an operand's kind
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `op_kind`: Operand kind
	void set_op_kind(std::uint32_t operand, OpKind op_kind) noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand < 4)
			op_kinds_[operand] = op_kind;
		else if (operand == 4)
			set_op4_kind(op_kind);
		else
			assert(false && "Invalid operand");
	}

	/// Sets an operand's kind. Returns an error if `operand` or `op_kind` is invalid.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `op_kind`: Operand kind
	Result<void> try_set_op_kind(std::uint32_t operand, OpKind op_kind) noexcept;

	/// Checks if the instruction has a segment override prefix, see `segment_prefix()`
	constexpr bool has_segment_prefix() const noexcept { return (((flags1_ >> F1_SEGMENT_PREFIX_SHIFT) & F1_SEGMENT_PREFIX_MASK) - 1) < 6; }

	/// Gets the segment override prefix or `Register::None` if none. See also `memory_segment()`.
	/// Use this method if the operand has kind `OpKind::Memory`,
	/// `OpKind::MemorySegSI`, `OpKind::MemorySegESI`, `OpKind::MemorySegRSI`
	Register segment_prefix() const noexcept {
		const std::uint32_t index = ((flags1_ >> F1_SEGMENT_PREFIX_SHIFT) & F1_SEGMENT_PREFIX_MASK) - 1;
		static_assert(static_cast<std::uint32_t>(Register::ES) + 1 == static_cast<std::uint32_t>(Register::CS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 2 == static_cast<std::uint32_t>(Register::SS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 3 == static_cast<std::uint32_t>(Register::DS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 4 == static_cast<std::uint32_t>(Register::FS), "");
		static_assert(static_cast<std::uint32_t>(Register::ES) + 5 == static_cast<std::uint32_t>(Register::GS), "");
		if (index < 6)
			return static_cast<Register>(static_cast<std::uint32_t>(Register::ES) + index);
		return Register::None;
	}

	/// Sets the segment override prefix or `Register::None` if none. See also `memory_segment()`.
	/// Use this method if the operand has kind `OpKind::Memory`,
	/// `OpKind::MemorySegSI`, `OpKind::MemorySegESI`, `OpKind::MemorySegRSI`
	///
	/// # Arguments
	///
	/// * `new_value`: Segment register prefix
	void set_segment_prefix(Register new_value) noexcept {
		assert(new_value == Register::None || (Register::ES <= new_value && new_value <= Register::GS));
		const std::uint32_t enc_value = new_value == Register::None
			? 0
			: ((static_cast<std::uint32_t>(new_value) - static_cast<std::uint32_t>(Register::ES)) + 1) & F1_SEGMENT_PREFIX_MASK;
		flags1_ = (flags1_ & ~(F1_SEGMENT_PREFIX_MASK << F1_SEGMENT_PREFIX_SHIFT)) | (enc_value << F1_SEGMENT_PREFIX_SHIFT);
	}

	/// Gets the effective segment register used to reference the memory location.
	/// Use this method if the operand has kind `OpKind::Memory`,
	/// `OpKind::MemorySegSI`, `OpKind::MemorySegESI`, `OpKind::MemorySegRSI`
	Register memory_segment() const noexcept {
		const Register seg_reg = segment_prefix();
		if (seg_reg != Register::None)
			return seg_reg;
		switch (mem_base_reg_) {
		case Register::BP:
		case Register::EBP:
		case Register::ESP:
		case Register::RBP:
		case Register::RSP:
			return Register::SS;
		default:
			return Register::DS;
		}
	}

	/// Gets the size of the memory displacement in bytes. Valid values are `0`, `1` (16/32/64-bit), `2` (16-bit), `4` (32-bit), `8` (64-bit).
	/// Note that the return value can be 1 and `memory_displacement64()` may still not fit in
	/// a signed byte if it's an EVEX/MVEX encoded instruction.
	/// Use this method if the operand has kind `OpKind::Memory`
	constexpr std::uint32_t memory_displ_size() const noexcept {
		const std::uint32_t size = displ_size_;
		if (size <= 2)
			return size;
		if (size == 3)
			return 4;
		return 8;
	}

	/// Sets the size of the memory displacement in bytes. Valid values are `0`, `1` (16/32/64-bit), `2` (16-bit), `4` (32-bit), `8` (64-bit).
	/// Note that the return value can be 1 and `memory_displacement64()` may still not fit in
	/// a signed byte if it's an EVEX/MVEX encoded instruction.
	/// Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: Displacement size
	void set_memory_displ_size(std::uint32_t new_value) noexcept {
		switch (new_value) {
		case 0: displ_size_ = 0; break;
		case 1: displ_size_ = 1; break;
		case 2: displ_size_ = 2; break;
		case 4: displ_size_ = 3; break;
		default: displ_size_ = 4; break;
		}
	}

	/// `true` if the data is broadcast (EVEX instructions only)
	constexpr bool is_broadcast() const noexcept { return (flags1_ & F1_BROADCAST) != 0; }

	/// Sets the is broadcast flag (EVEX instructions only)
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_is_broadcast(bool new_value) noexcept { set_flags1_bit(F1_BROADCAST, new_value); }

	/// `true` if eviction hint bit is set (`{eh}`) (MVEX instructions only)
	constexpr bool is_mvex_eviction_hint() const noexcept { return IcedConstants::is_mvex(code_) && (immediate_ & MVEX_EVICTION_HINT) != 0; }

	/// `true` if eviction hint bit is set (`{eh}`) (MVEX instructions only)
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_is_mvex_eviction_hint(bool new_value) noexcept {
		if (new_value)
			immediate_ |= MVEX_EVICTION_HINT;
		else
			immediate_ &= ~MVEX_EVICTION_HINT;
	}

	/// (MVEX) Register/memory operand conversion function
	MvexRegMemConv mvex_reg_mem_conv() const noexcept {
		if (!IcedConstants::is_mvex(code_))
			return MvexRegMemConv::None;
		const std::uint32_t value = (immediate_ >> MVEX_REG_MEM_CONV_SHIFT) & MVEX_REG_MEM_CONV_MASK;
		if (value < IcedConstants::MVEX_REG_MEM_CONV_ENUM_COUNT)
			return static_cast<MvexRegMemConv>(value);
		return MvexRegMemConv::None;
	}

	/// (MVEX) Register/memory operand conversion function
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_mvex_reg_mem_conv(MvexRegMemConv new_value) noexcept {
		immediate_ = (immediate_ & ~(MVEX_REG_MEM_CONV_MASK << MVEX_REG_MEM_CONV_SHIFT)) | (static_cast<std::uint32_t>(new_value) << MVEX_REG_MEM_CONV_SHIFT);
	}

	/// Gets the size of the memory location that is referenced by the operand. See also `is_broadcast()`.
	/// Use this method if the operand has kind `OpKind::Memory`,
	/// `OpKind::MemorySegSI`, `OpKind::MemorySegESI`, `OpKind::MemorySegRSI`,
	/// `OpKind::MemoryESDI`, `OpKind::MemoryESEDI`, `OpKind::MemoryESRDI`
	MemorySize memory_size() const noexcept {
		const Code code = code_;
		if (IcedConstants::is_mvex(code))
			return internal::get_mvex_memory_size(code, mvex_reg_mem_conv());
		if (!is_broadcast())
			return internal::SIZES_NORMAL[static_cast<std::size_t>(code)];
		return internal::SIZES_BCST[static_cast<std::size_t>(code)];
	}

	/// Gets the index register scale value, valid values are `*1`, `*2`, `*4`, `*8`. Use this method if the operand has kind `OpKind::Memory`
	constexpr std::uint32_t memory_index_scale() const noexcept { return 1U << scale_; }

	/// Sets the index register scale value, valid values are `*1`, `*2`, `*4`, `*8`. Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: New value (1, 2, 4 or 8)
	void set_memory_index_scale(std::uint32_t new_value) noexcept {
		switch (new_value) {
		case 1: scale_ = 0; break;
		case 2: scale_ = 1; break;
		case 4: scale_ = 2; break;
		default:
			assert(new_value == 8);
			scale_ = 3;
			break;
		}
	}

	/// Gets the memory operand's displacement or the 32-bit absolute address if it's
	/// an `EIP` or `RIP` relative memory operand.
	/// Use this method if the operand has kind `OpKind::Memory`
	constexpr std::uint32_t memory_displacement32() const noexcept { return static_cast<std::uint32_t>(mem_displ_); }

	/// Gets the memory operand's displacement or the 32-bit absolute address if it's
	/// an `EIP` or `RIP` relative memory operand.
	/// Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_memory_displacement32(std::uint32_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the memory operand's displacement or the 64-bit absolute address if it's
	/// an `EIP` or `RIP` relative memory operand.
	/// Use this method if the operand has kind `OpKind::Memory`
	constexpr std::uint64_t memory_displacement64() const noexcept { return mem_displ_; }

	/// Gets the memory operand's displacement or the 64-bit absolute address if it's
	/// an `EIP` or `RIP` relative memory operand.
	/// Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_memory_displacement64(std::uint64_t new_value) noexcept { mem_displ_ = new_value; }

	/// Tries to get an operand's immediate value.
	/// Can only be called if the operand has kind `OpKind::Immediate8`,
	/// `OpKind::Immediate8_2nd`, `OpKind::Immediate16`, `OpKind::Immediate32`,
	/// `OpKind::Immediate64`, `OpKind::Immediate8to16`, `OpKind::Immediate8to32`,
	/// `OpKind::Immediate8to64`, `OpKind::Immediate32to64`
	///
	/// # Errors
	///
	/// - Fails if the operand is not one of those listed above
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Result<std::uint64_t> try_immediate(std::uint32_t operand) const noexcept;

	/// Gets an operand's immediate value
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	std::uint64_t immediate(std::uint32_t operand) const noexcept {
		std::uint64_t value;
		if (get_immediate_core(operand, value))
			return value;
		assert(false && "Invalid operand");
		return 0;
	}

	/// Sets an operand's immediate value
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	void set_immediate_i32(std::uint32_t operand, std::int32_t new_value) noexcept { set_immediate_u64(operand, static_cast<std::uint64_t>(static_cast<std::int64_t>(new_value))); }

	/// Sets an operand's immediate value. Returns an error if it's not an immediate operand.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	Result<void> try_set_immediate_i32(std::uint32_t operand, std::int32_t new_value) noexcept {
		return try_set_immediate_u64(operand, static_cast<std::uint64_t>(static_cast<std::int64_t>(new_value)));
	}

	/// Sets an operand's immediate value
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	void set_immediate_u32(std::uint32_t operand, std::uint32_t new_value) noexcept { set_immediate_u64(operand, new_value); }

	/// Sets an operand's immediate value. Returns an error if it's not an immediate operand.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	Result<void> try_set_immediate_u32(std::uint32_t operand, std::uint32_t new_value) noexcept { return try_set_immediate_u64(operand, new_value); }

	/// Sets an operand's immediate value
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	void set_immediate_i64(std::uint32_t operand, std::int64_t new_value) noexcept { set_immediate_u64(operand, static_cast<std::uint64_t>(new_value)); }

	/// Sets an operand's immediate value. Returns an error if it's not an immediate operand.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	Result<void> try_set_immediate_i64(std::uint32_t operand, std::int64_t new_value) noexcept {
		return try_set_immediate_u64(operand, static_cast<std::uint64_t>(new_value));
	}

	/// Sets an operand's immediate value
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	void set_immediate_u64(std::uint32_t operand, std::uint64_t new_value) noexcept {
		const bool ok = set_immediate_core(operand, new_value);
		(void)ok;
		assert(ok && "Invalid operand");
	}

	/// Sets an operand's immediate value. Returns an error if it's not an immediate operand.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: Immediate
	Result<void> try_set_immediate_u64(std::uint32_t operand, std::uint64_t new_value) noexcept;

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8`
	constexpr std::uint8_t immediate8() const noexcept { return static_cast<std::uint8_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate8(std::uint8_t new_value) noexcept {
		// The upper bits are used by MVEX instructions (MvexInstrFlags)
		immediate_ = (immediate_ & 0xFFFF'FF00U) | new_value;
	}

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8_2nd`
	constexpr std::uint8_t immediate8_2nd() const noexcept { return static_cast<std::uint8_t>(mem_displ_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8_2nd`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate8_2nd(std::uint8_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate16`
	constexpr std::uint16_t immediate16() const noexcept { return static_cast<std::uint16_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate16`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate16(std::uint16_t new_value) noexcept { immediate_ = new_value; }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate32`
	constexpr std::uint32_t immediate32() const noexcept { return immediate_; }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate32`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate32(std::uint32_t new_value) noexcept { immediate_ = new_value; }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate64`
	constexpr std::uint64_t immediate64() const noexcept { return (mem_displ_ << 32) | immediate_; }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate64`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate64(std::uint64_t new_value) noexcept {
		immediate_ = static_cast<std::uint32_t>(new_value);
		mem_displ_ = new_value >> 32;
	}

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to16`
	constexpr std::int16_t immediate8to16() const noexcept { return static_cast<std::int8_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to16`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate8to16(std::int16_t new_value) noexcept { immediate_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(new_value))); }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to32`
	constexpr std::int32_t immediate8to32() const noexcept { return static_cast<std::int8_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to32`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate8to32(std::int32_t new_value) noexcept { immediate_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(new_value))); }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to64`
	constexpr std::int64_t immediate8to64() const noexcept { return static_cast<std::int8_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate8to64`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate8to64(std::int64_t new_value) noexcept { immediate_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(new_value))); }

	/// Gets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate32to64`
	constexpr std::int64_t immediate32to64() const noexcept { return static_cast<std::int32_t>(immediate_); }

	/// Sets the operand's immediate value. Use this method if the operand has kind `OpKind::Immediate32to64`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_immediate32to64(std::int64_t new_value) noexcept { immediate_ = static_cast<std::uint32_t>(new_value); }

	/// Gets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch16`
	constexpr std::uint16_t near_branch16() const noexcept { return static_cast<std::uint16_t>(mem_displ_); }

	/// Sets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch16`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_near_branch16(std::uint16_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch32`
	constexpr std::uint32_t near_branch32() const noexcept { return static_cast<std::uint32_t>(mem_displ_); }

	/// Sets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch32`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_near_branch32(std::uint32_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch64`
	constexpr std::uint64_t near_branch64() const noexcept { return mem_displ_; }

	/// Sets the operand's branch target. Use this method if the operand has kind `OpKind::NearBranch64`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_near_branch64(std::uint64_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the near branch target if it's a `CALL`/`JMP`/`Jcc` near branch instruction
	/// (i.e., if `op0_kind()` is `OpKind::NearBranch16`, `OpKind::NearBranch32` or `OpKind::NearBranch64`)
	std::uint64_t near_branch_target() const noexcept {
		OpKind op_kind = op0_kind();
		// Check if JKZD/JKNZD
		if (op_count() == 2)
			op_kind = op1_kind();
		switch (op_kind) {
		case OpKind::NearBranch16:
			return near_branch16();
		case OpKind::NearBranch32:
			return near_branch32();
		case OpKind::NearBranch64:
			return near_branch64();
		default:
			return 0;
		}
	}

	/// Gets the operand's branch target. Use this method if the operand has kind `OpKind::FarBranch16`
	constexpr std::uint16_t far_branch16() const noexcept { return static_cast<std::uint16_t>(immediate_); }

	/// Sets the operand's branch target. Use this method if the operand has kind `OpKind::FarBranch16`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_far_branch16(std::uint16_t new_value) noexcept { immediate_ = new_value; }

	/// Gets the operand's branch target. Use this method if the operand has kind `OpKind::FarBranch32`
	constexpr std::uint32_t far_branch32() const noexcept { return immediate_; }

	/// Sets the operand's branch target. Use this method if the operand has kind `OpKind::FarBranch32`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_far_branch32(std::uint32_t new_value) noexcept { immediate_ = new_value; }

	/// Gets the operand's branch target selector. Use this method if the operand has kind `OpKind::FarBranch16` or `OpKind::FarBranch32`
	constexpr std::uint16_t far_branch_selector() const noexcept { return static_cast<std::uint16_t>(mem_displ_); }

	/// Sets the operand's branch target selector. Use this method if the operand has kind `OpKind::FarBranch16` or `OpKind::FarBranch32`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_far_branch_selector(std::uint16_t new_value) noexcept { mem_displ_ = new_value; }

	/// Gets the memory operand's base register or `Register::None` if none. Use this method if the operand has kind `OpKind::Memory`
	constexpr Register memory_base() const noexcept { return mem_base_reg_; }

	/// Sets the memory operand's base register or `Register::None` if none. Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_memory_base(Register new_value) noexcept { mem_base_reg_ = new_value; }

	/// Gets the memory operand's index register or `Register::None` if none. Use this method if the operand has kind `OpKind::Memory`
	constexpr Register memory_index() const noexcept { return mem_index_reg_; }

	/// Sets the memory operand's index register or `Register::None` if none. Use this method if the operand has kind `OpKind::Memory`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_memory_index(Register new_value) noexcept { mem_index_reg_ = new_value; }

	/// Gets operand #0's register value. Use this method if operand #0 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	constexpr Register op0_register() const noexcept { return regs_[0]; }

	/// Sets operand #0's register value. Use this method if operand #0 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op0_register(Register new_value) noexcept { regs_[0] = new_value; }

	/// Gets operand #1's register value. Use this method if operand #1 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	constexpr Register op1_register() const noexcept { return regs_[1]; }

	/// Sets operand #1's register value. Use this method if operand #1 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op1_register(Register new_value) noexcept { regs_[1] = new_value; }

	/// Gets operand #2's register value. Use this method if operand #2 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	constexpr Register op2_register() const noexcept { return regs_[2]; }

	/// Sets operand #2's register value. Use this method if operand #2 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op2_register(Register new_value) noexcept { regs_[2] = new_value; }

	/// Gets operand #3's register value. Use this method if operand #3 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	constexpr Register op3_register() const noexcept { return regs_[3]; }

	/// Sets operand #3's register value. Use this method if operand #3 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op3_register(Register new_value) noexcept { regs_[3] = new_value; }

	/// Gets operand #4's register value. Use this method if operand #4 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	constexpr Register op4_register() const noexcept { return Register::None; }

	/// Sets operand #4's register value. Use this method if operand #4 (`op0_kind()`) has kind `OpKind::Register`, see `op_count()` and `try_op_register()`
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op4_register(Register new_value) noexcept {
		(void)new_value;
		assert(new_value == Register::None);
	}

	/// Sets operand #4's register value. Returns an error if `new_value` isn't `Register::None` (the only valid value).
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	Result<void> try_set_op4_register(Register new_value) noexcept;

	/// Gets the operand's register value. Use this method if the operand has kind `OpKind::Register`
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Register op_register(std::uint32_t operand) const noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand < 4)
			return regs_[operand];
		if (operand == 4)
			return op4_register();
		assert(false && "Invalid operand");
		return Register::None;
	}

	/// Gets the operand's register value. Returns an error if `operand` is invalid.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Result<Register> try_op_register(std::uint32_t operand) const noexcept;

	/// Sets the operand's register value. Use this method if the operand has kind `OpKind::Register`
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: New value
	void set_op_register(std::uint32_t operand, Register new_value) noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand < 4)
			regs_[operand] = new_value;
		else if (operand == 4)
			set_op4_register(new_value);
		else
			assert(false && "Invalid operand");
	}

	/// Sets the operand's register value. Returns an error if `operand` or `new_value` is invalid.
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	/// * `new_value`: New value
	Result<void> try_set_op_register(std::uint32_t operand, Register new_value) noexcept;

	/// Gets the opmask register (`Register::K1` - `Register::K7`) or `Register::None` if none
	Register op_mask() const noexcept {
		const std::uint32_t r = (flags1_ >> F1_OP_MASK_SHIFT) & F1_OP_MASK_MASK;
		static_assert(F1_OP_MASK_MASK == 7, "");
		static_assert(static_cast<std::uint32_t>(Register::K0) + 7 == static_cast<std::uint32_t>(Register::K7), "");
		return r == 0 ? Register::None : static_cast<Register>(r + static_cast<std::uint32_t>(Register::K0));
	}

	/// Sets the opmask register (`Register::K1` - `Register::K7`) or `Register::None` if none
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_op_mask(Register new_value) noexcept {
		assert(new_value == Register::None || (Register::K1 <= new_value && new_value <= Register::K7));
		const std::uint32_t r =
			new_value == Register::None ? 0 : (static_cast<std::uint32_t>(new_value) - static_cast<std::uint32_t>(Register::K0)) & F1_OP_MASK_MASK;
		flags1_ = (flags1_ & ~(F1_OP_MASK_MASK << F1_OP_MASK_SHIFT)) | (r << F1_OP_MASK_SHIFT);
	}

	/// Checks if there's an opmask register (`op_mask()`)
	constexpr bool has_op_mask() const noexcept { return (flags1_ & (F1_OP_MASK_MASK << F1_OP_MASK_SHIFT)) != 0; }

	/// `true` if zeroing-masking, `false` if merging-masking.
	/// Only used by most EVEX encoded instructions that use opmask registers.
	constexpr bool zeroing_masking() const noexcept { return (flags1_ & F1_ZEROING_MASKING) != 0; }

	/// `true` if zeroing-masking, `false` if merging-masking.
	/// Only used by most EVEX encoded instructions that use opmask registers.
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_zeroing_masking(bool new_value) noexcept { set_flags1_bit(F1_ZEROING_MASKING, new_value); }

	/// `true` if merging-masking, `false` if zeroing-masking.
	/// Only used by most EVEX encoded instructions that use opmask registers.
	constexpr bool merging_masking() const noexcept { return (flags1_ & F1_ZEROING_MASKING) == 0; }

	/// `true` if merging-masking, `false` if zeroing-masking.
	/// Only used by most EVEX encoded instructions that use opmask registers.
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_merging_masking(bool new_value) noexcept { set_flags1_bit(F1_ZEROING_MASKING, !new_value); }

	/// Gets the rounding control (SAE is implied but `suppress_all_exceptions()` still returns `false`)
	/// or `RoundingControl::None` if the instruction doesn't use it.
	constexpr RoundingControl rounding_control() const noexcept {
		return static_cast<RoundingControl>((flags1_ >> F1_ROUNDING_CONTROL_SHIFT) & F1_ROUNDING_CONTROL_MASK);
	}

	/// Sets the rounding control (SAE is implied but `suppress_all_exceptions()` still returns `false`)
	/// or `RoundingControl::None` if the instruction doesn't use it.
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_rounding_control(RoundingControl new_value) noexcept {
		flags1_ = (flags1_ & ~(F1_ROUNDING_CONTROL_MASK << F1_ROUNDING_CONTROL_SHIFT)) | (static_cast<std::uint32_t>(new_value) << F1_ROUNDING_CONTROL_SHIFT);
	}

	/// Gets the number of elements in a `db`/`dw`/`dd`/`dq` directive.
	/// Can only be called if `code()` is `Code::DeclareByte`, `Code::DeclareWord`, `Code::DeclareDword`, `Code::DeclareQword`
	constexpr std::size_t declare_data_len() const noexcept { return ((flags1_ >> F1_DATA_LENGTH_SHIFT) & F1_DATA_LENGTH_MASK) + 1; }

	/// Sets the number of elements in a `db`/`dw`/`dd`/`dq` directive.
	/// Can only be called if `code()` is `Code::DeclareByte`, `Code::DeclareWord`, `Code::DeclareDword`, `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `new_value`: New value: `db`: 1-16; `dw`: 1-8; `dd`: 1-4; `dq`: 1-2
	void set_declare_data_len(std::size_t new_value) noexcept {
		assert(1 <= new_value && new_value <= 0x10);
		flags1_ = (flags1_ & ~(F1_DATA_LENGTH_MASK << F1_DATA_LENGTH_SHIFT)) |
			(((static_cast<std::uint32_t>(new_value) - 1) & F1_DATA_LENGTH_MASK) << F1_DATA_LENGTH_SHIFT);
	}

	/// Sets a new `db` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	/// * `new_value`: New value
	void set_declare_byte_value_i8(std::size_t index, std::int8_t new_value) noexcept { set_declare_byte_value(index, static_cast<std::uint8_t>(new_value)); }

	/// Sets a new `db` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Errors
	///
	/// - Fails if `index` is invalid
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	/// * `new_value`: New value
	Result<void> try_set_declare_byte_value_i8(std::size_t index, std::int8_t new_value) noexcept {
		return try_set_declare_byte_value(index, static_cast<std::uint8_t>(new_value));
	}

	/// Sets a new `db` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	/// * `new_value`: New value
	void set_declare_byte_value(std::size_t index, std::uint8_t new_value) noexcept;

	/// Sets a new `db` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	/// * `new_value`: New value
	Result<void> try_set_declare_byte_value(std::size_t index, std::uint8_t new_value) noexcept;

	/// Gets a `db` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	std::uint8_t get_declare_byte_value(std::size_t index) const noexcept;

	/// Gets a `db` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareByte`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-15)
	Result<std::uint8_t> try_get_declare_byte_value(std::size_t index) const noexcept;

	/// Sets a new `dw` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	/// * `new_value`: New value
	void set_declare_word_value_i16(std::size_t index, std::int16_t new_value) noexcept { set_declare_word_value(index, static_cast<std::uint16_t>(new_value)); }

	/// Sets a new `dw` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	/// * `new_value`: New value
	Result<void> try_set_declare_word_value_i16(std::size_t index, std::int16_t new_value) noexcept {
		return try_set_declare_word_value(index, static_cast<std::uint16_t>(new_value));
	}

	/// Sets a new `dw` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	/// * `new_value`: New value
	void set_declare_word_value(std::size_t index, std::uint16_t new_value) noexcept;

	/// Sets a new `dw` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	/// * `new_value`: New value
	Result<void> try_set_declare_word_value(std::size_t index, std::uint16_t new_value) noexcept;

	/// Gets a `dw` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	std::uint16_t get_declare_word_value(std::size_t index) const noexcept;

	/// Gets a `dw` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareWord`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-7)
	Result<std::uint16_t> try_get_declare_word_value(std::size_t index) const noexcept;

	/// Sets a new `dd` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	/// * `new_value`: New value
	void set_declare_dword_value_i32(std::size_t index, std::int32_t new_value) noexcept { set_declare_dword_value(index, static_cast<std::uint32_t>(new_value)); }

	/// Sets a new `dd` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	/// * `new_value`: New value
	Result<void> try_set_declare_dword_value_i32(std::size_t index, std::int32_t new_value) noexcept {
		return try_set_declare_dword_value(index, static_cast<std::uint32_t>(new_value));
	}

	/// Sets a new `dd` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	/// * `new_value`: New value
	void set_declare_dword_value(std::size_t index, std::uint32_t new_value) noexcept;

	/// Sets a new `dd` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	/// * `new_value`: New value
	Result<void> try_set_declare_dword_value(std::size_t index, std::uint32_t new_value) noexcept;

	/// Gets a `dd` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	std::uint32_t get_declare_dword_value(std::size_t index) const noexcept;

	/// Gets a `dd` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareDword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-3)
	Result<std::uint32_t> try_get_declare_dword_value(std::size_t index) const noexcept;

	/// Sets a new `dq` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	/// * `new_value`: New value
	void set_declare_qword_value_i64(std::size_t index, std::int64_t new_value) noexcept { set_declare_qword_value(index, static_cast<std::uint64_t>(new_value)); }

	/// Sets a new `dq` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	/// * `new_value`: New value
	Result<void> try_set_declare_qword_value_i64(std::size_t index, std::int64_t new_value) noexcept {
		return try_set_declare_qword_value(index, static_cast<std::uint64_t>(new_value));
	}

	/// Sets a new `dq` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	/// * `new_value`: New value
	void set_declare_qword_value(std::size_t index, std::uint64_t new_value) noexcept;

	/// Sets a new `dq` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	/// * `new_value`: New value
	Result<void> try_set_declare_qword_value(std::size_t index, std::uint64_t new_value) noexcept;

	/// Gets a `dq` value, see also `declare_data_len()`.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	std::uint64_t get_declare_qword_value(std::size_t index) const noexcept;

	/// Gets a `dq` value, see also `declare_data_len()`. Returns an error if `index` is invalid.
	/// Can only be called if `code()` is `Code::DeclareQword`
	///
	/// # Arguments
	///
	/// * `index`: Index (0-1)
	Result<std::uint64_t> try_get_declare_qword_value(std::size_t index) const noexcept;

	/// Checks if this is a VSIB instruction, see also `is_vsib32()`, `is_vsib64()`
	bool is_vsib() const noexcept { return vsib().has_value(); }

	/// VSIB instructions only (`is_vsib()`): `true` if it's using 32-bit indexes, `false` if it's using 64-bit indexes
	bool is_vsib32() const noexcept {
		const auto is_vsib64 = vsib();
		return is_vsib64.has_value() && !*is_vsib64;
	}

	/// VSIB instructions only (`is_vsib()`): `true` if it's using 64-bit indexes, `false` if it's using 32-bit indexes
	bool is_vsib64() const noexcept {
		const auto is_vsib64 = vsib();
		return is_vsib64.has_value() && *is_vsib64;
	}

	/// Checks if it's a vsib instruction.
	///
	/// # Returns
	///
	/// * `true` if it's a VSIB instruction with 64-bit indexes
	/// * `false` if it's a VSIB instruction with 32-bit indexes
	/// * `std::nullopt` if it's not a VSIB instruction.
	std::optional<bool> vsib() const noexcept;

	/// Gets the suppress all exceptions flag (EVEX/MVEX encoded instructions). Note that if `rounding_control()` is
	/// not `RoundingControl::None`, SAE is implied but this method will still return `false`.
	constexpr bool suppress_all_exceptions() const noexcept { return (flags1_ & F1_SUPPRESS_ALL_EXCEPTIONS) != 0; }

	/// Sets the suppress all exceptions flag (EVEX/MVEX encoded instructions). Note that if `rounding_control()` is
	/// not `RoundingControl::None`, SAE is implied but this method will still return `false`.
	///
	/// # Arguments
	///
	/// * `new_value`: New value
	void set_suppress_all_exceptions(bool new_value) noexcept { set_flags1_bit(F1_SUPPRESS_ALL_EXCEPTIONS, new_value); }

	/// Checks if the memory operand is `RIP`/`EIP` relative
	constexpr bool is_ip_rel_memory_operand() const noexcept { return mem_base_reg_ == Register::RIP || mem_base_reg_ == Register::EIP; }

	/// Gets the `RIP`/`EIP` releative address (`memory_displacement32()` or `memory_displacement64()`).
	/// This method is only valid if there's a memory operand with `RIP`/`EIP` relative addressing, see `is_ip_rel_memory_operand()`
	constexpr std::uint64_t ip_rel_memory_address() const noexcept {
		return mem_base_reg_ == Register::RIP ? memory_displacement64() : memory_displacement32();
	}

	/// Gets the virtual address of a memory operand
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4, must be a memory operand
	/// * `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * `get_register_value`: Function that returns the value of a register or the base address of a segment register, or `std::nullopt` for unsupported registers.
	///   It's called as `get_register_value(context, register_, element_index, element_size)`, see `GetRegisterValueFn`.
	/// * `context`: Passed to `get_register_value`
	///
	/// Returns `std::nullopt` if `get_register_value` returned `std::nullopt`.
	std::optional<std::uint64_t> virtual_address(std::uint32_t operand, std::size_t element_index, GetRegisterValueFn get_register_value, void* context) const;

	/// Gets the virtual address of a memory operand
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4, must be a memory operand
	/// * `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * `get_register_value`: Function (eg. a lambda) that returns the value of a register or the base address of a segment register, or `std::nullopt` for unsupported registers.
	///   Signature: `std::optional<std::uint64_t>(Register register_, std::size_t element_index, std::size_t element_size)`
	///
	/// # Call-back function args
	///
	/// * Arg 1: `register_`: Register (GPR8, GPR16, GPR32, GPR64, XMM, YMM, ZMM, seg). If it's a segment register, the call-back function should return the segment's base address, not the segment's register value.
	/// * Arg 2: `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * Arg 3: `element_size`: Only used if it's a vsib memory operand. Size in bytes of elements in vector index register (4 or 8).
	///
	/// Returns `std::nullopt` if `get_register_value` returned `std::nullopt`.
	template <typename F>
	std::optional<std::uint64_t> virtual_address(std::uint32_t operand, std::size_t element_index, F&& get_register_value) const {
		using FnType = std::remove_reference_t<F>;
		return virtual_address(
			operand, element_index,
			[](void* context, Register register_, std::size_t elem_index, std::size_t elem_size) -> std::optional<std::uint64_t> {
				return (*static_cast<FnType*>(context))(register_, elem_index, elem_size);
			},
			const_cast<void*>(static_cast<const void*>(std::addressof(get_register_value))));
	}

	/// Same as `virtual_address()`
	std::optional<std::uint64_t> try_virtual_address(std::uint32_t operand, std::size_t element_index, GetRegisterValueFn get_register_value, void* context) const {
		return virtual_address(operand, element_index, get_register_value, context);
	}

	/// Same as `virtual_address()`
	template <typename F>
	std::optional<std::uint64_t> try_virtual_address(std::uint32_t operand, std::size_t element_index, F&& get_register_value) const {
		return virtual_address(operand, element_index, std::forward<F>(get_register_value));
	}

	// ---- Instruction info (implemented by the instruction info component) ----

	/// Gets the number of bytes added to `SP`/`ESP`/`RSP` or 0 if it's not an instruction that pushes or pops data. This method assumes
	/// the instruction doesn't change the privilege level (eg. `IRET/D/Q`). If it's the `LEAVE` instruction, this method returns 0.
	std::int32_t stack_pointer_increment() const noexcept;

	/// Gets the FPU status word's `TOP` increment value and whether it's a conditional or unconditional push/pop
	/// and whether `TOP` is written.
	FpuStackIncrementInfo fpu_stack_increment_info() const noexcept;

	/// Instruction encoding, eg. Legacy, 3DNow!, VEX, EVEX, XOP
	EncodingKind encoding() const noexcept { return code_ext::encoding(code_); }

	/// Gets the CPU or CPUID feature flags
	Slice<CpuidFeature> cpuid_features() const noexcept { return code_ext::cpuid_features(code_); }

	/// Control flow info
	FlowControl flow_control() const noexcept { return code_ext::flow_control(code_); }

	/// `true` if it's a privileged instruction (all CPL=0 instructions (except `VMCALL`) and IOPL instructions `IN`, `INS`, `OUT`, `OUTS`, `CLI`, `STI`)
	bool is_privileged() const noexcept { return code_ext::is_privileged(code_); }

	/// `true` if this is an instruction that implicitly uses the stack pointer (`SP`/`ESP`/`RSP`), eg. `CALL`, `PUSH`, `POP`, `RET`, etc.
	/// See also `stack_pointer_increment()`
	bool is_stack_instruction() const noexcept { return code_ext::is_stack_instruction(code_); }

	/// `true` if it's an instruction that saves or restores too many registers (eg. `FXRSTOR`, `XSAVE`, etc).
	bool is_save_restore_instruction() const noexcept { return code_ext::is_save_restore_instruction(code_); }

	/// `true` if it's a "string" instruction, such as `MOVS`, `LODS`, `SCAS`, etc.
	constexpr bool is_string_instruction() const noexcept { return code_ext::is_string_instruction(code_); }

	/// All flags that are read by the CPU when executing the instruction.
	/// This method returns an `RflagsBits` value. See also `rflags_modified()`.
	std::uint32_t rflags_read() const noexcept;

	/// All flags that are written by the CPU, except those flags that are known to be undefined, always set or always cleared.
	/// This method returns an `RflagsBits` value. See also `rflags_modified()`.
	std::uint32_t rflags_written() const noexcept;

	/// All flags that are always cleared by the CPU.
	/// This method returns an `RflagsBits` value. See also `rflags_modified()`.
	std::uint32_t rflags_cleared() const noexcept;

	/// All flags that are always set by the CPU.
	/// This method returns an `RflagsBits` value. See also `rflags_modified()`.
	std::uint32_t rflags_set() const noexcept;

	/// All flags that are undefined after executing the instruction.
	/// This method returns an `RflagsBits` value. See also `rflags_modified()`.
	std::uint32_t rflags_undefined() const noexcept;

	/// All flags that are modified by the CPU. This is `rflags_written() + rflags_cleared() + rflags_set() + rflags_undefined()`. This method returns an `RflagsBits` value.
	std::uint32_t rflags_modified() const noexcept;

	// ---- Branch / condition code helpers ----

	/// Checks if it's a `Jcc SHORT` or `Jcc NEAR` instruction
	constexpr bool is_jcc_short_or_near() const noexcept { return code_ext::is_jcc_short_or_near(code_); }

	/// Checks if it's a `Jcc NEAR` instruction
	constexpr bool is_jcc_near() const noexcept { return code_ext::is_jcc_near(code_); }

	/// Checks if it's a `Jcc SHORT` instruction
	constexpr bool is_jcc_short() const noexcept { return code_ext::is_jcc_short(code_); }

	/// Checks if it's a `JMP SHORT` instruction
	constexpr bool is_jmp_short() const noexcept { return code_ext::is_jmp_short(code_); }

	/// Checks if it's a `JMP NEAR` instruction
	constexpr bool is_jmp_near() const noexcept { return code_ext::is_jmp_near(code_); }

	/// Checks if it's a `JMP SHORT` or a `JMP NEAR` instruction
	constexpr bool is_jmp_short_or_near() const noexcept { return code_ext::is_jmp_short_or_near(code_); }

	/// Checks if it's a `JMP FAR` instruction
	constexpr bool is_jmp_far() const noexcept { return code_ext::is_jmp_far(code_); }

	/// Checks if it's a `CALL NEAR` instruction
	constexpr bool is_call_near() const noexcept { return code_ext::is_call_near(code_); }

	/// Checks if it's a `CALL FAR` instruction
	constexpr bool is_call_far() const noexcept { return code_ext::is_call_far(code_); }

	/// Checks if it's a `JMP NEAR reg/[mem]` instruction
	constexpr bool is_jmp_near_indirect() const noexcept { return code_ext::is_jmp_near_indirect(code_); }

	/// Checks if it's a `JMP FAR [mem]` instruction
	constexpr bool is_jmp_far_indirect() const noexcept { return code_ext::is_jmp_far_indirect(code_); }

	/// Checks if it's a `CALL NEAR reg/[mem]` instruction
	constexpr bool is_call_near_indirect() const noexcept { return code_ext::is_call_near_indirect(code_); }

	/// Checks if it's a `CALL FAR [mem]` instruction
	constexpr bool is_call_far_indirect() const noexcept { return code_ext::is_call_far_indirect(code_); }

	/// Checks if it's a `JKccD SHORT` or `JKccD NEAR` instruction
	constexpr bool is_jkcc_short_or_near() const noexcept { return code_ext::is_jkcc_short_or_near(code_); }

	/// Checks if it's a `JKccD NEAR` instruction
	constexpr bool is_jkcc_near() const noexcept { return code_ext::is_jkcc_near(code_); }

	/// Checks if it's a `JKccD SHORT` instruction
	constexpr bool is_jkcc_short() const noexcept { return code_ext::is_jkcc_short(code_); }

	/// Checks if it's a `JCXZ SHORT`, `JECXZ SHORT` or `JRCXZ SHORT` instruction
	constexpr bool is_jcx_short() const noexcept { return code_ext::is_jcx_short(code_); }

	/// Checks if it's a `LOOPcc SHORT` instruction
	constexpr bool is_loopcc() const noexcept { return code_ext::is_loopcc(code_); }

	/// Checks if it's a `LOOP SHORT` instruction
	constexpr bool is_loop() const noexcept { return code_ext::is_loop(code_); }

	/// Negates the condition code, eg. `JE` -> `JNE`. Can be used if it's `Jcc`, `SETcc`, `CMOVcc`, `CMPccXADD`, `LOOPcc`
	/// and does nothing if the instruction doesn't have a condition code.
	void negate_condition_code() noexcept { code_ = code_ext::negate_condition_code(code_); }

	/// Converts `Jcc/JMP NEAR` to `Jcc/JMP SHORT` and does nothing if it's not a `Jcc/JMP NEAR` instruction
	void as_short_branch() noexcept { code_ = code_ext::as_short_branch(code_); }

	/// Converts `Jcc/JMP SHORT` to `Jcc/JMP NEAR` and does nothing if it's not a `Jcc/JMP SHORT` instruction
	void as_near_branch() noexcept { code_ = code_ext::as_near_branch(code_); }

	/// Gets the condition code if it's `Jcc`, `SETcc`, `CMOVcc`, `CMPccXADD`, `LOOPcc` else `ConditionCode::None` is returned
	ConditionCode condition_code() const noexcept { return code_ext::condition_code(code_); }

	// ---- Op code info (implemented by the encoder component) ----

	/// Gets the `OpCodeInfo`
	const OpCodeInfo& op_code() const noexcept { return code_ext::op_code(code_); }

	// ---- Comparison ----

	/// Checks if two instructions are equal. It ignores some fields (the IP, the instruction length and the code size),
	/// see also `eq_all_bits()`.
	bool operator==(const Instruction& other) const noexcept {
		return mem_displ_ == other.mem_displ_ && ((flags1_ ^ other.flags1_) & ~F1_EQUALS_IGNORE_MASK) == 0 && immediate_ == other.immediate_ &&
			code_ == other.code_ && mem_base_reg_ == other.mem_base_reg_ && mem_index_reg_ == other.mem_index_reg_ &&
			std::memcmp(regs_, other.regs_, sizeof(regs_)) == 0 && std::memcmp(op_kinds_, other.op_kinds_, sizeof(op_kinds_)) == 0 &&
			scale_ == other.scale_ && displ_size_ == other.displ_size_ && pad_ == other.pad_;
	}

	/// Checks if two instructions are not equal (see `operator==`)
	bool operator!=(const Instruction& other) const noexcept { return !(*this == other); }

	/// Gets a hash code of the fields compared by `operator==`
	std::size_t hash() const noexcept {
		// FNV-1a
		std::uint64_t h = 0xCBF2'9CE4'8422'2325ULL;
		auto add = [&h](std::uint64_t value) noexcept {
			h ^= value;
			h *= 0x0000'0100'0000'01B3ULL;
		};
		add(mem_displ_);
		add(flags1_ & ~F1_EQUALS_IGNORE_MASK);
		add(immediate_);
		add(static_cast<std::uint64_t>(code_));
		add(static_cast<std::uint64_t>(mem_base_reg_));
		add(static_cast<std::uint64_t>(mem_index_reg_));
		for (Register reg : regs_)
			add(static_cast<std::uint64_t>(reg));
		for (OpKind op_kind : op_kinds_)
			add(static_cast<std::uint64_t>(op_kind));
		add(scale_);
		add(displ_size_);
		add(pad_);
		return static_cast<std::size_t>(h);
	}

	// ---- Factory methods (implemented by the encoder component, Rust: instruction_create.rs) ----

	// GENERATOR-BEGIN: Create
	// ⚠️This was generated by GENERATOR!🦹‍♂️
	/// Creates an instruction with no operands
	///
	/// # Arguments
	///
	/// * `code`: Code value
	static Instruction with(Code code);

	/// Creates an instruction with 1 operand
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	static Result<Instruction> with1(Code code, Register register_);

	/// Creates an instruction with 1 operand
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate`: op0: Immediate value
	static Result<Instruction> with1(Code code, std::int32_t immediate);

	/// Creates an instruction with 1 operand
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate`: op0: Immediate value
	static Result<Instruction> with1(Code code, std::uint32_t immediate);

	/// Creates an instruction with 1 operand
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	static Result<Instruction> with1(Code code, const MemoryOperand& memory);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	static Result<Instruction> with2(Code code, Register register1, Register register2);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, Register register_, std::int32_t immediate);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, Register register_, std::uint32_t immediate);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, Register register_, std::int64_t immediate);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, Register register_, std::uint64_t immediate);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `memory`: op1: Memory operand
	static Result<Instruction> with2(Code code, Register register_, const MemoryOperand& memory);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate`: op0: Immediate value
	/// * `register_`: op1: Register
	static Result<Instruction> with2(Code code, std::int32_t immediate, Register register_);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate`: op0: Immediate value
	/// * `register_`: op1: Register
	static Result<Instruction> with2(Code code, std::uint32_t immediate, Register register_);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate1`: op0: Immediate value
	/// * `immediate2`: op1: Immediate value
	static Result<Instruction> with2(Code code, std::int32_t immediate1, std::int32_t immediate2);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `immediate1`: op0: Immediate value
	/// * `immediate2`: op1: Immediate value
	static Result<Instruction> with2(Code code, std::uint32_t immediate1, std::uint32_t immediate2);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `register_`: op1: Register
	static Result<Instruction> with2(Code code, const MemoryOperand& memory, Register register_);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, const MemoryOperand& memory, std::int32_t immediate);

	/// Creates an instruction with 2 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `immediate`: op1: Immediate value
	static Result<Instruction> with2(Code code, const MemoryOperand& memory, std::uint32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	static Result<Instruction> with3(Code code, Register register1, Register register2, Register register3);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register1, Register register2, std::int32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register1, Register register2, std::uint32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	static Result<Instruction> with3(Code code, Register register1, Register register2, const MemoryOperand& memory);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate1`: op1: Immediate value
	/// * `immediate2`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register_, std::int32_t immediate1, std::int32_t immediate2);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `immediate1`: op1: Immediate value
	/// * `immediate2`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register_, std::uint32_t immediate1, std::uint32_t immediate2);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `memory`: op1: Memory operand
	/// * `register2`: op2: Register
	static Result<Instruction> with3(Code code, Register register1, const MemoryOperand& memory, Register register2);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `memory`: op1: Memory operand
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register_, const MemoryOperand& memory, std::int32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register_`: op0: Register
	/// * `memory`: op1: Memory operand
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, Register register_, const MemoryOperand& memory, std::uint32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `register1`: op1: Register
	/// * `register2`: op2: Register
	static Result<Instruction> with3(Code code, const MemoryOperand& memory, Register register1, Register register2);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `register_`: op1: Register
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, const MemoryOperand& memory, Register register_, std::int32_t immediate);

	/// Creates an instruction with 3 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `memory`: op0: Memory operand
	/// * `register_`: op1: Register
	/// * `immediate`: op2: Immediate value
	static Result<Instruction> with3(Code code, const MemoryOperand& memory, Register register_, std::uint32_t immediate);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `register4`: op3: Register
	static Result<Instruction> with4(Code code, Register register1, Register register2, Register register3, Register register4);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `immediate`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, Register register3, std::int32_t immediate);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `immediate`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, Register register3, std::uint32_t immediate);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `memory`: op3: Memory operand
	static Result<Instruction> with4(Code code, Register register1, Register register2, Register register3, const MemoryOperand& memory);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `immediate1`: op2: Immediate value
	/// * `immediate2`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, std::int32_t immediate1, std::int32_t immediate2);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `immediate1`: op2: Immediate value
	/// * `immediate2`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, std::uint32_t immediate1, std::uint32_t immediate2);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	/// * `register3`: op3: Register
	static Result<Instruction> with4(Code code, Register register1, Register register2, const MemoryOperand& memory, Register register3);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	/// * `immediate`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, const MemoryOperand& memory, std::int32_t immediate);

	/// Creates an instruction with 4 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	/// * `immediate`: op3: Immediate value
	static Result<Instruction> with4(Code code, Register register1, Register register2, const MemoryOperand& memory, std::uint32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `register4`: op3: Register
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, Register register3, Register register4, std::int32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `register4`: op3: Register
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, Register register3, Register register4, std::uint32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `memory`: op3: Memory operand
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, Register register3, const MemoryOperand& memory, std::int32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `register3`: op2: Register
	/// * `memory`: op3: Memory operand
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, Register register3, const MemoryOperand& memory, std::uint32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	/// * `register3`: op3: Register
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, const MemoryOperand& memory, Register register3, std::int32_t immediate);

	/// Creates an instruction with 5 operands
	///
	/// # Errors
	///
	/// Fails if one of the operands is invalid (basic checks)
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `register1`: op0: Register
	/// * `register2`: op1: Register
	/// * `memory`: op2: Memory operand
	/// * `register3`: op3: Register
	/// * `immediate`: op4: Immediate value
	static Result<Instruction> with5(Code code, Register register1, Register register2, const MemoryOperand& memory, Register register3, std::uint32_t immediate);

	/// Creates a new near/short branch instruction
	///
	/// # Errors
	///
	/// Fails if the created instruction doesn't have a near branch operand
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `target`: Target address
	static Result<Instruction> with_branch(Code code, std::uint64_t target);

	/// Creates a new far branch instruction
	///
	/// # Errors
	///
	/// Fails if the created instruction doesn't have a far branch operand
	///
	/// # Arguments
	///
	/// * `code`: Code value
	/// * `selector`: Selector/segment value
	/// * `offset`: Offset
	static Result<Instruction> with_far_branch(Code code, std::uint16_t selector, std::uint32_t offset);

	/// Creates a new `XBEGIN` instruction
	///
	/// # Errors
	///
	/// Fails if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32, or 64
	/// * `target`: Target address
	static Result<Instruction> with_xbegin(std::uint32_t bitness, std::uint64_t target);

	/// Creates a `OUTSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_outsb(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP OUTSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_outsb(std::uint32_t address_size);

	/// Creates a `OUTSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_outsw(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP OUTSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_outsw(std::uint32_t address_size);

	/// Creates a `OUTSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_outsd(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP OUTSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_outsd(std::uint32_t address_size);

	/// Creates a `LODSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_lodsb(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP LODSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_lodsb(std::uint32_t address_size);

	/// Creates a `LODSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_lodsw(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP LODSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_lodsw(std::uint32_t address_size);

	/// Creates a `LODSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_lodsd(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP LODSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_lodsd(std::uint32_t address_size);

	/// Creates a `LODSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_lodsq(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP LODSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_lodsq(std::uint32_t address_size);

	/// Creates a `SCASB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_scasb(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE SCASB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_scasb(std::uint32_t address_size);

	/// Creates a `REPNE SCASB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_scasb(std::uint32_t address_size);

	/// Creates a `SCASW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_scasw(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE SCASW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_scasw(std::uint32_t address_size);

	/// Creates a `REPNE SCASW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_scasw(std::uint32_t address_size);

	/// Creates a `SCASD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_scasd(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE SCASD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_scasd(std::uint32_t address_size);

	/// Creates a `REPNE SCASD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_scasd(std::uint32_t address_size);

	/// Creates a `SCASQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_scasq(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE SCASQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_scasq(std::uint32_t address_size);

	/// Creates a `REPNE SCASQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_scasq(std::uint32_t address_size);

	/// Creates a `INSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_insb(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP INSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_insb(std::uint32_t address_size);

	/// Creates a `INSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_insw(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP INSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_insw(std::uint32_t address_size);

	/// Creates a `INSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_insd(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP INSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_insd(std::uint32_t address_size);

	/// Creates a `STOSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_stosb(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP STOSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_stosb(std::uint32_t address_size);

	/// Creates a `STOSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_stosw(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP STOSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_stosw(std::uint32_t address_size);

	/// Creates a `STOSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_stosd(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP STOSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_stosd(std::uint32_t address_size);

	/// Creates a `STOSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_stosq(std::uint32_t address_size, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP STOSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_stosq(std::uint32_t address_size);

	/// Creates a `CMPSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_cmpsb(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE CMPSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_cmpsb(std::uint32_t address_size);

	/// Creates a `REPNE CMPSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_cmpsb(std::uint32_t address_size);

	/// Creates a `CMPSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_cmpsw(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE CMPSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_cmpsw(std::uint32_t address_size);

	/// Creates a `REPNE CMPSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_cmpsw(std::uint32_t address_size);

	/// Creates a `CMPSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_cmpsd(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE CMPSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_cmpsd(std::uint32_t address_size);

	/// Creates a `REPNE CMPSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_cmpsd(std::uint32_t address_size);

	/// Creates a `CMPSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_cmpsq(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REPE CMPSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repe_cmpsq(std::uint32_t address_size);

	/// Creates a `REPNE CMPSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_repne_cmpsq(std::uint32_t address_size);

	/// Creates a `MOVSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_movsb(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP MOVSB` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_movsb(std::uint32_t address_size);

	/// Creates a `MOVSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_movsw(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP MOVSW` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_movsw(std::uint32_t address_size);

	/// Creates a `MOVSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_movsd(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP MOVSD` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_movsd(std::uint32_t address_size);

	/// Creates a `MOVSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `segment_prefix`: Segment override or `Register::None`
	/// * `rep_prefix`: Rep prefix or `RepPrefixKind::None`
	static Result<Instruction> with_movsq(std::uint32_t address_size, Register segment_prefix = Register::None, RepPrefixKind rep_prefix = RepPrefixKind::None);

	/// Creates a `REP MOVSQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	static Result<Instruction> with_rep_movsq(std::uint32_t address_size);

	/// Creates a `MASKMOVQ` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `register1`: Register
	/// * `register2`: Register
	/// * `segment_prefix`: Segment override or `Register::None`
	static Result<Instruction> with_maskmovq(std::uint32_t address_size, Register register1, Register register2, Register segment_prefix = Register::None);

	/// Creates a `MASKMOVDQU` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `register1`: Register
	/// * `register2`: Register
	/// * `segment_prefix`: Segment override or `Register::None`
	static Result<Instruction> with_maskmovdqu(std::uint32_t address_size, Register register1, Register register2, Register segment_prefix = Register::None);

	/// Creates a `VMASKMOVDQU` instruction
	///
	/// # Errors
	///
	/// Fails if `address_size` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `address_size`: 16, 32, or 64
	/// * `register1`: Register
	/// * `register2`: Register
	/// * `segment_prefix`: Segment override or `Register::None`
	static Result<Instruction> with_vmaskmovdqu(std::uint32_t address_size, Register register1, Register register2, Register segment_prefix = Register::None);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	static Instruction with_declare_byte_1(std::uint8_t b0);

	/// Same as `with_declare_byte_1()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	static Result<Instruction> try_with_declare_byte_1(std::uint8_t b0);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	static Instruction with_declare_byte_2(std::uint8_t b0, std::uint8_t b1);

	/// Same as `with_declare_byte_2()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	static Result<Instruction> try_with_declare_byte_2(std::uint8_t b0, std::uint8_t b1);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	static Instruction with_declare_byte_3(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2);

	/// Same as `with_declare_byte_3()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	static Result<Instruction> try_with_declare_byte_3(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	static Instruction with_declare_byte_4(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3);

	/// Same as `with_declare_byte_4()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	static Result<Instruction> try_with_declare_byte_4(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	static Instruction with_declare_byte_5(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4);

	/// Same as `with_declare_byte_5()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	static Result<Instruction> try_with_declare_byte_5(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	static Instruction with_declare_byte_6(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5);

	/// Same as `with_declare_byte_6()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	static Result<Instruction> try_with_declare_byte_6(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	static Instruction with_declare_byte_7(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6);

	/// Same as `with_declare_byte_7()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	static Result<Instruction> try_with_declare_byte_7(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	static Instruction with_declare_byte_8(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7);

	/// Same as `with_declare_byte_8()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	static Result<Instruction> try_with_declare_byte_8(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	static Instruction with_declare_byte_9(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8);

	/// Same as `with_declare_byte_9()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	static Result<Instruction> try_with_declare_byte_9(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	static Instruction with_declare_byte_10(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9);

	/// Same as `with_declare_byte_10()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	static Result<Instruction> try_with_declare_byte_10(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	static Instruction with_declare_byte_11(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10);

	/// Same as `with_declare_byte_11()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	static Result<Instruction> try_with_declare_byte_11(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	static Instruction with_declare_byte_12(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11);

	/// Same as `with_declare_byte_12()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	static Result<Instruction> try_with_declare_byte_12(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	static Instruction with_declare_byte_13(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12);

	/// Same as `with_declare_byte_13()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	static Result<Instruction> try_with_declare_byte_13(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	static Instruction with_declare_byte_14(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13);

	/// Same as `with_declare_byte_14()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	static Result<Instruction> try_with_declare_byte_14(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	/// * `b14`: Byte 14
	static Instruction with_declare_byte_15(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13, std::uint8_t b14);

	/// Same as `with_declare_byte_15()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	/// * `b14`: Byte 14
	static Result<Instruction> try_with_declare_byte_15(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13, std::uint8_t b14);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	/// * `b14`: Byte 14
	/// * `b15`: Byte 15
	static Instruction with_declare_byte_16(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13, std::uint8_t b14, std::uint8_t b15);

	/// Same as `with_declare_byte_16()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `b0`: Byte 0
	/// * `b1`: Byte 1
	/// * `b2`: Byte 2
	/// * `b3`: Byte 3
	/// * `b4`: Byte 4
	/// * `b5`: Byte 5
	/// * `b6`: Byte 6
	/// * `b7`: Byte 7
	/// * `b8`: Byte 8
	/// * `b9`: Byte 9
	/// * `b10`: Byte 10
	/// * `b11`: Byte 11
	/// * `b12`: Byte 12
	/// * `b13`: Byte 13
	/// * `b14`: Byte 14
	/// * `b15`: Byte 15
	static Result<Instruction> try_with_declare_byte_16(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13, std::uint8_t b14, std::uint8_t b15);

	/// Creates a `db`/`.byte` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 1-16
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_byte(const std::uint8_t* data, std::size_t size);

	/// Same as `with_declare_byte(data, size)`
	static Result<Instruction> with_declare_byte(std::initializer_list<std::uint8_t> data) { return with_declare_byte(data.begin(), data.size()); }

	/// Same as `with_declare_byte(data, size)`
	static Result<Instruction> with_declare_byte(const std::vector<std::uint8_t>& data) { return with_declare_byte(data.data(), data.size()); }

	/// Same as `with_declare_byte(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_byte(const std::uint8_t (&data)[N]) { return with_declare_byte(data, N); }

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	static Instruction with_declare_word_1(std::uint16_t w0);

	/// Same as `with_declare_word_1()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	static Result<Instruction> try_with_declare_word_1(std::uint16_t w0);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	static Instruction with_declare_word_2(std::uint16_t w0, std::uint16_t w1);

	/// Same as `with_declare_word_2()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	static Result<Instruction> try_with_declare_word_2(std::uint16_t w0, std::uint16_t w1);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	static Instruction with_declare_word_3(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2);

	/// Same as `with_declare_word_3()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	static Result<Instruction> try_with_declare_word_3(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	static Instruction with_declare_word_4(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3);

	/// Same as `with_declare_word_4()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	static Result<Instruction> try_with_declare_word_4(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	static Instruction with_declare_word_5(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4);

	/// Same as `with_declare_word_5()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	static Result<Instruction> try_with_declare_word_5(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	static Instruction with_declare_word_6(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5);

	/// Same as `with_declare_word_6()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	static Result<Instruction> try_with_declare_word_6(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	/// * `w6`: Word 6
	static Instruction with_declare_word_7(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5, std::uint16_t w6);

	/// Same as `with_declare_word_7()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	/// * `w6`: Word 6
	static Result<Instruction> try_with_declare_word_7(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5, std::uint16_t w6);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	/// * `w6`: Word 6
	/// * `w7`: Word 7
	static Instruction with_declare_word_8(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5, std::uint16_t w6, std::uint16_t w7);

	/// Same as `with_declare_word_8()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `w0`: Word 0
	/// * `w1`: Word 1
	/// * `w2`: Word 2
	/// * `w3`: Word 3
	/// * `w4`: Word 4
	/// * `w5`: Word 5
	/// * `w6`: Word 6
	/// * `w7`: Word 7
	static Result<Instruction> try_with_declare_word_8(std::uint16_t w0, std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4, std::uint16_t w5, std::uint16_t w6, std::uint16_t w7);

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 2-16 or not a multiple of 2
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_word_slice_u8(const std::uint8_t* data, std::size_t size);

	/// Same as `with_declare_word_slice_u8(data, size)`
	static Result<Instruction> with_declare_word_slice_u8(std::initializer_list<std::uint8_t> data) { return with_declare_word_slice_u8(data.begin(), data.size()); }

	/// Same as `with_declare_word_slice_u8(data, size)`
	static Result<Instruction> with_declare_word_slice_u8(const std::vector<std::uint8_t>& data) { return with_declare_word_slice_u8(data.data(), data.size()); }

	/// Same as `with_declare_word_slice_u8(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_word_slice_u8(const std::uint8_t (&data)[N]) { return with_declare_word_slice_u8(data, N); }

	/// Creates a `dw`/`.word` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 1-8
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_word(const std::uint16_t* data, std::size_t size);

	/// Same as `with_declare_word(data, size)`
	static Result<Instruction> with_declare_word(std::initializer_list<std::uint16_t> data) { return with_declare_word(data.begin(), data.size()); }

	/// Same as `with_declare_word(data, size)`
	static Result<Instruction> with_declare_word(const std::vector<std::uint16_t>& data) { return with_declare_word(data.data(), data.size()); }

	/// Same as `with_declare_word(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_word(const std::uint16_t (&data)[N]) { return with_declare_word(data, N); }

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	static Instruction with_declare_dword_1(std::uint32_t d0);

	/// Same as `with_declare_dword_1()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	static Result<Instruction> try_with_declare_dword_1(std::uint32_t d0);

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	static Instruction with_declare_dword_2(std::uint32_t d0, std::uint32_t d1);

	/// Same as `with_declare_dword_2()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	static Result<Instruction> try_with_declare_dword_2(std::uint32_t d0, std::uint32_t d1);

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	/// * `d2`: Dword 2
	static Instruction with_declare_dword_3(std::uint32_t d0, std::uint32_t d1, std::uint32_t d2);

	/// Same as `with_declare_dword_3()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	/// * `d2`: Dword 2
	static Result<Instruction> try_with_declare_dword_3(std::uint32_t d0, std::uint32_t d1, std::uint32_t d2);

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	/// * `d2`: Dword 2
	/// * `d3`: Dword 3
	static Instruction with_declare_dword_4(std::uint32_t d0, std::uint32_t d1, std::uint32_t d2, std::uint32_t d3);

	/// Same as `with_declare_dword_4()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `d0`: Dword 0
	/// * `d1`: Dword 1
	/// * `d2`: Dword 2
	/// * `d3`: Dword 3
	static Result<Instruction> try_with_declare_dword_4(std::uint32_t d0, std::uint32_t d1, std::uint32_t d2, std::uint32_t d3);

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 4-16 or not a multiple of 4
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_dword_slice_u8(const std::uint8_t* data, std::size_t size);

	/// Same as `with_declare_dword_slice_u8(data, size)`
	static Result<Instruction> with_declare_dword_slice_u8(std::initializer_list<std::uint8_t> data) { return with_declare_dword_slice_u8(data.begin(), data.size()); }

	/// Same as `with_declare_dword_slice_u8(data, size)`
	static Result<Instruction> with_declare_dword_slice_u8(const std::vector<std::uint8_t>& data) { return with_declare_dword_slice_u8(data.data(), data.size()); }

	/// Same as `with_declare_dword_slice_u8(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_dword_slice_u8(const std::uint8_t (&data)[N]) { return with_declare_dword_slice_u8(data, N); }

	/// Creates a `dd`/`.int` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 1-4
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_dword(const std::uint32_t* data, std::size_t size);

	/// Same as `with_declare_dword(data, size)`
	static Result<Instruction> with_declare_dword(std::initializer_list<std::uint32_t> data) { return with_declare_dword(data.begin(), data.size()); }

	/// Same as `with_declare_dword(data, size)`
	static Result<Instruction> with_declare_dword(const std::vector<std::uint32_t>& data) { return with_declare_dword(data.data(), data.size()); }

	/// Same as `with_declare_dword(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_dword(const std::uint32_t (&data)[N]) { return with_declare_dword(data, N); }

	/// Creates a `dq`/`.quad` asm directive
	///
	/// # Arguments
	///
	/// * `q0`: Qword 0
	static Instruction with_declare_qword_1(std::uint64_t q0);

	/// Same as `with_declare_qword_1()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `q0`: Qword 0
	static Result<Instruction> try_with_declare_qword_1(std::uint64_t q0);

	/// Creates a `dq`/`.quad` asm directive
	///
	/// # Arguments
	///
	/// * `q0`: Qword 0
	/// * `q1`: Qword 1
	static Instruction with_declare_qword_2(std::uint64_t q0, std::uint64_t q1);

	/// Same as `with_declare_qword_2()` but returns a `Result<Instruction>` (it never fails)
	///
	/// # Arguments
	///
	/// * `q0`: Qword 0
	/// * `q1`: Qword 1
	static Result<Instruction> try_with_declare_qword_2(std::uint64_t q0, std::uint64_t q1);

	/// Creates a `dq`/`.quad` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 8-16 or not a multiple of 8
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_qword_slice_u8(const std::uint8_t* data, std::size_t size);

	/// Same as `with_declare_qword_slice_u8(data, size)`
	static Result<Instruction> with_declare_qword_slice_u8(std::initializer_list<std::uint8_t> data) { return with_declare_qword_slice_u8(data.begin(), data.size()); }

	/// Same as `with_declare_qword_slice_u8(data, size)`
	static Result<Instruction> with_declare_qword_slice_u8(const std::vector<std::uint8_t>& data) { return with_declare_qword_slice_u8(data.data(), data.size()); }

	/// Same as `with_declare_qword_slice_u8(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_qword_slice_u8(const std::uint8_t (&data)[N]) { return with_declare_qword_slice_u8(data, N); }

	/// Creates a `dq`/`.quad` asm directive
	///
	/// # Errors
	///
	/// Fails if `size` is not 1-2
	///
	/// # Arguments
	///
	/// * `data`: Data
	/// * `size`: Number of elements in `data`
	static Result<Instruction> with_declare_qword(const std::uint64_t* data, std::size_t size);

	/// Same as `with_declare_qword(data, size)`
	static Result<Instruction> with_declare_qword(std::initializer_list<std::uint64_t> data) { return with_declare_qword(data.begin(), data.size()); }

	/// Same as `with_declare_qword(data, size)`
	static Result<Instruction> with_declare_qword(const std::vector<std::uint64_t>& data) { return with_declare_qword(data.data(), data.size()); }

	/// Same as `with_declare_qword(data, size)`
	template <std::size_t N>
	static Result<Instruction> with_declare_qword(const std::uint64_t (&data)[N]) { return with_declare_qword(data, N); }
	// GENERATOR-END: Create

	// `with1()`..`with5()` overloads for integer types that aren't `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t`
	// (eg. `long long`, `unsigned long`, `std::size_t` on some platforms). They would be ambiguous without these templates.

	/// Same as the other `with1()` overloads but it also accepts other integer types (eg. `long long`, `unsigned long`):
	/// each integer arg is converted to the `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t` with the same size
	/// and signedness.
	template <typename... Args, internal::EnableIfOtherIntArgs<Args...> = 0>
	static Result<Instruction> with1(Code code, const Args&... args) {
		return with1(code, internal::int_arg(args)...);
	}

	/// Same as the other `with2()` overloads but it also accepts other integer types (eg. `long long`, `unsigned long`):
	/// each integer arg is converted to the `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t` with the same size
	/// and signedness.
	template <typename... Args, internal::EnableIfOtherIntArgs<Args...> = 0>
	static Result<Instruction> with2(Code code, const Args&... args) {
		return with2(code, internal::int_arg(args)...);
	}

	/// Same as the other `with3()` overloads but it also accepts other integer types (eg. `long long`, `unsigned long`):
	/// each integer arg is converted to the `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t` with the same size
	/// and signedness.
	template <typename... Args, internal::EnableIfOtherIntArgs<Args...> = 0>
	static Result<Instruction> with3(Code code, const Args&... args) {
		return with3(code, internal::int_arg(args)...);
	}

	/// Same as the other `with4()` overloads but it also accepts other integer types (eg. `long long`, `unsigned long`):
	/// each integer arg is converted to the `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t` with the same size
	/// and signedness.
	template <typename... Args, internal::EnableIfOtherIntArgs<Args...> = 0>
	static Result<Instruction> with4(Code code, const Args&... args) {
		return with4(code, internal::int_arg(args)...);
	}

	/// Same as the other `with5()` overloads but it also accepts other integer types (eg. `long long`, `unsigned long`):
	/// each integer arg is converted to the `std::int32_t`/`std::uint32_t`/`std::int64_t`/`std::uint64_t` with the same size
	/// and signedness.
	template <typename... Args, internal::EnableIfOtherIntArgs<Args...> = 0>
	static Result<Instruction> with5(Code code, const Args&... args) {
		return with5(code, internal::int_arg(args)...);
	}

private:
	// Copies of the internal::InstrFlags1 and internal::MvexInstrFlags constants (verified in instruction.cpp)
	static constexpr std::uint32_t F1_SEGMENT_PREFIX_MASK = 0x0000'0007;
	static constexpr std::uint32_t F1_SEGMENT_PREFIX_SHIFT = 0x0000'0005;
	static constexpr std::uint32_t F1_DATA_LENGTH_MASK = 0x0000'000F;
	static constexpr std::uint32_t F1_DATA_LENGTH_SHIFT = 0x0000'0008;
	static constexpr std::uint32_t F1_ROUNDING_CONTROL_MASK = 0x0000'0007;
	static constexpr std::uint32_t F1_ROUNDING_CONTROL_SHIFT = 0x0000'000C;
	static constexpr std::uint32_t F1_OP_MASK_MASK = 0x0000'0007;
	static constexpr std::uint32_t F1_OP_MASK_SHIFT = 0x0000'000F;
	static constexpr std::uint32_t F1_CODE_SIZE_MASK = 0x0000'0003;
	static constexpr std::uint32_t F1_CODE_SIZE_SHIFT = 0x0000'0012;
	static constexpr std::uint32_t F1_BROADCAST = 0x0400'0000;
	static constexpr std::uint32_t F1_SUPPRESS_ALL_EXCEPTIONS = 0x0800'0000;
	static constexpr std::uint32_t F1_ZEROING_MASKING = 0x1000'0000;
	static constexpr std::uint32_t F1_REPE_PREFIX = 0x2000'0000;
	static constexpr std::uint32_t F1_REPNE_PREFIX = 0x4000'0000;
	static constexpr std::uint32_t F1_LOCK_PREFIX = 0x8000'0000;
	static constexpr std::uint32_t F1_EQUALS_IGNORE_MASK = 0x000C'0000;
	static constexpr std::uint32_t MVEX_REG_MEM_CONV_SHIFT = 0x0000'0010;
	static constexpr std::uint32_t MVEX_REG_MEM_CONV_MASK = 0x0000'001F;
	static constexpr std::uint32_t MVEX_EVICTION_HINT = 0x8000'0000;

	// Returns false if it's not an immediate operand
	bool get_immediate_core(std::uint32_t operand, std::uint64_t& value) const noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand > 4)
			return false;
		switch (operand < 4 ? op_kinds_[operand] : op4_kind()) {
		case OpKind::Immediate8:
			value = immediate8();
			return true;
		case OpKind::Immediate8_2nd:
			value = immediate8_2nd();
			return true;
		case OpKind::Immediate16:
			value = immediate16();
			return true;
		case OpKind::Immediate32:
			value = immediate32();
			return true;
		case OpKind::Immediate64:
			value = immediate64();
			return true;
		case OpKind::Immediate8to16:
			value = static_cast<std::uint64_t>(static_cast<std::int64_t>(immediate8to16()));
			return true;
		case OpKind::Immediate8to32:
			value = static_cast<std::uint64_t>(static_cast<std::int64_t>(immediate8to32()));
			return true;
		case OpKind::Immediate8to64:
			value = static_cast<std::uint64_t>(immediate8to64());
			return true;
		case OpKind::Immediate32to64:
			value = static_cast<std::uint64_t>(immediate32to64());
			return true;
		default:
			return false;
		}
	}

	// Returns false if it's not an immediate operand
	bool set_immediate_core(std::uint32_t operand, std::uint64_t new_value) noexcept {
		static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
		if (operand > 4)
			return false;
		switch (operand < 4 ? op_kinds_[operand] : op4_kind()) {
		case OpKind::Immediate8:
			set_immediate8(static_cast<std::uint8_t>(new_value));
			return true;
		case OpKind::Immediate8to16:
			set_immediate8to16(static_cast<std::int16_t>(new_value));
			return true;
		case OpKind::Immediate8to32:
			set_immediate8to32(static_cast<std::int32_t>(new_value));
			return true;
		case OpKind::Immediate8to64:
			set_immediate8to64(static_cast<std::int64_t>(new_value));
			return true;
		case OpKind::Immediate8_2nd:
			set_immediate8_2nd(static_cast<std::uint8_t>(new_value));
			return true;
		case OpKind::Immediate16:
			set_immediate16(static_cast<std::uint16_t>(new_value));
			return true;
		case OpKind::Immediate32to64:
			set_immediate32to64(static_cast<std::int64_t>(new_value));
			return true;
		case OpKind::Immediate32:
			set_immediate32(static_cast<std::uint32_t>(new_value));
			return true;
		case OpKind::Immediate64:
			set_immediate64(new_value);
			return true;
		default:
			return false;
		}
	}

	void set_flags1_bit(std::uint32_t bit, bool value) noexcept {
		if (value)
			flags1_ |= bit;
		else
			flags1_ &= ~bit;
	}

	bool is_xacquire_instr() const noexcept {
		// This method can return true even if it's not an xacquire/xrelease instruction. This only happens if
		// it has an invalid LOCK prefix though.
		if (op0_kind() != OpKind::Memory)
			return false;
		if (has_lock_prefix())
			return code_ != Code::Cmpxchg16b_m128;
		return mnemonic() == Mnemonic::Xchg;
	}

	bool is_xrelease_instr() const noexcept {
		// This method can return true even if it's not an xacquire/xrelease instruction. This only happens if
		// it has an invalid LOCK prefix though.
		if (op0_kind() != OpKind::Memory)
			return false;
		if (has_lock_prefix())
			return code_ != Code::Cmpxchg16b_m128;
		switch (code_) {
		case Code::Xchg_rm8_r8:
		case Code::Xchg_rm16_r16:
		case Code::Xchg_rm32_r32:
		case Code::Xchg_rm64_r64:
		case Code::Mov_rm8_r8:
		case Code::Mov_rm16_r16:
		case Code::Mov_rm32_r32:
		case Code::Mov_rm64_r64:
		case Code::Mov_rm8_imm8:
		case Code::Mov_rm16_imm16:
		case Code::Mov_rm32_imm32:
		case Code::Mov_rm64_imm32:
			return true;
		default:
			return false;
		}
	}

	std::uint64_t next_rip_ = 0;
	std::uint64_t mem_displ_ = 0;
	std::uint32_t flags1_ = 0; // InstrFlags1
	std::uint32_t immediate_ = 0;
	Code code_ = Code::INVALID;
	Register mem_base_reg_ = Register::None;
	Register mem_index_reg_ = Register::None;
	Register regs_[4] = {Register::None, Register::None, Register::None, Register::None};
	OpKind op_kinds_[4] = {OpKind::Register, OpKind::Register, OpKind::Register, OpKind::Register};
	std::uint8_t scale_ = 0; // InstrScale
	std::uint8_t displ_size_ = 0;
	std::uint8_t len_ = 0;
	std::uint8_t pad_ = 0;
};

static_assert(sizeof(Instruction) == 40, "Instruction must have the same size as the Rust Instruction");
static_assert(std::is_trivially_copyable<Instruction>::value, "Instruction must be trivially copyable");

/// Formats the instruction using the default formatter (Rust: `impl Display for Instruction`).
/// Implemented by the formatters component.
std::string to_string(const Instruction& instruction);

} // namespace iced_x86

namespace std {
/// Hashes an `Instruction` (same fields as `operator==`)
template <>
struct hash<iced_x86::Instruction> {
	std::size_t operator()(const iced_x86::Instruction& instruction) const noexcept { return instruction.hash(); }
};
} // namespace std
