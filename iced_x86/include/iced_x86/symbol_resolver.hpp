// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iced_x86/formatter_text_kind.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/symbol_flags.hpp"

namespace iced_x86 {

class Instruction;

/// Contains a borrowed string (`std::string_view`, Rust: `SymResString::Str`) or an owned string (`std::string`, Rust: `SymResString::String`)
class SymResString {
public:
	/// Creates an empty borrowed string
	SymResString() noexcept : is_string_(false), str_() {}
	/// Creates a borrowed string. The string must outlive this instance (or until the formatter has used it).
	SymResString(std::string_view str) noexcept : is_string_(false), str_(str) {}
	/// Creates a borrowed string. The string must outlive this instance (or until the formatter has used it).
	SymResString(const char* str) noexcept : is_string_(false), str_(str) {}
	/// Creates an owned string
	explicit SymResString(std::string string) noexcept : is_string_(true), str_(), string_(std::move(string)) {}

	/// `true` if it's an owned string (`std::string`), `false` if it's a borrowed string (`std::string_view`)
	bool is_string() const noexcept { return is_string_; }

	/// Gets the string
	std::string_view as_str() const noexcept { return is_string_ ? std::string_view(string_) : str_; }

	/// Creates an owned copy of this string
	SymResString to_owned() const { return SymResString(std::string(as_str())); }

	/// Compares the strings (and whether it's a borrowed or an owned string)
	bool operator==(const SymResString& other) const noexcept { return is_string_ == other.is_string_ && as_str() == other.as_str(); }
	/// Compares the strings (and whether it's a borrowed or an owned string)
	bool operator!=(const SymResString& other) const noexcept { return !(*this == other); }

private:
	bool is_string_;
	std::string_view str_;
	std::string string_;
};

/// Contains text and colors
struct SymResTextPart {
	/// Text
	SymResString text;
	/// Color
	FormatterTextKind color = FormatterTextKind::Text;

	/// Creates an empty text part
	SymResTextPart() noexcept = default;

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text (borrowed, it must outlive this instance)
	/// - `color`: Color
	SymResTextPart(std::string_view text_, FormatterTextKind color_) noexcept : text(text_), color(color_) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text (borrowed, it must outlive this instance)
	/// - `color`: Color
	SymResTextPart(const char* text_, FormatterTextKind color_) noexcept : text(text_), color(color_) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text
	/// - `color`: Color
	SymResTextPart(SymResString text_, FormatterTextKind color_) noexcept : text(std::move(text_)), color(color_) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text (owned)
	/// - `color`: Color
	static SymResTextPart with_string(std::string text, FormatterTextKind color) noexcept { return SymResTextPart(SymResString(std::move(text)), color); }

	/// Creates an owned copy (all strings are copied)
	SymResTextPart to_owned() const { return SymResTextPart(text.to_owned(), color); }

