// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_size.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/op_access.hpp"
#include "iced_x86/register.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace iced_x86 {

class Instruction;
class InstructionInfoFactory;

namespace internal {
struct InstructionInfoFactoryImpl;
} // namespace internal

/// A register used by an instruction
class UsedRegister {
public:
	/// Creates a new instance (`Register::None`, `OpAccess::None`)
	constexpr UsedRegister() noexcept : register_value_(Register::None), access_(OpAccess::None) {}

	/// Creates a new instance
	///
	/// # Arguments
	///
	/// * `reg`: Register
	/// * `access`: Register access
	constexpr UsedRegister(Register reg, OpAccess access) noexcept : register_value_(reg), access_(access) {}

	/// Gets the register
	constexpr Register register_() const noexcept { return register_value_; }

	/// Gets the register access
	constexpr OpAccess access() const noexcept { return access_; }

	constexpr bool operator==(const UsedRegister& other) const noexcept {
		return register_value_ == other.register_value_ && access_ == other.access_;
	}
	constexpr bool operator!=(const UsedRegister& other) const noexcept { return !(*this == other); }

private:
	friend struct internal::InstructionInfoFactoryImpl;

	Register register_value_;
	OpAccess access_;
};

/// A memory location used by an instruction
class UsedMemory {
public:
	/// Creates a new instance (all fields are cleared)
	constexpr UsedMemory() noexcept
		: displacement_(0), segment_(Register::None), base_(Register::None), index_(Register::None), scale_(0), memory_size_(MemorySize::Unknown),
		  access_(OpAccess::None), address_size_(CodeSize::Unknown), vsib_size_(0) {}

	/// Creates a new instance
	///
	/// # Arguments
	///
	/// * `segment`: Effective segment register or `Register::None` if the segment register is ignored
	/// * `base`: Base register
	/// * `index`: Index register
	/// * `scale`: 1, 2, 4 or 8
	/// * `displacement`: Displacement
	/// * `memory_size`: Memory size
	/// * `access`: Access
	constexpr UsedMemory(Register segment, Register base, Register index, std::uint32_t scale, std::uint64_t displacement, MemorySize memory_size,
		OpAccess access) noexcept
		: displacement_(displacement), segment_(segment), base_(base), index_(index), scale_(static_cast<std::uint8_t>(scale)),
		  memory_size_(memory_size), access_(access), address_size_(CodeSize::Unknown), vsib_size_(0) {}

	/// Creates a new instance
	///
	/// # Arguments
	///
	/// * `segment`: Effective segment register or `Register::None` if the segment register is ignored
	/// * `base`: Base register
	/// * `index`: Index register
	/// * `scale`: 1, 2, 4 or 8
	/// * `displacement`: Displacement
	/// * `memory_size`: Memory size
	/// * `access`: Access
	/// * `address_size`: Address size
	/// * `vsib_size`: VSIB size (`0`, `4` or `8`)
	constexpr UsedMemory(Register segment, Register base, Register index, std::uint32_t scale, std::uint64_t displacement, MemorySize memory_size,
		OpAccess access, CodeSize address_size, std::uint32_t vsib_size) noexcept
		: displacement_(displacement), segment_(segment), base_(base), index_(index), scale_(static_cast<std::uint8_t>(scale)),
		  memory_size_(memory_size), access_(access), address_size_(address_size), vsib_size_(static_cast<std::uint8_t>(vsib_size)) {}

	/// Effective segment register or `Register::None` if the segment register is ignored
	constexpr Register segment() const noexcept { return segment_; }

	/// Base register or `Register::None` if none
	constexpr Register base() const noexcept { return base_; }

	/// Index register or `Register::None` if none
	constexpr Register index() const noexcept { return index_; }

	/// Index scale (1, 2, 4 or 8)
	constexpr std::uint32_t scale() const noexcept { return scale_; }

	/// Displacement
	constexpr std::uint64_t displacement() const noexcept { return displacement_; }

