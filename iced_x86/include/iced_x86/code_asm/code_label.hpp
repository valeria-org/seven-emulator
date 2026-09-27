// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

namespace iced_x86::code_asm {

class CodeAssembler;
class CodeAssemblerResult;

/// A label created by `CodeAssembler::create_label()`. It can be referenced by branch instructions
/// (eg. `a.jne(label)`) and memory operands (eg. `a.lea(rax, ptr(label))`).
///
/// A default constructed label is invalid (empty).
class CodeLabel {
public:
	/// Creates an invalid (empty) label. Use `CodeAssembler::create_label()` to create a valid label.
	constexpr CodeLabel() noexcept : id_(0), instruction_index_(0) {}

	/// Gets the label id. It's unique per `CodeAssembler` instance. 0 is an invalid label id.
	[[nodiscard]] constexpr std::uint64_t id() const noexcept { return id_; }

	/// `true` if it's an invalid label (not created by `CodeAssembler::create_label()`)
	[[nodiscard]] constexpr bool is_empty() const noexcept { return id_ == 0; }

	/// `true` if the label has been emitted by `CodeAssembler::set_label()`
	[[nodiscard]] constexpr bool has_instruction_index() const noexcept { return instruction_index_ != NO_INDEX; }

	/// Labels are equal if they have the same id
	constexpr bool operator==(const CodeLabel& other) const noexcept { return id_ == other.id_; }
	/// Labels are equal if they have the same id
	constexpr bool operator!=(const CodeLabel& other) const noexcept { return id_ != other.id_; }

private:
	friend class CodeAssembler;
	friend class CodeAssemblerResult;

	static constexpr std::size_t NO_INDEX = std::numeric_limits<std::size_t>::max();

	explicit constexpr CodeLabel(std::uint64_t id) noexcept : id_(id), instruction_index_(NO_INDEX) {}

	std::uint64_t id_;
	std::size_t instruction_index_;
};

} // namespace iced_x86::code_asm

namespace std {
/// Hashes the label id (labels are equal if they have the same id)
template <>
struct hash<iced_x86::code_asm::CodeLabel> {
	std::size_t operator()(const iced_x86::code_asm::CodeLabel& label) const noexcept { return std::hash<std::uint64_t>()(label.id()); }
};
} // namespace std
