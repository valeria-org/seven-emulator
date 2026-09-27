// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_asm/code_label.hpp"
#include "iced_x86/code_asm/memory_operand_size.hpp"
#include "iced_x86/code_asm/op_state.hpp"
#include "iced_x86/code_asm/reg.hpp"
#include "iced_x86/register.hpp"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace iced_x86::code_asm {

namespace internal {
// Integer types that can be used as displacements and scales (same as Rust: i8-i64, isize, u8-u64, usize)
template <typename T>
using EnableIfInteger = std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value, int>;
} // namespace internal

/// A memory operand passed to `CodeAssembler` methods.
///
/// You can create a memory operand in many different ways:
///
/// ```cpp
/// using namespace iced_x86::code_asm;
///
/// // <???> ptr [rax+rdx*4]
/// auto m1 = rax + rdx * 4;
/// // byte ptr [rax]
/// auto m2 = byte_ptr(rax);
/// // <???> ptr gs:[rcx*4+123]
/// auto m3 = ptr(rcx * 4 + 123).gs();
/// // qword bcst [rdx+xmm0*8+123]
/// auto m4 = qword_bcst(rdx + xmm0 * 8 + 123);
/// ```
///
/// Anytime you add something to a register (or subtract from it), you create a memory operand. You can also call
/// `ptr()`, `word_ptr()`, `dword_bcst()` etc to create memory operands.
class AsmMemoryOperand {
public:
	/// Creates a memory operand
	///
	/// @param base Base register or `Register::None`
	/// @param index Index register or `Register::None`
	/// @param scale Index register scale (1, 2, 4, or 8)
	/// @param displacement Displacement
	/// @param state Operand state (segment override, size hint, broadcast, opmask)
	constexpr AsmMemoryOperand(Register base, Register index, std::uint32_t scale, std::int64_t displacement, CodeAsmOpState state) noexcept
		: base_(base), index_(index), scale_(static_cast<std::uint8_t>(scale)), displ_(displacement), state_(state) {}

	/// Gets the base register or `Register::None`
	[[nodiscard]] constexpr Register base() const noexcept { return base_; }
	/// Gets the index register or `Register::None`
	[[nodiscard]] constexpr Register index() const noexcept { return index_; }
	/// Gets the index register scale (1, 2, 4, or 8)
	[[nodiscard]] constexpr std::uint32_t scale() const noexcept { return scale_; }
	/// Gets the displacement
	[[nodiscard]] constexpr std::int64_t displacement() const noexcept { return displ_; }
	/// Gets the operand state (segment override, size hint, broadcast, opmask)
	[[nodiscard]] constexpr CodeAsmOpState state() const noexcept { return state_; }
	/// Gets the memory size hint
	[[nodiscard]] constexpr MemoryOperandSize size() const noexcept { return state_.size(); }
	/// `true` if it's a broadcast memory operand
	[[nodiscard]] constexpr bool is_broadcast() const noexcept { return state_.is_broadcast(); }
	/// Gets the segment override or `Register::None`
	[[nodiscard]] constexpr Register segment() const noexcept { return state_.segment(); }
	/// `true` if there's no base and no index register
	[[nodiscard]] constexpr bool is_displacement_only() const noexcept { return base_ == Register::None && index_ == Register::None; }

