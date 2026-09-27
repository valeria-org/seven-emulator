// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "iced_x86/iced_constants.hpp"
#include "iced_x86/memory_size.hpp"

namespace iced_x86 {

/// `MemorySize` information
class MemorySizeInfo {
public:
	/// Constructor (used by the generated table)
	constexpr MemorySizeInfo(MemorySize memory_size, std::uint16_t size, std::uint16_t element_size, MemorySize element_type, bool is_signed,
		bool is_broadcast) noexcept
		: size_(size), element_size_(element_size), memory_size_(memory_size), element_type_(element_type), is_signed_(is_signed),
		  is_broadcast_(is_broadcast) {}

	/// Gets the `MemorySize` value
	constexpr MemorySize memory_size() const noexcept { return memory_size_; }

	/// Gets the size in bytes of the memory location or 0 if it's not accessed or unknown
	constexpr std::size_t size() const noexcept { return size_; }

	/// Gets the size in bytes of the packed element. If it's not a packed data type, it's equal to `size()`.
	constexpr std::size_t element_size() const noexcept { return element_size_; }

	/// Gets the element type if it's packed data or the type itself if it's not packed data
	constexpr MemorySize element_type() const noexcept { return element_type_; }

	/// Gets the element type if it's packed data or the type itself if it's not packed data
	inline const MemorySizeInfo& element_type_info() const noexcept;

	/// `true` if it's signed data (signed integer or a floating point value)
	constexpr bool is_signed() const noexcept { return is_signed_; }

	/// `true` if it's a broadcast memory type
	constexpr bool is_broadcast() const noexcept { return is_broadcast_; }

	/// `true` if this is a packed data type, eg. `MemorySize::Packed128_Float32`. See also `element_count()`
	constexpr bool is_packed() const noexcept { return element_size_ < size_; }

	/// Gets the number of elements in the packed data type or `1` if it's not packed data (`is_packed()`)
	constexpr std::size_t element_count() const noexcept {
		// element_size can be 0 so we don't divide by it if es == s
		return element_size_ == size_ ? 1 : static_cast<std::size_t>(size_) / static_cast<std::size_t>(element_size_);
	}

	/// Compares all fields
	constexpr bool operator==(const MemorySizeInfo& other) const noexcept {
		return size_ == other.size_ && element_size_ == other.element_size_ && memory_size_ == other.memory_size_ &&
			element_type_ == other.element_type_ && is_signed_ == other.is_signed_ && is_broadcast_ == other.is_broadcast_;
	}
	/// Compares all fields
	constexpr bool operator!=(const MemorySizeInfo& other) const noexcept { return !(*this == other); }

private:
	std::uint16_t size_;
	std::uint16_t element_size_;
	MemorySize memory_size_;
	MemorySize element_type_;
	// Use flags if more booleans are needed
	bool is_signed_;
	bool is_broadcast_;
};

namespace internal {
extern const MemorySizeInfo MEMORY_SIZE_INFOS[IcedConstants::MEMORY_SIZE_ENUM_COUNT];
} // namespace internal

/// `MemorySize` helper methods (Rust: `impl MemorySize`)
namespace memory_size_ext {

/// Gets the memory size info
inline const MemorySizeInfo& info(MemorySize memory_size) noexcept {
	return internal::MEMORY_SIZE_INFOS[static_cast<std::size_t>(memory_size)];
}

/// Gets the size in bytes of the memory location or 0 if it's not accessed by the instruction or unknown or variable sized
inline std::size_t size(MemorySize memory_size) noexcept { return info(memory_size).size(); }

/// Gets the size in bytes of the packed element. If it's not a packed data type, it's equal to `size()`.
inline std::size_t element_size(MemorySize memory_size) noexcept { return info(memory_size).element_size(); }

/// Gets the element type if it's packed data or `memory_size` if it's not packed data
inline MemorySize element_type(MemorySize memory_size) noexcept { return info(memory_size).element_type(); }

/// Gets the element type info if it's packed data or `memory_size` if it's not packed data
inline const MemorySizeInfo& element_type_info(MemorySize memory_size) noexcept { return info(info(memory_size).element_type()); }

/// `true` if it's signed data (signed integer or a floating point value)
inline bool is_signed(MemorySize memory_size) noexcept { return info(memory_size).is_signed(); }

/// `true` if this is a packed data type, eg. `MemorySize::Packed128_Float32`
inline bool is_packed(MemorySize memory_size) noexcept { return info(memory_size).is_packed(); }

/// Gets the number of elements in the packed data type or `1` if it's not packed data (`is_packed()`)
inline std::size_t element_count(MemorySize memory_size) noexcept { return info(memory_size).element_count(); }

/// Checks if it is a broadcast memory type
constexpr bool is_broadcast(MemorySize memory_size) noexcept { return memory_size >= IcedConstants::FIRST_BROADCAST_MEMORY_SIZE; }

} // namespace memory_size_ext

inline const MemorySizeInfo& MemorySizeInfo::element_type_info() const noexcept { return memory_size_ext::info(element_type_); }

} // namespace iced_x86

namespace std {
/// Hashes a `MemorySizeInfo`
template <>
struct hash<iced_x86::MemorySizeInfo> {
	std::size_t operator()(const iced_x86::MemorySizeInfo& info) const noexcept {
		std::size_t h = info.size();
		h = h * 31 + info.element_size();
		h = h * 31 + static_cast<std::size_t>(info.memory_size());
		h = h * 31 + static_cast<std::size_t>(info.element_type());
		h = h * 31 + (info.is_signed() ? 1 : 0);
		h = h * 31 + (info.is_broadcast() ? 1 : 0);
		return h;
	}
};
} // namespace std