	/// Compares the text parts
	bool operator==(const SymResTextPart& other) const noexcept { return text == other.text && color == other.color; }
	/// Compares the text parts
	bool operator!=(const SymResTextPart& other) const noexcept { return !(*this == other); }
};

/// Contains one or more `SymResTextPart`s (text and color)
///
/// It's either one text part (Rust: `SymResTextInfo::Text`) or a borrowed array of text parts (Rust: `SymResTextInfo::TextVec`).
class SymResTextInfo {
public:
	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text (borrowed)
	/// - `color`: Color
	SymResTextInfo(std::string_view text, FormatterTextKind color) noexcept : text_(text, color), vec_(nullptr), vec_len_(0), is_vec_(false) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text (owned)
	/// - `color`: Color
	static SymResTextInfo with_string(std::string text, FormatterTextKind color) noexcept {
		return with_text(SymResTextPart::with_string(std::move(text), color));
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text
	static SymResTextInfo with_text(SymResTextPart text) noexcept { return SymResTextInfo(std::move(text)); }

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text parts (borrowed, the array must outlive this instance)
	/// - `count`: Number of text parts
	static SymResTextInfo with_vec(const SymResTextPart* text, std::size_t count) noexcept { return SymResTextInfo(text, count); }

	/// Constructor
	///
	/// # Arguments
	///
	/// - `text`: Text parts (borrowed, the vector must outlive this instance and must not be modified)
	static SymResTextInfo with_vec(const std::vector<SymResTextPart>& text) noexcept { return SymResTextInfo(text.data(), text.size()); }

	/// `true` if it's a (borrowed) array of text parts (Rust: `SymResTextInfo::TextVec`), `false` if it's one text part (Rust: `SymResTextInfo::Text`)
	bool is_text_vec() const noexcept { return is_vec_; }

	/// Gets the text part. Only valid if `is_text_vec()` is `false`
	const SymResTextPart& text() const noexcept { return text_; }

	/// Gets the number of text parts (1 if `is_text_vec()` is `false`)
	std::size_t size() const noexcept { return is_vec_ ? vec_len_ : 1; }

	/// Gets the first text part
	const SymResTextPart* begin() const noexcept { return is_vec_ ? vec_ : &text_; }

	/// Gets the end of the text parts
	const SymResTextPart* end() const noexcept { return begin() + size(); }

	/// Gets a text part
	const SymResTextPart& operator[](std::size_t index) const noexcept { return begin()[index]; }

	/// Creates an owned copy. The text parts of a text vector are copied to `vec` (which must outlive the result)
	SymResTextInfo to_owned(std::vector<SymResTextPart>& vec) const {
		if (!is_vec_)
			return SymResTextInfo(text_.to_owned());
		vec.clear();
		vec.reserve(vec_len_);
		for (std::size_t i = 0; i < vec_len_; i++)
			vec.push_back(vec_[i].to_owned());
		return SymResTextInfo(vec.data(), vec.size());
	}

	/// Compares the text parts
	bool operator==(const SymResTextInfo& other) const noexcept {
		if (is_vec_ != other.is_vec_ || size() != other.size())
			return false;
		for (std::size_t i = 0; i < size(); i++) {
			if ((*this)[i] != other[i])
				return false;
		}
		return true;
	}
	/// Compares the text parts
	bool operator!=(const SymResTextInfo& other) const noexcept { return !(*this == other); }

private:
	explicit SymResTextInfo(SymResTextPart text) noexcept : text_(std::move(text)), vec_(nullptr), vec_len_(0), is_vec_(false) {}
	SymResTextInfo(const SymResTextPart* vec, std::size_t len) noexcept : text_(), vec_(vec), vec_len_(len), is_vec_(true) {}

	SymResTextPart text_;
	const SymResTextPart* vec_;
	std::size_t vec_len_;
	bool is_vec_;
};

/// Created by a `SymbolResolver`
struct SymbolResult {
	/// The address of the symbol
	std::uint64_t address;

	/// Contains the symbol
	SymResTextInfo text;

	/// Symbol flags, see `SymbolFlags`
	std::uint32_t flags;