	/// Returns a copy with a size hint (and no broadcast). Normally you call eg. `dword_ptr(mem)` instead.
	///
	/// @param size Size hint
	[[nodiscard]] constexpr AsmMemoryOperand with_ptr(MemoryOperandSize size) const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.ptr(size);
		return result;
	}

	/// Returns a broadcast copy with a size hint. Normally you call eg. `dword_bcst(mem)` instead.
	///
	/// @param size Size hint
	[[nodiscard]] constexpr AsmMemoryOperand with_bcst(MemoryOperandSize size) const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.bcst(size);
		return result;
	}

	/// Adds an `ES` segment override
	[[nodiscard]] constexpr AsmMemoryOperand es() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_es();
		return result;
	}

	/// Adds a `CS` segment override
	[[nodiscard]] constexpr AsmMemoryOperand cs() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_cs();
		return result;
	}

	/// Adds an `SS` segment override
	[[nodiscard]] constexpr AsmMemoryOperand ss() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_ss();
		return result;
	}

	/// Adds a `DS` segment override
	[[nodiscard]] constexpr AsmMemoryOperand ds() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_ds();
		return result;
	}

	/// Adds an `FS` segment override
	[[nodiscard]] constexpr AsmMemoryOperand fs() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_fs();
		return result;
	}

	/// Adds a `GS` segment override
	[[nodiscard]] constexpr AsmMemoryOperand gs() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_gs();
		return result;
	}

	/// Adds a `{k1}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k1() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k1();
		return result;
	}

	/// Adds a `{k2}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k2() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k2();
		return result;
	}

	/// Adds a `{k3}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k3() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k3();
		return result;
	}

	/// Adds a `{k4}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k4() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k4();
		return result;
	}

	/// Adds a `{k5}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k5() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k5();
		return result;
	}

	/// Adds a `{k6}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k6() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k6();
		return result;
	}

	/// Adds a `{k7}` opmask register
	[[nodiscard]] constexpr AsmMemoryOperand k7() const noexcept {
		AsmMemoryOperand result = *this;
		result.state_.set_k7();
		return result;
	}

	constexpr bool operator==(const AsmMemoryOperand& other) const noexcept {
		return base_ == other.base_ && index_ == other.index_ && scale_ == other.scale_ && displ_ == other.displ_ && state_ == other.state_;
	}
	constexpr bool operator!=(const AsmMemoryOperand& other) const noexcept { return !(*this == other); }

	/// `mem + reg`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmMemoryOperand mem, AsmRegister16 base) noexcept { return mem.add_base(base.register_()); }
	/// `mem + reg`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmMemoryOperand mem, AsmRegister32 base) noexcept { return mem.add_base(base.register_()); }
	/// `mem + reg`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmMemoryOperand mem, AsmRegister64 base) noexcept { return mem.add_base(base.register_()); }
	/// `reg + mem`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmRegister16 base, AsmMemoryOperand mem) noexcept { return mem.add_base(base.register_()); }
	/// `reg + mem`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmRegister32 base, AsmMemoryOperand mem) noexcept { return mem.add_base(base.register_()); }
	/// `reg + mem`: sets the base register (it must not have a base register)
	friend constexpr AsmMemoryOperand operator+(AsmRegister64 base, AsmMemoryOperand mem) noexcept { return mem.add_base(base.register_()); }

	/// `mem + displ`: adds a displacement
	template <typename T, internal::EnableIfInteger<T> = 0>
	friend constexpr AsmMemoryOperand operator+(AsmMemoryOperand mem, T displ) noexcept {
		mem.displ_ = wrapping_add(mem.displ_, static_cast<std::int64_t>(displ));
		return mem;
	}

	/// `displ + mem`: adds a displacement
	template <typename T, internal::EnableIfInteger<T> = 0>
	friend constexpr AsmMemoryOperand operator+(T displ, AsmMemoryOperand mem) noexcept {
		mem.displ_ = wrapping_add(mem.displ_, static_cast<std::int64_t>(displ));
		return mem;
	}

	/// `mem - displ`: subtracts a displacement
	template <typename T, internal::EnableIfInteger<T> = 0>
	friend constexpr AsmMemoryOperand operator-(AsmMemoryOperand mem, T displ) noexcept {
		mem.displ_ = static_cast<std::int64_t>(static_cast<std::uint64_t>(mem.displ_) - static_cast<std::uint64_t>(static_cast<std::int64_t>(displ)));
		return mem;
	}

	/// `mem + mem`: combines two memory operands. The base and index registers must not be set in both operands.
	///
	/// The remaining fields aren't copied/updated from the right operand, eg. segment register, size, etc.
	friend constexpr AsmMemoryOperand operator+(AsmMemoryOperand left, AsmMemoryOperand right) noexcept {
		if (left.base_ == Register::None)
			left.base_ = right.base_;
		else
			assert(right.base_ == Register::None);
		if (left.index_ == Register::None) {
			left.index_ = right.index_;
			left.scale_ = right.scale_;
		}
		else
			assert(right.index_ == Register::None);
		left.displ_ = wrapping_add(left.displ_, right.displ_);
		return left;
	}

private:
	static constexpr std::int64_t wrapping_add(std::int64_t a, std::int64_t b) noexcept {
		return static_cast<std::int64_t>(static_cast<std::uint64_t>(a) + static_cast<std::uint64_t>(b));
	}

	constexpr AsmMemoryOperand add_base(Register base) const noexcept {
		assert(base_ == Register::None);
		AsmMemoryOperand result = *this;
		result.base_ = base;
		return result;
	}

	Register base_;
	Register index_;
	std::uint8_t scale_;
	std::int64_t displ_;
	CodeAsmOpState state_;
};

