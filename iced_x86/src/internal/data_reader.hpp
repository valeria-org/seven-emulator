// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

// Rust: data_reader.rs. Reads the serialized tables.
class DataReader {
public:
	constexpr DataReader(const std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size), index_(0) {}
	template <std::size_t N>
	constexpr explicit DataReader(const std::uint8_t (&data)[N]) noexcept : data_(data), size_(N), index_(0) {}

	std::size_t index() const noexcept { return index_; }

	void set_index(std::size_t index) noexcept { index_ = index; }

	std::size_t len_left() const noexcept { return size_ - index_; }

	bool can_read() const noexcept { return index_ < size_; }

	std::size_t read_u8() noexcept {
		ICED_ASSERT(index_ < size_);
		return data_[index_++];
	}

	std::uint32_t read_compressed_u32() noexcept {
		std::uint32_t result = 0;
		std::uint32_t shift = 0;
		for (;;) {
			ICED_DEBUG_ASSERT(shift < 32);

			const auto b = static_cast<std::uint32_t>(read_u8());
			if ((b & 0x80) == 0)
				return result | (b << shift);
			result |= (b & 0x7F) << shift;

			shift += 7;
		}
	}

	// The returned string points into the data (which is static data)
	std::string_view read_ascii_str() noexcept {
		const std::size_t len = read_u8();
		ICED_ASSERT(len <= size_ - index_);
		const std::string_view s(reinterpret_cast<const char*>(data_ + index_), len);
		index_ += len;
		return s;
	}

	// Returns the whole slice starting at the current index,
	// including the length byte, and advances the index by the current length + 1.
	// `out_size` is the size of the returned data (all bytes from the current index to the end of the data).
	const std::uint8_t* read_len_data(std::size_t& out_size) noexcept {
		ICED_ASSERT(index_ < size_);
		const std::size_t len = data_[index_];
		ICED_DEBUG_ASSERT(index_ + 1 + len <= size_);
		const std::uint8_t* len_data = data_ + index_;
		out_size = size_ - index_;
		index_ += 1 + len;
		return len_data;
	}

private:
	const std::uint8_t* data_;
	std::size_t size_;
	std::size_t index_;
};

} // namespace iced_x86::internal
