// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "iced_x86/block_encoder_options.hpp"
#include "iced_x86/constant_offsets.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/reloc_kind.hpp"

namespace iced_x86 {

/// Relocation info
struct RelocInfo {
	/// Address
	std::uint64_t address;

	/// Relocation kind
	RelocKind kind;

	/// Constructor
	///
	/// # Arguments
	///
	/// * `kind_`: Relocation kind
	/// * `address_`: Address
	constexpr RelocInfo(RelocKind kind_, std::uint64_t address_) noexcept : address(address_), kind(kind_) {}

	friend constexpr bool operator==(const RelocInfo& a, const RelocInfo& b) noexcept { return a.address == b.address && a.kind == b.kind; }
	friend constexpr bool operator!=(const RelocInfo& a, const RelocInfo& b) noexcept { return !(a == b); }
};

/// Contains a slice of instructions that should be encoded by `BlockEncoder`
///
/// It doesn't own the instructions, they must be alive until `BlockEncoder::encode()` / `BlockEncoder::encode_slice()` returns.
class InstructionBlock {
public:
	/// Constructor
	///
	/// # Arguments
	///
	/// * `instructions`: All instructions
	/// * `count`: Number of instructions in `instructions`
	/// * `rip`: Base IP of all encoded instructions
	constexpr InstructionBlock(const Instruction* instructions, std::size_t count, std::uint64_t rip) noexcept
		: instructions_(instructions), count_(count), rip_(rip) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// * `instructions`: All instructions
	/// * `rip`: Base IP of all encoded instructions
	InstructionBlock(const std::vector<Instruction>& instructions, std::uint64_t rip) noexcept
		: instructions_(instructions.data()), count_(instructions.size()), rip_(rip) {}
	// It doesn't own the instructions so a temporary vector isn't allowed
	InstructionBlock(std::vector<Instruction>&& instructions, std::uint64_t rip) = delete;

	/// Gets the instructions
	constexpr const Instruction* instructions() const noexcept { return instructions_; }
	/// Gets the number of instructions
	constexpr std::size_t count() const noexcept { return count_; }
	/// Gets the base IP of all encoded instructions
	constexpr std::uint64_t rip() const noexcept { return rip_; }

private:
	const Instruction* instructions_;
	std::size_t count_;
	std::uint64_t rip_;
};

/// `BlockEncoder` result if it was successful
struct BlockEncoderResult {
	/// Base IP of all encoded instructions
	std::uint64_t rip = 0;

	/// The bytes of all encoded instructions
	std::vector<std::uint8_t> code_buffer;

	/// If `BlockEncoderOptions::RETURN_RELOC_INFOS` option was enabled:
	///
	/// All `RelocInfo`s.
	std::vector<RelocInfo> reloc_infos;

	/// If `BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS` option was enabled:
	///
	/// Offsets of the instructions relative to the base IP. If the instruction was rewritten to a new instruction
	/// (eg. `JE TARGET_TOO_FAR_AWAY` -> `JNE SHORT SKIP ; JMP QWORD PTR [MEM]`), the value `UINT32_MAX` is stored in that element.
	std::vector<std::uint32_t> new_instruction_offsets;

	/// If `BlockEncoderOptions::RETURN_CONSTANT_OFFSETS` option was enabled:
	///
	/// Offsets of all constants in the new encoded instructions. If the instruction was rewritten,
	/// the default (`ConstantOffsets{}`) value is stored in the corresponding element.
	std::vector<ConstantOffsets> constant_offsets;
};

/// Encodes instructions. It can be used to move instructions from one location to another location.
class BlockEncoder {
public:
	BlockEncoder() = delete;