	/// Symbol size or `std::nullopt`
	std::optional<MemorySize> symbol_size;

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol
	/// - `flags`: Symbol flags, see `SymbolFlags`
	/// - `size`: Symbol size or `std::nullopt`
	SymbolResult(std::uint64_t address_, SymResTextInfo text_, std::uint32_t flags_ = SymbolFlags::NONE,
				 std::optional<MemorySize> symbol_size_ = std::nullopt) noexcept
		: address(address_), text(std::move(text_)), flags(flags_), symbol_size(symbol_size_) {}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (borrowed)
	static SymbolResult with_str(std::uint64_t address, std::string_view text) noexcept {
		return SymbolResult(address, SymResTextInfo(text, DEFAULT_KIND));
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (borrowed)
	/// - `size`: Symbol size
	static SymbolResult with_str_size(std::uint64_t address, std::string_view text, MemorySize size) noexcept {
		return SymbolResult(address, SymResTextInfo(text, DEFAULT_KIND), SymbolFlags::NONE, size);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (borrowed)
	/// - `color`: Color
	static SymbolResult with_str_kind(std::uint64_t address, std::string_view text, FormatterTextKind color) noexcept {
		return SymbolResult(address, SymResTextInfo(text, color));
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (borrowed)
	/// - `color`: Color
	/// - `flags`: Symbol flags, see `SymbolFlags`
	static SymbolResult with_str_kind_flags(std::uint64_t address, std::string_view text, FormatterTextKind color, std::uint32_t flags) noexcept {
		return SymbolResult(address, SymResTextInfo(text, color), flags);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (owned)
	static SymbolResult with_string(std::uint64_t address, std::string text) noexcept {
		return SymbolResult(address, SymResTextInfo::with_string(std::move(text), DEFAULT_KIND));
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (owned)
	/// - `size`: Symbol size
	static SymbolResult with_string_size(std::uint64_t address, std::string text, MemorySize size) noexcept {
		return SymbolResult(address, SymResTextInfo::with_string(std::move(text), DEFAULT_KIND), SymbolFlags::NONE, size);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (owned)
	/// - `color`: Color
	static SymbolResult with_string_kind(std::uint64_t address, std::string text, FormatterTextKind color) noexcept {
		return SymbolResult(address, SymResTextInfo::with_string(std::move(text), color));
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol (owned)
	/// - `color`: Color
	/// - `flags`: Symbol flags, see `SymbolFlags`
	static SymbolResult with_string_kind_flags(std::uint64_t address, std::string text, FormatterTextKind color, std::uint32_t flags) noexcept {
		return SymbolResult(address, SymResTextInfo::with_string(std::move(text), color), flags);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol
	static SymbolResult with_text(std::uint64_t address, SymResTextInfo text) noexcept { return SymbolResult(address, std::move(text)); }

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol
	/// - `size`: Symbol size
	static SymbolResult with_text_size(std::uint64_t address, SymResTextInfo text, MemorySize size) noexcept {
		return SymbolResult(address, std::move(text), SymbolFlags::NONE, size);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol
	/// - `flags`: Symbol flags, see `SymbolFlags`
	static SymbolResult with_text_flags(std::uint64_t address, SymResTextInfo text, std::uint32_t flags) noexcept {
		return SymbolResult(address, std::move(text), flags);
	}

	/// Constructor
	///
	/// # Arguments
	///
	/// - `address`: The address of the symbol
	/// - `text`: Symbol
	/// - `flags`: Symbol flags, see `SymbolFlags`
	/// - `size`: Symbol size
	static SymbolResult with_text_flags_size(std::uint64_t address, SymResTextInfo text, std::uint32_t flags, MemorySize size) noexcept {
		return SymbolResult(address, std::move(text), flags, size);
	}

	/// Creates an owned copy (all strings are copied). The text parts of a text vector are copied to `vec` (which must outlive the result)
	SymbolResult to_owned(std::vector<SymResTextPart>& vec) const { return SymbolResult(address, text.to_owned(vec), flags, symbol_size); }

	/// Compares the symbol results
	bool operator==(const SymbolResult& other) const noexcept {
		return address == other.address && text == other.text && flags == other.flags && symbol_size == other.symbol_size;
	}
	/// Compares the symbol results
	bool operator!=(const SymbolResult& other) const noexcept { return !(*this == other); }

private:
	static constexpr FormatterTextKind DEFAULT_KIND = FormatterTextKind::Label;
};

/// Used by a `Formatter` to resolve symbols
class SymbolResolver {
public:
	virtual ~SymbolResolver() = default;

	/// Tries to resolve a symbol
	///
	/// Borrowed strings in the returned `SymbolResult` must stay valid until this method is called again
	/// (or until the formatter returns).
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `operand`: Operand number, 0-based. This is a formatter operand and isn't necessarily the same as an instruction operand.
	/// - `instruction_operand`: Instruction operand number, 0-based, or `std::nullopt` if it's an operand created by the formatter.
	/// - `address`: Address
	/// - `address_size`: Size of `address` in bytes (eg. 1, 2, 4 or 8)
	virtual std::optional<SymbolResult> symbol(const Instruction& instruction, std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
											   std::uint64_t address, std::uint32_t address_size) = 0;
};

} // namespace iced_x86
