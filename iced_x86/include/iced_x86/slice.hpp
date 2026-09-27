// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>

namespace iced_x86 {

/// A non-owning read-only view of a contiguous array (Rust: `&[T]`), eg. the result of `code_ext::cpuid_features()`.
///
/// It's used where Rust returns a `&'static [T]` and C++17's `std::span` isn't available.
template <typename T>
class Slice {
public:
	/// Element type
	using value_type = T;
	/// Iterator type
	using const_iterator = const T*;
	/// Iterator type
	using iterator = const T*;

	/// Creates an empty slice
	constexpr Slice() noexcept : data_(nullptr), size_(0) {}
	/// Creates a slice
	///
	/// # Arguments
	///
	/// * `data`: Data (can be `nullptr` if `size` is 0)
	/// * `size`: Number of elements
	constexpr Slice(const T* data, std::size_t size) noexcept : data_(data), size_(size) {}
	/// Creates a slice from an array
	template <std::size_t N>
	constexpr Slice(const T (&data)[N]) noexcept : data_(data), size_(N) {}

	/// Gets a pointer to the first element
	constexpr const T* data() const noexcept { return data_; }
	/// Gets the number of elements
	constexpr std::size_t size() const noexcept { return size_; }
	/// `true` if it's empty
	constexpr bool empty() const noexcept { return size_ == 0; }
	/// Gets an element. `index` must be less than `size()`.
	constexpr const T& operator[](std::size_t index) const noexcept { return data_[index]; }
	/// Gets the first element iterator
	constexpr const T* begin() const noexcept { return data_; }
	/// Gets the end iterator
	constexpr const T* end() const noexcept { return data_ + size_; }

private:
	const T* data_;
	std::size_t size_;
};

} // namespace iced_x86
