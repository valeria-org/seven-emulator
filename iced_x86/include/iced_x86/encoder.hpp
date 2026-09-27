// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/constant_offsets.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/memory_operand.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iced_x86 {

namespace internal {
struct EncOpCodeHandler;
struct EncoderInternal;
enum class DisplSize : std::uint8_t;
enum class ImmSize : std::uint8_t;
} // namespace internal

/// Encodes instructions decoded by the decoder or instructions created by other code.
/// See also `BlockEncoder` which can encode any number of instructions.
///
/// ```cpp
/// // xchg ah,[rdx+rsi+16h]
/// const std::uint8_t bytes[] = {0x86, 0x64, 0x32, 0x16};
/// Decoder decoder(64, bytes, DecoderOptions::NONE);
/// decoder.set_ip(0x1234'5678);
/// Instruction instr = decoder.decode();
///
/// Encoder encoder(64);
/// auto result = encoder.encode(instr, 0x5555'5555);
/// // result.value() == 4
/// // We're done, take ownership of the buffer
/// std::vector<std::uint8_t> buffer = encoder.take_buffer();
/// // buffer == {0x86, 0x64, 0x32, 0x16}
/// ```
class Encoder {
public:
	/// Creates an encoder
	///
	/// # Panics
	///
	/// Aborts if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	explicit Encoder(std::uint32_t bitness) noexcept;

	/// Creates an encoder (same as the constructor)
	///
	/// # Panics
	///
	/// Aborts if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	static Encoder new_(std::uint32_t bitness) noexcept { return Encoder(bitness); }

	/// Creates an encoder
	///
	/// # Errors
	///
	/// Fails if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	static Result<Encoder> try_new(std::uint32_t bitness);

	/// Creates an encoder with an initial buffer capacity
	///
	/// # Errors
	///
	/// Fails if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `capacity`: Initial capacity of the `uint8_t` buffer
	static Result<Encoder> try_with_capacity(std::uint32_t bitness, std::size_t capacity);

	/// Encodes an instruction and returns the size of the encoded instruction
	///
	/// # Errors
	///
	/// Returns an error if it failed to encode the instruction.
	///
	/// # Arguments
	///
	/// * `instruction`: Instruction to encode
	/// * `rip`: `RIP` of the encoded instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// // je short $+4
	/// const std::uint8_t bytes[] = {0x75, 0x02};
	/// Decoder decoder(64, bytes, DecoderOptions::NONE);
	/// decoder.set_ip(0x1234'5678);
	/// Instruction instr = decoder.decode();
	///
	/// Encoder encoder(64);
	/// // Use a different IP (orig rip + 0x10)
	/// auto result = encoder.encode(instr, 0x1234'5688);
	/// // result.value() == 2
	/// // encoder.take_buffer() == {0x75, 0xF2}
	/// ```
	Result<std::size_t> encode(const Instruction& instruction, std::uint64_t rip);

	/// Writes a byte to the output buffer
	///
	/// # Arguments
	///
	/// `value`: Value to write
	///
	/// # Examples
	///
	/// ```cpp
	/// Encoder encoder(64);
	/// Instruction instr = Instruction::with2(Code::Add_r64_rm64, Register::R8, Register::RBP).value();
	/// encoder.write_u8(0x90);
	/// auto result = encoder.encode(instr, 0x5555'5555);
	/// // result.value() == 3
	/// encoder.write_u8(0xCC);
	/// // encoder.take_buffer() == {0x90, 0x4C, 0x03, 0xC5, 0xCC}
	/// ```
	void write_u8(std::uint8_t value) {
		buffer_.push_back(value);
		current_rip_++;
	}

	/// Gets the buffer with all encoded instructions
	const std::vector<std::uint8_t>& buffer() const noexcept { return buffer_; }

	/// Returns the buffer and initializes the internal buffer to an empty vector. Should be called when
	/// you've encoded all instructions and need the raw instruction bytes. See also `set_buffer()`.
	std::vector<std::uint8_t> take_buffer() noexcept {
		std::vector<std::uint8_t> result = std::move(buffer_);
		buffer_ = std::vector<std::uint8_t>();
		return result;
	}

	/// Overwrites the buffer with a new vector. The old buffer is dropped. See also `take_buffer()`.
	void set_buffer(std::vector<std::uint8_t> buffer) noexcept { buffer_ = std::move(buffer); }

	/// Gets the offsets of the constants (memory displacement and immediate) in the encoded instruction.
	/// The caller can use this information to add relocations if needed.
	ConstantOffsets get_constant_offsets() const noexcept;

	/// Disables 2-byte VEX encoding and encodes all VEX instructions with the 3-byte VEX encoding
	bool prevent_vex2() const noexcept { return prevent_vex2_ != 0; }