	/// Size of location
	constexpr MemorySize memory_size() const noexcept { return memory_size_; }

	/// Memory access
	constexpr OpAccess access() const noexcept { return access_; }

	/// Address size
	constexpr CodeSize address_size() const noexcept { return address_size_; }

	/// VSIB size (`0`, `4` or `8`)
	constexpr std::uint32_t vsib_size() const noexcept { return vsib_size_; }

	/// Gets the virtual address of a used memory location, or `std::nullopt` if register resolution fails.
	///
	/// # Arguments
	///
	/// * `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * `get_register_value`: Function that returns the value of a register or the base address of a segment register, or `std::nullopt` on failure.
	///
	/// # Call-back function args
	///
	/// The call-back function has the signature `std::optional<std::uint64_t>(Register register, std::size_t element_index, std::size_t element_size)`.
	///
	/// * Arg 1: `register`: Register. If it's a segment register, the call-back should return the segment's base address, not the segment's register value.
	/// * Arg 2: `element_index`: Only used if it's a vsib memory operand. This is the element index of the vector index register.
	/// * Arg 3: `element_size`: Only used if it's a vsib memory operand. Size in bytes of elements in vector index register (4 or 8).
	template <typename F>
	std::optional<std::uint64_t> virtual_address(std::size_t element_index, F&& get_register_value) const {
		return try_virtual_address(element_index, std::forward<F>(get_register_value));
	}

	/// Same as `virtual_address()`
	template <typename F>
	std::optional<std::uint64_t> try_virtual_address(std::size_t element_index, F&& get_register_value) const {
		std::uint64_t effective = displacement_;

		if (base_ != Register::None) {
			std::optional<std::uint64_t> base = get_register_value(base_, static_cast<std::size_t>(0), static_cast<std::size_t>(0));
			if (!base)
				return std::nullopt;
			effective += *base;
		}

		if (index_ != Register::None) {
			std::optional<std::uint64_t> index_value = get_register_value(index_, element_index, static_cast<std::size_t>(vsib_size_));
			if (!index_value)
				return std::nullopt;
			std::uint64_t index = *index_value;
			if (vsib_size_ == 4)
				index = static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(index))));
			effective += index * static_cast<std::uint64_t>(scale_);
		}

		switch (address_size_) {
		case CodeSize::Code16:
			effective = static_cast<std::uint16_t>(effective);
			break;
		case CodeSize::Code32:
			effective = static_cast<std::uint32_t>(effective);
			break;
		default:
			break;
		}

		if (segment_ != Register::None) {
			std::optional<std::uint64_t> segment_base = get_register_value(segment_, static_cast<std::size_t>(0), static_cast<std::size_t>(0));
			if (!segment_base)
				return std::nullopt;
			effective += *segment_base;
		}

		return effective;
	}

	constexpr bool operator==(const UsedMemory& other) const noexcept {
		return displacement_ == other.displacement_ && segment_ == other.segment_ && base_ == other.base_ && index_ == other.index_ &&
			   scale_ == other.scale_ && memory_size_ == other.memory_size_ && access_ == other.access_ && address_size_ == other.address_size_ &&
			   vsib_size_ == other.vsib_size_;
	}
	constexpr bool operator!=(const UsedMemory& other) const noexcept { return !(*this == other); }

private:
	friend struct internal::InstructionInfoFactoryImpl;

	std::uint64_t displacement_;
	Register segment_;
	Register base_;
	Register index_;
	std::uint8_t scale_;
	MemorySize memory_size_;
	OpAccess access_;
	CodeSize address_size_;
	std::uint8_t vsib_size_;
};
static_assert(sizeof(UsedMemory) == 16, "UsedMemory should be 16 bytes");

/// Formats a `UsedRegister` (same format as Rust's `Debug` impl), eg. `RAX:Read`
std::string to_string(const UsedRegister& value);