	/// Encodes instructions. Any number of branches can be part of this block.
	/// You can use this function to move instructions from one location to another location.
	/// If the target of a branch is too far away, it'll be rewritten to a longer branch.
	/// You can disable this by passing in `BlockEncoderOptions::DONT_FIX_BRANCHES`.
	/// If the block has any `RIP`-relative memory operands, make sure the data isn't too
	/// far away from the new location of the encoded instructions. Every OS should have
	/// some API to allocate memory close (+/-2GB) to the original code location.
	///
	/// # Errors
	///
	/// Returns an error if it failed to encode one or more instructions.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32, or 64
	/// * `block`: All instructions
	/// * `options`: Encoder options, see `BlockEncoderOptions`
	///
	/// # Examples
	///
	/// ```
	/// // je short $-2
	/// // add dh,cl
	/// // sbb r9d,ebx
	/// const std::uint8_t bytes[] = {0x75, 0xFC, 0x00, 0xCE, 0x41, 0x19, 0xD9};
	/// std::vector<Instruction> instructions;
	/// Decoder decoder = Decoder::with_ip(64, bytes, 0x1234'5678'9ABC'DEF0, DecoderOptions::NONE);
	/// while (decoder.can_decode())
	///     instructions.push_back(decoder.decode());
	///
	/// // orig_rip + 8
	/// InstructionBlock block(instructions, 0x1234'5678'9ABC'DEF8);
	/// auto result = BlockEncoder::encode(64, block, BlockEncoderOptions::NONE);
	/// if (result.is_err()) {
	///     // Failed: result.error().message()
	/// }
	/// // result.value().code_buffer == {0x75, 0xF4, 0x00, 0xCE, 0x41, 0x19, 0xD9}
	/// ```
	static Result<BlockEncoderResult> encode(std::uint32_t bitness, const InstructionBlock& block, std::uint32_t options);

	/// Encodes instructions. Any number of branches can be part of this block.
	/// You can use this function to move instructions from one location to another location.
	/// If the target of a branch is too far away, it'll be rewritten to a longer branch.
	/// You can disable this by passing in `BlockEncoderOptions::DONT_FIX_BRANCHES`.
	/// If the block has any `RIP`-relative memory operands, make sure the data isn't too
	/// far away from the new location of the encoded instructions. Every OS should have
	/// some API to allocate memory close (+/-2GB) to the original code location.
	///
	/// The returned results are sorted by the blocks' `rip` (lowest address first).
	///
	/// # Errors
	///
	/// Returns an error if it failed to encode one or more instructions.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32, or 64
	/// * `blocks`: All instructions
	/// * `count`: Number of blocks in `blocks`
	/// * `options`: Encoder options, see `BlockEncoderOptions`
	///
	/// # Examples
	///
	/// ```
	/// // je short $-2
	/// // add dh,cl
	/// // sbb r9d,ebx
	/// const std::uint8_t bytes1[] = {0x75, 0xFC, 0x00, 0xCE, 0x41, 0x19, 0xD9};
	/// std::vector<Instruction> instructions1 = ...; // decoded with ip = 0x1234'5678'9ABC'DEF0
	///
	/// // je short $
	/// const std::uint8_t bytes2[] = {0x75, 0xFE};
	/// std::vector<Instruction> instructions2 = ...; // decoded with ip = 0x1234'5678
	///
	/// const InstructionBlock blocks[] = {
	///     // orig_rip + 8
	///     InstructionBlock(instructions1, 0x1234'5678'9ABC'DEF8),
	///     // a new ip
	///     InstructionBlock(instructions2, 0x8000'4000'2000'1000),
	/// };
	/// auto result = BlockEncoder::encode_slice(64, blocks, 2, BlockEncoderOptions::NONE);
	/// // result.value().size() == 2
	/// // result.value()[0].code_buffer == {0x75, 0xF4, 0x00, 0xCE, 0x41, 0x19, 0xD9}
	/// // result.value()[1].code_buffer == {0x75, 0xFE}
	/// ```
	static Result<std::vector<BlockEncoderResult>> encode_slice(std::uint32_t bitness, const InstructionBlock* blocks, std::size_t count, std::uint32_t options);

	/// Encodes instructions. Same as `encode_slice(bitness, blocks.data(), blocks.size(), options)`, see that method for more info.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32, or 64
	/// * `blocks`: All instructions
	/// * `options`: Encoder options, see `BlockEncoderOptions`
	static Result<std::vector<BlockEncoderResult>> encode_slice(std::uint32_t bitness, const std::vector<InstructionBlock>& blocks, std::uint32_t options) {
		return encode_slice(bitness, blocks.data(), blocks.size(), options);
	}
};

} // namespace iced_x86

namespace std {
template <>
struct hash<iced_x86::RelocInfo> {
	std::size_t operator()(const iced_x86::RelocInfo& value) const noexcept {
		return std::hash<std::uint64_t>()(value.address) ^ (static_cast<std::size_t>(value.kind) * 0x9E37'79B9U);
	}
};
} // namespace std
