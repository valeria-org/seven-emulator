// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// The `FormatterOutput` used by the gas/intel/masm/nasm formatters' `format(const Instruction&, std::string&)`

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "iced_x86/formatter_output.hpp"
#include "internal/iced_assert.hpp"

// `append()` is force-inlined when optimizing (it's small and it's the hot path). When not optimizing (-O0), every
// inlined copy would get its own stack slots (big stack frames) so it's a normal function then.
#if (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#define ICED_BUFFERED_OUTPUT_INLINE inline __attribute__((always_inline))
#else
#define ICED_BUFFERED_OUTPUT_INLINE inline
#endif

namespace iced_x86::internal {

/// A char buffer owned by a formatter (used by `BufferedStringOutput`). It's allocated the first time
/// `format(const Instruction&, std::string&)` is called and it grows if it's too small (eg. long symbol names).
struct FormatterStringBuffer {
	static constexpr std::size_t INITIAL_CAPACITY = 256;

	std::unique_ptr<char[]> data;
	std::size_t capacity = 0;
};

/// Writes all text to the formatter's char buffer and `flush()` appends it to a `std::string` with one `append()` call.
///
/// Appending each piece of text (mnemonic, `,`, register, ...) directly to the `std::string` is slow since libstdc++'s
/// `append()` isn't inlined. The formatters are instantiated with this class (it's `final`) so all writes are inlined.
class BufferedStringOutput final : public FormatterOutput {
public:
	/// Creates an instance that writes to `buffer` (it's allocated if needed)
	explicit BufferedStringOutput(FormatterStringBuffer& buffer) : buffer_(&buffer) {
		if (ICED_UNLIKELY(!buffer.data))
			allocate(buffer);
		pos_ = buffer.data.get();
		end_ = pos_ + buffer.capacity;
	}

	BufferedStringOutput(const BufferedStringOutput&) = delete;
	BufferedStringOutput& operator=(const BufferedStringOutput&) = delete;

	/// Appends all written text to `output`
	void flush(std::string& output) {
		const char* start = buffer_->data.get();
		output.append(start, static_cast<std::size_t>(pos_ - start));
		pos_ = buffer_->data.get();
	}

	void write(std::string_view text, FormatterTextKind kind) override {
		static_cast<void>(kind);
		append(text);
	}

	void write_prefix(const Instruction& instruction, std::string_view text, PrefixKind prefix) override {
		static_cast<void>(instruction);
		static_cast<void>(prefix);
		append(text);
	}

	void write_mnemonic(const Instruction& instruction, std::string_view text) override {
		static_cast<void>(instruction);
		append(text);
	}

	void write_number(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
					  std::uint64_t value, NumberKind number_kind, FormatterTextKind kind) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(value);
		static_cast<void>(number_kind);
		static_cast<void>(kind);
		append(text);
	}

	void write_decorator(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
						 DecoratorKind decorator) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(decorator);
		append(text);
	}

	void write_register(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand, std::string_view text,
						Register register_) override {
		static_cast<void>(instruction);
		static_cast<void>(operand);
		static_cast<void>(instruction_operand);
		static_cast<void>(register_);
		append(text);
	}

private:
	ICED_BUFFERED_OUTPUT_INLINE void append(std::string_view text) {
		const std::size_t size = text.size();
		if (ICED_UNLIKELY(static_cast<std::size_t>(end_ - pos_) < size))
			grow(size);
		copy(pos_, text.data(), size);
		pos_ += size;
	}

	// Almost all strings are short (mnemonics, registers, numbers, punctuation). Copy them with 1-2 (overlapping) loads/stores
	// instead of calling the (non-inlined) `memcpy()`.
	ICED_BUFFERED_OUTPUT_INLINE static void copy(char* dst, const char* src, std::size_t size) noexcept {
		if (size <= 16) {
			if (size >= 8) {
				copy_fixed<8>(dst, src);
				copy_fixed<8>(dst + size - 8, src + size - 8);
			} else if (size >= 4) {
				copy_fixed<4>(dst, src);
				copy_fixed<4>(dst + size - 4, src + size - 4);
			} else if (size >= 2) {
				copy_fixed<2>(dst, src);
				copy_fixed<2>(dst + size - 2, src + size - 2);
			} else if (size != 0)
				*dst = *src;
		} else
			std::memcpy(dst, src, size);
	}

	template <std::size_t N>
	static void copy_fixed(char* dst, const char* src) noexcept {
		// Compilers convert this to one load + one store
		char tmp[N];
		std::memcpy(tmp, src, N);
		std::memcpy(dst, tmp, N);
	}

	// Allocates the buffer (the first time it's used)
	ICED_NOINLINE static void allocate(FormatterStringBuffer& buffer);
	// Makes room for at least `size` more chars
	ICED_NOINLINE void grow(std::size_t size);

	FormatterStringBuffer* buffer_;
	char* pos_;
	char* end_;
};

} // namespace iced_x86::internal