/// Formats a `UsedMemory` (same format as Rust's `Debug` impl), eg. `[DS:RDI+R12*8+0xFFFFFFFFA55A1234;UInt32;ReadWrite]`
std::string to_string(const UsedMemory& value);

/// Contains information about an instruction, eg. read/written registers and memory locations, operand accesses.
/// Created by an `InstructionInfoFactory`.
class InstructionInfo {
public:
	/// Gets all accessed registers. This method doesn't return all accessed registers if `is_save_restore_instruction()` is `true`.
	///
	/// Some instructions have a `r16`/`r32` operand but only use the low 8 bits of the register. In that case
	/// this method returns the 8-bit register even if it's `SPL`, `BPL`, `SIL`, `DIL` and the
	/// instruction was decoded in 16 or 32-bit mode. This is more accurate than returning the `r16`/`r32`
	/// register. Example instructions that do this: `PINSRB`, `ARPL`
	const std::vector<UsedRegister>& used_registers() const noexcept { return used_registers_; }

	/// Gets all accessed memory locations
	const std::vector<UsedMemory>& used_memory() const noexcept { return used_memory_locations_; }

	/// Operand #0 access
	OpAccess op0_access() const noexcept { return op_accesses_[0]; }

	/// Operand #1 access
	OpAccess op1_access() const noexcept { return op_accesses_[1]; }

	/// Operand #2 access
	OpAccess op2_access() const noexcept { return op_accesses_[2]; }

	/// Operand #3 access
	OpAccess op3_access() const noexcept { return op_accesses_[3]; }

	/// Operand #4 access
	OpAccess op4_access() const noexcept { return op_accesses_[4]; }

	/// Gets operand access (`OpAccess::None` if `operand` is invalid, and debug builds abort)
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	OpAccess op_access(std::uint32_t operand) const noexcept;

	/// Gets operand access
	///
	/// # Errors
	///
	/// Fails if `operand` is invalid
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Result<OpAccess> try_op_access(std::uint32_t operand) const;

private:
	friend class InstructionInfoFactory;
	friend struct internal::InstructionInfoFactoryImpl;

	explicit InstructionInfo(std::uint32_t options);

	std::vector<UsedRegister> used_registers_;
	std::vector<UsedMemory> used_memory_locations_;
	std::array<OpAccess, IcedConstants::MAX_OP_COUNT> op_accesses_;
};

/// Instruction info options used by `InstructionInfoFactory`
struct InstructionInfoOptions {
	/// No option is enabled
	static constexpr std::uint32_t NONE = 0;
	/// Don't include memory usage, i.e., `InstructionInfo::used_memory()` will return an empty vector. All
	/// registers that are used by memory operands are still returned by `InstructionInfo::used_registers()`.
	static constexpr std::uint32_t NO_MEMORY_USAGE = 0x0000'0001;
	/// Don't include register usage, i.e., `InstructionInfo::used_registers()` will return an empty vector
	static constexpr std::uint32_t NO_REGISTER_USAGE = 0x0000'0002;
};

/// Creates `InstructionInfo`s.
///
/// The returned `InstructionInfo` reference is owned by the factory and it's only valid until the next
/// `info()`/`info_options()` call (its vectors are re-used to avoid allocations).
class InstructionInfoFactory {
public:
	/// Creates a new instance.
	///
	/// # Examples
	///
	/// ```cpp
	/// // add [rdi+r12*8-5AA5EDCCh],esi
	/// const std::uint8_t bytes[] = {0x42, 0x01, 0xB4, 0xE7, 0x34, 0x12, 0x5A, 0xA5};
	/// Decoder decoder(64, bytes, DecoderOptions::NONE);
	///
	/// // This allocates two vectors but they get re-used every time you call info() and info_options().
	/// InstructionInfoFactory info_factory;
	///
	/// while (decoder.can_decode()) {
	///     Instruction instr = decoder.decode();
	///     // There's also info_options() if you only need reg usage or only mem usage.
	///     // info() returns both.
	///     const InstructionInfo& info = info_factory.info(instr);
	///     for (const UsedMemory& mem_info : info.used_memory())
	///         std::printf("%s\n", to_string(mem_info).c_str());
	///     for (const UsedRegister& reg_info : info.used_registers())
	///         std::printf("%s\n", to_string(reg_info).c_str());
	/// }
	/// ```
	InstructionInfoFactory();