	/// Disables 2-byte VEX encoding and encodes all VEX instructions with the 3-byte VEX encoding
	///
	/// # Arguments
	///
	/// * `new_value`: new value
	void set_prevent_vex2(bool new_value) noexcept { prevent_vex2_ = new_value ? 0xFFFF'FFFFU : 0; }

	/// Value of the `VEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	std::uint32_t vex_wig() const noexcept { return (internal_vex_wig_lig_ >> 7) & 1; }

	/// Value of the `VEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	///
	/// # Arguments
	///
	/// * `new_value`: new value (0 or 1)
	void set_vex_wig(std::uint32_t new_value) noexcept { internal_vex_wig_lig_ = (internal_vex_wig_lig_ & ~0x80U) | ((new_value & 1) << 7); }

	/// Value of the `VEX.L` bit to use if it's an instruction that ignores the bit. Default is 0.
	std::uint32_t vex_lig() const noexcept { return (internal_vex_wig_lig_ >> 2) & 1; }

	/// Value of the `VEX.L` bit to use if it's an instruction that ignores the bit. Default is 0.
	///
	/// # Arguments
	///
	/// * `new_value`: new value (0 or 1)
	void set_vex_lig(std::uint32_t new_value) noexcept {
		internal_vex_wig_lig_ = (internal_vex_wig_lig_ & ~4U) | ((new_value & 1) << 2);
		internal_vex_lig_ = (new_value & 1) << 2;
	}

	/// Value of the `EVEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	std::uint32_t evex_wig() const noexcept { return internal_evex_wig_ >> 7; }

	/// Value of the `EVEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	///
	/// # Arguments
	///
	/// * `new_value`: new value (0 or 1)
	void set_evex_wig(std::uint32_t new_value) noexcept { internal_evex_wig_ = (new_value & 1) << 7; }

	/// Value of the `EVEX.L'L` bits to use if it's an instruction that ignores the bits. Default is 0.
	std::uint32_t evex_lig() const noexcept { return internal_evex_lig_ >> 5; }

	/// Value of the `EVEX.L'L` bits to use if it's an instruction that ignores the bits. Default is 0.
	///
	/// # Arguments
	///
	/// * `new_value`: new value (0 or 3)
	void set_evex_lig(std::uint32_t new_value) noexcept { internal_evex_lig_ = (new_value & 3) << 5; }

	/// Value of the `MVEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	std::uint32_t mvex_wig() const noexcept { return internal_mvex_wig_ >> 7; }

	/// Value of the `MVEX.W` bit to use if it's an instruction that ignores the bit. Default is 0.
	///
	/// # Arguments
	///
	/// * `new_value`: new value (0 or 1)
	void set_mvex_wig(std::uint32_t new_value) noexcept { internal_mvex_wig_ = (new_value & 1) << 7; }

	/// Gets the bitness (16, 32 or 64)
	std::uint32_t bitness() const noexcept { return bitness_; }

private:
	struct PrivateTag {};
	Encoder(PrivateTag, std::uint32_t bitness, std::size_t capacity);

	friend struct internal::EncoderInternal;

	std::uint64_t current_rip_;
	std::vector<std::uint8_t> buffer_;
	const internal::EncOpCodeHandler* handler_;
	std::string error_message_;
	std::uint32_t bitness_;
	std::uint32_t eip_;
	std::uint32_t displ_addr_;
	std::uint32_t imm_addr_;
	std::uint32_t immediate_;
	// high 32 bits if it's a 64-bit immediate
	// high 32 bits if it's an IP relative immediate (jcc,call target)
	// high 32 bits if it's a 64-bit absolute address
	std::uint32_t immediate_hi_;
	std::uint32_t displ_;
	// high 32 bits if it's an IP relative mem displ (target)
	std::uint32_t displ_hi_;
	std::uint32_t op_code_;
	std::uint32_t internal_vex_wig_lig_;
	std::uint32_t internal_vex_lig_;
	std::uint32_t internal_evex_wig_;
	std::uint32_t internal_evex_lig_;
	std::uint32_t internal_mvex_wig_;
	std::uint32_t prevent_vex2_;
	std::uint32_t opsize16_flags_;
	std::uint32_t opsize32_flags_;
	std::uint32_t adrsize16_flags_;
	std::uint32_t adrsize32_flags_;
	// ***************************
	// These fields must be 64-bit aligned.
	// They are cleared in encode() and should be close so the compiler can optimize clearing them.
	std::uint32_t encoder_flags_; // EncoderFlags
	internal::DisplSize displ_size_;
	internal::ImmSize imm_size_;
	std::uint8_t mod_rm_;
	std::uint8_t sib_;
	// ***************************
};

} // namespace iced_x86