/// Anything that can be converted to a memory operand by `ptr()`, `byte_ptr()`, ..., `dword_bcst()`, ...:
/// a memory operand (`rax + rcx * 4`), a register (the base register), a label (RIP-relative memory) or an
/// integer (the displacement).
class AsmMemoryOperandArg {
public:
	/// Memory operand
	constexpr AsmMemoryOperandArg(const AsmMemoryOperand& mem) noexcept : mem_(mem) {}
	/// Base register
	constexpr AsmMemoryOperandArg(AsmRegister16 base) noexcept : mem_(base.register_(), Register::None, 1, 0, CodeAsmOpState()) {}
	/// Base register
	constexpr AsmMemoryOperandArg(AsmRegister32 base) noexcept : mem_(base.register_(), Register::None, 1, 0, CodeAsmOpState()) {}
	/// Base register
	constexpr AsmMemoryOperandArg(AsmRegister64 base) noexcept : mem_(base.register_(), Register::None, 1, 0, CodeAsmOpState()) {}
	/// Base register
	constexpr AsmMemoryOperandArg(Register base) noexcept : mem_(base, Register::None, 1, 0, CodeAsmOpState()) {}
	/// RIP-relative memory operand referencing a label
	constexpr AsmMemoryOperandArg(CodeLabel label) noexcept
		: mem_(Register::RIP, Register::None, 1, static_cast<std::int64_t>(label.id()), CodeAsmOpState()) {}
	/// Displacement (absolute address)
	template <typename T, internal::EnableIfInteger<T> = 0>
	constexpr AsmMemoryOperandArg(T displacement) noexcept
		: mem_(Register::None, Register::None, 1, static_cast<std::int64_t>(displacement), CodeAsmOpState()) {}

	/// Gets the memory operand
	[[nodiscard]] constexpr const AsmMemoryOperand& value() const noexcept { return mem_; }

private:
	AsmMemoryOperand mem_;
};

namespace internal {
constexpr AsmMemoryOperand base_displ(Register base, std::int64_t displ) noexcept {
	return AsmMemoryOperand(base, Register::None, 1, displ, CodeAsmOpState());
}
constexpr AsmMemoryOperand index_scale(Register index, std::uint32_t scale) noexcept {
	return AsmMemoryOperand(Register::None, index, scale, 0, CodeAsmOpState());
}
constexpr AsmMemoryOperand base_index(Register base, Register index) noexcept {
	return AsmMemoryOperand(base, index, 1, 0, CodeAsmOpState());
}
} // namespace internal

// reg + displ, displ + reg, reg - displ

/// `reg + displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(AsmRegister16 base, T displ) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `displ + reg`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(T displ, AsmRegister16 base) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `reg - displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator-(AsmRegister16 base, T displ) noexcept {
	return internal::base_displ(base.register_(), static_cast<std::int64_t>(0 - static_cast<std::uint64_t>(static_cast<std::int64_t>(displ))));
}
/// `reg + displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(AsmRegister32 base, T displ) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `displ + reg`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(T displ, AsmRegister32 base) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `reg - displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator-(AsmRegister32 base, T displ) noexcept {
	return internal::base_displ(base.register_(), static_cast<std::int64_t>(0 - static_cast<std::uint64_t>(static_cast<std::int64_t>(displ))));
}
/// `reg + displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(AsmRegister64 base, T displ) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `displ + reg`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator+(T displ, AsmRegister64 base) noexcept { return internal::base_displ(base.register_(), static_cast<std::int64_t>(displ)); }
/// `reg - displ`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator-(AsmRegister64 base, T displ) noexcept {
	return internal::base_displ(base.register_(), static_cast<std::int64_t>(0 - static_cast<std::uint64_t>(static_cast<std::int64_t>(displ))));
}

// reg * scale, scale * reg

/// `index * scale`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(AsmRegister32 index, T scale) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `scale * index`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(T scale, AsmRegister32 index) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `index * scale`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(AsmRegister64 index, T scale) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `scale * index`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(T scale, AsmRegister64 index) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `index * scale`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(AsmRegisterXmm index, T scale) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `scale * index`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(T scale, AsmRegisterXmm index) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `index * scale`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(AsmRegisterYmm index, T scale) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `scale * index`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(T scale, AsmRegisterYmm index) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `index * scale`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(AsmRegisterZmm index, T scale) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }
/// `scale * index`
template <typename T, internal::EnableIfInteger<T> = 0>
constexpr AsmMemoryOperand operator*(T scale, AsmRegisterZmm index) noexcept { return internal::index_scale(index.register_(), static_cast<std::uint8_t>(scale)); }

// base + index

/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister16 base, AsmRegister16 index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister32 base, AsmRegister32 index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister32 base, AsmRegisterXmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister32 base, AsmRegisterYmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister32 base, AsmRegisterZmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister64 base, AsmRegister64 index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister64 base, AsmRegisterXmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister64 base, AsmRegisterYmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `base + index`
constexpr AsmMemoryOperand operator+(AsmRegister64 base, AsmRegisterZmm index) noexcept { return internal::base_index(base.register_(), index.register_()); }

// Special case `vec + reg` and treat `vec` as the index register (it can't be the base)

/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterXmm index, AsmRegister32 base) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterYmm index, AsmRegister32 base) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterZmm index, AsmRegister32 base) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterXmm index, AsmRegister64 base) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterYmm index, AsmRegister64 base) noexcept { return internal::base_index(base.register_(), index.register_()); }
/// `index + base` (a vector register is always the index register)
constexpr AsmMemoryOperand operator+(AsmRegisterZmm index, AsmRegister64 base) noexcept { return internal::base_index(base.register_(), index.register_()); }

} // namespace iced_x86::code_asm