	/// Creates a new `InstructionInfo`, see also `info_options()` if you only need register usage
	/// but not memory usage or vice versa.
	///
	/// # Arguments
	///
	/// * `instruction`: The instruction that should be analyzed
	///
	/// # Examples
	///
	/// ```cpp
	/// // add [rdi+r12*8-5AA5EDCCh],esi
	/// const std::uint8_t bytes[] = {0x42, 0x01, 0xB4, 0xE7, 0x34, 0x12, 0x5A, 0xA5};
	/// Decoder decoder(64, bytes, DecoderOptions::NONE);
	/// InstructionInfoFactory info_factory;
	///
	/// Instruction instr = decoder.decode();
	/// const InstructionInfo& info = info_factory.info(instr);
	///
	/// assert(info.used_memory().size() == 1);
	/// UsedMemory mem = info.used_memory()[0];
	/// assert(mem.segment() == Register::DS);
	/// assert(mem.base() == Register::RDI);
	/// assert(mem.index() == Register::R12);
	/// assert(mem.scale() == 8);
	/// assert(mem.displacement() == 0xFFFFFFFFA55A1234);
	/// assert(mem.memory_size() == MemorySize::UInt32);
	/// assert(mem.access() == OpAccess::ReadWrite);
	///
	/// const auto& regs = info.used_registers();
	/// assert(regs.size() == 3);
	/// assert(regs[0].register_() == Register::RDI);
	/// assert(regs[0].access() == OpAccess::Read);
	/// assert(regs[1].register_() == Register::R12);
	/// assert(regs[1].access() == OpAccess::Read);
	/// assert(regs[2].register_() == Register::ESI);
	/// assert(regs[2].access() == OpAccess::Read);
	/// ```
	const InstructionInfo& info(const Instruction& instruction);

	/// Creates a new `InstructionInfo`, see also `info()`.
	///
	/// # Arguments
	///
	/// * `instruction`: The instruction that should be analyzed
	/// * `options`: Options, see `InstructionInfoOptions`
	const InstructionInfo& info_options(const Instruction& instruction, std::uint32_t options);

private:
	InstructionInfo info_;
};

} // namespace iced_x86

namespace std {

template <>
struct hash<iced_x86::UsedRegister> {
	std::size_t operator()(const iced_x86::UsedRegister& value) const noexcept {
		return (static_cast<std::size_t>(value.register_()) << 8) | static_cast<std::size_t>(value.access());
	}
};

template <>
struct hash<iced_x86::UsedMemory> {
	std::size_t operator()(const iced_x86::UsedMemory& value) const noexcept {
		std::uint64_t v = static_cast<std::uint64_t>(value.segment()) | (static_cast<std::uint64_t>(value.base()) << 8) |
						  (static_cast<std::uint64_t>(value.index()) << 16) | (static_cast<std::uint64_t>(value.scale()) << 24) |
						  (static_cast<std::uint64_t>(value.memory_size()) << 32) | (static_cast<std::uint64_t>(value.access()) << 40) |
						  (static_cast<std::uint64_t>(value.address_size()) << 48) | (static_cast<std::uint64_t>(value.vsib_size()) << 56);
		std::size_t h = std::hash<std::uint64_t>()(value.displacement());
		return h ^ (std::hash<std::uint64_t>()(v) + static_cast<std::size_t>(0x9E37'79B9U) + (h << 6) + (h >> 2));
	}
};

} // namespace std
