// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>

#include "iced_x86/iced_constants.hpp"
#include "iced_x86/register.hpp"

namespace iced_x86 {

/// `Register` information
class RegisterInfo {
public:
	/// Constructor (used by the generated table)
	constexpr RegisterInfo(Register register_, Register base, Register full_register32, Register full_register, std::uint16_t size) noexcept
		: register__(register_), base_(base), full_register_(full_register), full_register32_(full_register32), size_(size) {}

	/// Gets the register
	constexpr Register register_() const noexcept { return register__; }

	/// Gets the base register, eg. `AL`, `AX`, `EAX`, `RAX`, `MM0`, `XMM0`, `YMM0`, `ZMM0`, `ES`
	constexpr Register base() const noexcept { return base_; }

	/// The register number (index) relative to `base()`, eg. 0-15, or 0-31, or if 8-bit GPR, 0-19
	constexpr std::size_t number() const noexcept {
		return static_cast<std::size_t>(register__) - static_cast<std::size_t>(base_);
	}

	/// The full register that this one is a part of, eg. `CL`/`CH`/`CX`/`ECX`/`RCX` -> `RCX`, `XMM11`/`YMM11`/`ZMM11` -> `ZMM11`
	constexpr Register full_register() const noexcept { return full_register_; }

	/// Gets the full register that this one is a part of, except if it's a GPR in which case the 32-bit register is returned,
	/// eg. `CL`/`CH`/`CX`/`ECX`/`RCX` -> `ECX`, `XMM11`/`YMM11`/`ZMM11` -> `ZMM11`
	constexpr Register full_register32() const noexcept { return full_register32_; }

	/// Size of the register in bytes
	constexpr std::size_t size() const noexcept { return size_; }

