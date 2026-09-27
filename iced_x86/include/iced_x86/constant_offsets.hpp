// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace iced_x86 {

class Decoder;
class Encoder;

/// Contains the offsets of the displacement and immediate. Call `Decoder::get_constant_offsets()` or
/// `Encoder::get_constant_offsets()` to get the offsets of the constants after the instruction has been
/// decoded/encoded.
class ConstantOffsets {
public:
	/// Creates an instance with all fields cleared (no displacement or immediates)
	constexpr ConstantOffsets() noexcept = default;

	/// Creates an instance (used by the decoder and encoder)
	///
	/// # Arguments
	///
	/// * `displacement_offset`: The offset of the displacement, if any
	/// * `displacement_size`: Size in bytes of the displacement, or 0 if there's no displacement
	/// * `immediate_offset`: The offset of the first immediate, if any
	/// * `immediate_size`: Size in bytes of the first immediate, or 0 if there's no immediate
	/// * `immediate_offset2`: The offset of the second immediate, if any
	/// * `immediate_size2`: Size in bytes of the second immediate, or 0 if there's no second immediate
	constexpr ConstantOffsets(std::uint8_t displacement_offset, std::uint8_t displacement_size, std::uint8_t immediate_offset, std::uint8_t immediate_size,
		std::uint8_t immediate_offset2, std::uint8_t immediate_size2) noexcept
		: displacement_offset_(displacement_offset), displacement_size_(displacement_size), immediate_offset_(immediate_offset),
		  immediate_size_(immediate_size), immediate_offset2_(immediate_offset2), immediate_size2_(immediate_size2) {}

	/// The offset of the displacement, if any
	constexpr std::size_t displacement_offset() const noexcept { return displacement_offset_; }

	/// Size in bytes of the displacement, or 0 if there's no displacement
	constexpr std::size_t displacement_size() const noexcept { return displacement_size_; }

	/// The offset of the first immediate, if any.
	///
	/// This field can be invalid even if the operand has an immediate if it's an immediate that isn't part
	/// of the instruction stream, eg. `SHL AL,1`.
	constexpr std::size_t immediate_offset() const noexcept { return immediate_offset_; }

	/// Size in bytes of the first immediate, or 0 if there's no immediate
	constexpr std::size_t immediate_size() const noexcept { return immediate_size_; }

	/// The offset of the second immediate, if any.
	constexpr std::size_t immediate_offset2() const noexcept { return immediate_offset2_; }

	/// Size in bytes of the second immediate, or 0 if there's no second immediate
	constexpr std::size_t immediate_size2() const noexcept { return immediate_size2_; }

	/// `true` if `displacement_offset()` and `displacement_size()` are valid
	constexpr bool has_displacement() const noexcept { return displacement_size_ != 0; }

	/// `true` if `immediate_offset()` and `immediate_size()` are valid
	constexpr bool has_immediate() const noexcept { return immediate_size_ != 0; }

	/// `true` if `immediate_offset2()` and `immediate_size2()` are valid
	constexpr bool has_immediate2() const noexcept { return immediate_size2_ != 0; }

	/// Compares all fields
	constexpr bool operator==(const ConstantOffsets& other) const noexcept {
		return displacement_offset_ == other.displacement_offset_ && displacement_size_ == other.displacement_size_ &&
			immediate_offset_ == other.immediate_offset_ && immediate_size_ == other.immediate_size_ &&
			immediate_offset2_ == other.immediate_offset2_ && immediate_size2_ == other.immediate_size2_ && pad1_ == other.pad1_ && pad2_ == other.pad2_;
	}
	/// Compares all fields
	constexpr bool operator!=(const ConstantOffsets& other) const noexcept { return !(*this == other); }

private:
	// The fields are written by the decoder and encoder
	friend class Decoder;
	friend class Encoder;

	std::uint8_t displacement_offset_ = 0;
	std::uint8_t displacement_size_ = 0;
	std::uint8_t immediate_offset_ = 0;
	std::uint8_t immediate_size_ = 0;
	std::uint8_t immediate_offset2_ = 0;
	std::uint8_t immediate_size2_ = 0;
	std::uint8_t pad1_ = 0;
	std::uint8_t pad2_ = 0;
};

} // namespace iced_x86

namespace std {
/// Hashes a `ConstantOffsets`
template <>
struct hash<iced_x86::ConstantOffsets> {
	std::size_t operator()(const iced_x86::ConstantOffsets& value) const noexcept {
		std::size_t h = value.displacement_offset();
		h = h * 31 + value.displacement_size();
		h = h * 31 + value.immediate_offset();
		h = h * 31 + value.immediate_size();
		h = h * 31 + value.immediate_offset2();
		h = h * 31 + value.immediate_size2();
		return h;
	}
};
} // namespace std