	/// Compares all fields
	constexpr bool operator==(const RegisterInfo& other) const noexcept {
		return register__ == other.register__ && base_ == other.base_ && full_register_ == other.full_register_ &&
			full_register32_ == other.full_register32_ && size_ == other.size_;
	}
	/// Compares all fields
	constexpr bool operator!=(const RegisterInfo& other) const noexcept { return !(*this == other); }

private:
	Register register__;
	Register base_;
	Register full_register_;
	Register full_register32_;
	std::uint16_t size_;
};

namespace internal {
extern const RegisterInfo REGISTER_INFOS[IcedConstants::REGISTER_ENUM_COUNT];
} // namespace internal

/// `Register` helper methods (Rust: `impl Register`)
namespace register_ext {

/// Gets register info
inline const RegisterInfo& info(Register register_) noexcept {
	return internal::REGISTER_INFOS[static_cast<std::size_t>(register_)];
}

/// Gets the base register, eg. `AL`, `AX`, `EAX`, `RAX`, `MM0`, `XMM0`, `YMM0`, `ZMM0`, `ES`
inline Register base(Register register_) noexcept { return info(register_).base(); }

/// The register number (index) relative to `base()`, eg. 0-15, or 0-31, or if 8-bit GPR, 0-19
inline std::size_t number(Register register_) noexcept { return info(register_).number(); }

/// Gets the full register that this one is a part of, eg. `CL`/`CH`/`CX`/`ECX`/`RCX` -> `RCX`, `XMM11`/`YMM11`/`ZMM11` -> `ZMM11`
inline Register full_register(Register register_) noexcept { return info(register_).full_register(); }

/// Gets the full register that this one is a part of, except if it's a GPR in which case the 32-bit register is returned,
/// eg. `CL`/`CH`/`CX`/`ECX`/`RCX` -> `ECX`, `XMM11`/`YMM11`/`ZMM11` -> `ZMM11`
inline Register full_register32(Register register_) noexcept { return info(register_).full_register32(); }

/// Gets the size of the register in bytes
inline std::size_t size(Register register_) noexcept { return info(register_).size(); }

/// Checks if it's a segment register (`ES`, `CS`, `SS`, `DS`, `FS`, `GS`)
constexpr bool is_segment_register(Register register_) noexcept { return Register::ES <= register_ && register_ <= Register::GS; }

/// Checks if it's a general purpose register (`AL`-`R15L`, `AX`-`R15W`, `EAX`-`R15D`, `RAX`-`R15`)
constexpr bool is_gpr(Register register_) noexcept { return Register::AL <= register_ && register_ <= Register::R15; }

/// Checks if it's an 8-bit general purpose register (`AL`-`R15L`)
constexpr bool is_gpr8(Register register_) noexcept { return Register::AL <= register_ && register_ <= Register::R15L; }

/// Checks if it's a 16-bit general purpose register (`AX`-`R15W`)
constexpr bool is_gpr16(Register register_) noexcept { return Register::AX <= register_ && register_ <= Register::R15W; }

/// Checks if it's a 32-bit general purpose register (`EAX`-`R15D`)
constexpr bool is_gpr32(Register register_) noexcept { return Register::EAX <= register_ && register_ <= Register::R15D; }

/// Checks if it's a 64-bit general purpose register (`RAX`-`R15`)
constexpr bool is_gpr64(Register register_) noexcept { return Register::RAX <= register_ && register_ <= Register::R15; }

/// Checks if it's a 128-bit vector register (`XMM0`-`XMM31`)
constexpr bool is_xmm(Register register_) noexcept { return Register::XMM0 <= register_ && register_ <= IcedConstants::XMM_LAST; }

/// Checks if it's a 256-bit vector register (`YMM0`-`YMM31`)
constexpr bool is_ymm(Register register_) noexcept { return Register::YMM0 <= register_ && register_ <= IcedConstants::YMM_LAST; }

/// Checks if it's a 512-bit vector register (`ZMM0`-`ZMM31`)
constexpr bool is_zmm(Register register_) noexcept { return Register::ZMM0 <= register_ && register_ <= IcedConstants::ZMM_LAST; }

/// Checks if it's an `XMM`, `YMM` or `ZMM` register
constexpr bool is_vector_register(Register register_) noexcept { return Register::XMM0 <= register_ && register_ <= IcedConstants::VMM_LAST; }

/// Checks if it's `EIP`/`RIP`
constexpr bool is_ip(Register register_) noexcept { return register_ == Register::EIP || register_ == Register::RIP; }

/// Checks if it's an opmask register (`K0`-`K7`)
constexpr bool is_k(Register register_) noexcept { return Register::K0 <= register_ && register_ <= Register::K7; }

/// Checks if it's a control register (`CR0`-`CR15`)
constexpr bool is_cr(Register register_) noexcept { return Register::CR0 <= register_ && register_ <= Register::CR15; }

/// Checks if it's a debug register (`DR0`-`DR15`)
constexpr bool is_dr(Register register_) noexcept { return Register::DR0 <= register_ && register_ <= Register::DR15; }

/// Checks if it's a test register (`TR0`-`TR7`)
constexpr bool is_tr(Register register_) noexcept { return Register::TR0 <= register_ && register_ <= Register::TR7; }

/// Checks if it's an FPU stack register (`ST0`-`ST7`)
constexpr bool is_st(Register register_) noexcept { return Register::ST0 <= register_ && register_ <= Register::ST7; }

/// Checks if it's a bound register (`BND0`-`BND3`)
constexpr bool is_bnd(Register register_) noexcept { return Register::BND0 <= register_ && register_ <= Register::BND3; }

/// Checks if it's an MMX register (`MM0`-`MM7`)
constexpr bool is_mm(Register register_) noexcept { return Register::MM0 <= register_ && register_ <= Register::MM7; }

/// Checks if it's a tile register (`TMM0`-`TMM7`)
constexpr bool is_tmm(Register register_) noexcept { return Register::TMM0 <= register_ && register_ <= IcedConstants::TMM_LAST; }

} // namespace register_ext

namespace internal {
// Rust: `Register::from_u8()`. All `u8` values are valid `Register` values.
constexpr Register register_from_u8(std::uint8_t value) noexcept {
	static_assert(IcedConstants::REGISTER_ENUM_COUNT >= 0x100, "");
	return static_cast<Register>(value);
}

inline Register register_add(Register register_, std::uint32_t rhs) noexcept {
	const std::uint32_t result = static_cast<std::uint32_t>(register_) + rhs;
	if (result >= IcedConstants::REGISTER_ENUM_COUNT)
		std::abort();
	return static_cast<Register>(result);
}

inline Register register_sub(Register register_, std::uint32_t rhs) noexcept {
	const std::uint32_t result = static_cast<std::uint32_t>(register_) - rhs;
	if (result >= IcedConstants::REGISTER_ENUM_COUNT)
		std::abort();
	return static_cast<Register>(result);
}
} // namespace internal

/// `Register + std::int32_t`. Aborts if the result isn't a valid `Register` value.
inline Register operator+(Register lhs, std::int32_t rhs) noexcept { return internal::register_add(lhs, static_cast<std::uint32_t>(rhs)); }
/// `Register + std::uint32_t`. Aborts if the result isn't a valid `Register` value.
inline Register operator+(Register lhs, std::uint32_t rhs) noexcept { return internal::register_add(lhs, rhs); }
/// `std::int32_t + Register`. Aborts if the result isn't a valid `Register` value.
inline Register operator+(std::int32_t lhs, Register rhs) noexcept { return internal::register_add(rhs, static_cast<std::uint32_t>(lhs)); }
/// `std::uint32_t + Register`. Aborts if the result isn't a valid `Register` value.
inline Register operator+(std::uint32_t lhs, Register rhs) noexcept { return internal::register_add(rhs, lhs); }
/// `Register += std::int32_t`. Aborts if the result isn't a valid `Register` value.
inline Register& operator+=(Register& lhs, std::int32_t rhs) noexcept { return lhs = internal::register_add(lhs, static_cast<std::uint32_t>(rhs)); }
/// `Register += std::uint32_t`. Aborts if the result isn't a valid `Register` value.
inline Register& operator+=(Register& lhs, std::uint32_t rhs) noexcept { return lhs = internal::register_add(lhs, rhs); }
/// `Register - std::int32_t`. Aborts if the result isn't a valid `Register` value.
inline Register operator-(Register lhs, std::int32_t rhs) noexcept { return internal::register_sub(lhs, static_cast<std::uint32_t>(rhs)); }
/// `Register - std::uint32_t`. Aborts if the result isn't a valid `Register` value.
inline Register operator-(Register lhs, std::uint32_t rhs) noexcept { return internal::register_sub(lhs, rhs); }
/// `Register -= std::int32_t`. Aborts if the result isn't a valid `Register` value.
inline Register& operator-=(Register& lhs, std::int32_t rhs) noexcept { return lhs = internal::register_sub(lhs, static_cast<std::uint32_t>(rhs)); }
/// `Register -= std::uint32_t`. Aborts if the result isn't a valid `Register` value.
inline Register& operator-=(Register& lhs, std::uint32_t rhs) noexcept { return lhs = internal::register_sub(lhs, rhs); }

} // namespace iced_x86

namespace std {
/// Hashes a `RegisterInfo`
template <>
struct hash<iced_x86::RegisterInfo> {
	std::size_t operator()(const iced_x86::RegisterInfo& info) const noexcept {
		std::size_t h = static_cast<std::size_t>(info.register_());
		h = h * 31 + static_cast<std::size_t>(info.base());
		h = h * 31 + static_cast<std::size_t>(info.full_register());
		h = h * 31 + static_cast<std::size_t>(info.full_register32());
		h = h * 31 + info.size();
		return h;
	}
};
} // namespace std
