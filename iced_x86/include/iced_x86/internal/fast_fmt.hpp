// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// NOT PART OF THE PUBLIC API. Types and tables used by the `SpecializedFormatter<TraitOptions>` template
// (it's a template so it must be in a public header). The tables are constant data defined by the library.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "iced_x86/code.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"

// The fast formatter force-inlines its small helpers when optimizing (ICED_X86_INTERNAL_FORCE_INLINE)
#include "iced_x86/internal/macros.hpp"

namespace iced_x86::internal {
/// Address size in bytes (2, 4, 8) or 0 (index = `Register` value)
extern const std::uint8_t REG_TO_ADDR_SIZE[IcedConstants::REGISTER_ENUM_COUNT];
} // namespace iced_x86::internal

namespace iced_x86::internal::fast {

[[noreturn]] inline void fast_fmt_assert_failed() noexcept { std::abort(); }

/// Counts the leading zero bits. `value` must not be 0.
ICED_X86_INTERNAL_FORCE_INLINE std::uint32_t leading_zeros64(std::uint64_t value) noexcept {
#if defined(__GNUC__) || defined(__clang__)
	return static_cast<std::uint32_t>(__builtin_clzll(value));
#else
	std::uint32_t count = 0;
	if ((value & 0xFFFF'FFFF'0000'0000ULL) == 0) {
		count += 32;
		value <<= 32;
	}
	if ((value & 0xFFFF'0000'0000'0000ULL) == 0) {
		count += 16;
		value <<= 16;
	}
	if ((value & 0xFF00'0000'0000'0000ULL) == 0) {
		count += 8;
		value <<= 8;
	}
	if ((value & 0xF000'0000'0000'0000ULL) == 0) {
		count += 4;
		value <<= 4;
	}
	if ((value & 0xC000'0000'0000'0000ULL) == 0) {
		count += 2;
		value <<= 2;
	}
	if ((value & 0x8000'0000'0000'0000ULL) == 0)
		count += 1;
	return count;
#endif
}

/// A string with a length byte followed by `SIZE` readable bytes (only the first `len()` bytes are part of the string).
/// It's always possible to copy all `SIZE` bytes which is faster than copying exactly `len()` bytes.
template <std::size_t SIZE_>
struct FastString {
	static constexpr std::size_t SIZE = SIZE_;

	/// Points to the length byte followed by `SIZE` bytes
	const std::uint8_t* len_data;

	constexpr std::size_t len() const noexcept { return len_data[0]; }
	constexpr const std::uint8_t* utf8_data() const noexcept { return len_data + 1; }
};

// FastString2 isn't used since the code needs a 66h prefix (if target CPU is x86)
using FastString4 = FastString<4>;	 // ld 4
using FastString8 = FastString<8>;	 // ld 8
using FastString12 = FastString<12>; // ld 8 + ld 4
using FastString16 = FastString<16>; // ld 16
using FastString20 = FastString<20>; // ld 16 + ld 4

using FastStringMnemonic = FastString20;
using FastStringMemorySize = FastString16;
using FastStringRegister = FastString8;

/// Creates the data of a `FastString<SIZE>` (length byte + `SIZE` bytes padded with spaces) at compile time
template <std::size_t SIZE, std::size_t N>
constexpr std::array<std::uint8_t, 1 + SIZE> mk_fast_str_data(const char (&s)[N]) noexcept {
	static_assert(N - 1 <= SIZE, "String is too long");
	std::array<std::uint8_t, 1 + SIZE> result{};
	result[0] = static_cast<std::uint8_t>(N - 1);
	for (std::size_t i = 0; i < SIZE; i++)
		result[1 + i] = i < N - 1 ? static_cast<std::uint8_t>(s[i]) : static_cast<std::uint8_t>(' ');
	return result;
}

// Must be the same as the generated `MAX_MNEMONIC_LEN` (verified by the library)
constexpr std::size_t MAX_MNEMONIC_STRING_LEN = 18;

// full fmt'd str = "prefixes mnemonic op0<decorators1>, op1, op2, op3, op4<decorators2>"
// prefixes = "es xacquire xrelease lock notrack repe repne "
// mnemonic = "prefetch_exclusive"
// op sep = ", "
// op = "fpustate108 ptr fs:[rax+zmm31*8+0x12345678]"
//		- longest 'xxxx ptr' and longest memory operand
// op = "0x123456789ABCDEF0"
// op = "0x1234:0x12345678"
// op = "zmm31"
// op = "offset symbol (123456789ABCDEF0)"
//		- symbol can have any length
// <decorators1> = "{k3}{z}"
// <decorators2> = "{rn-sae}"
// symbol = any length + optional address " (123456789ABCDEF0)"
// full = "es xacquire xrelease lock notrack repe repne prefetch_exclusive fpustate108 ptr fs:[rax+zmm31*8+0x12345678]{k3}{z}, fpustate108 ptr fs:[rax+zmm31*8+0x12345678], fpustate108 ptr fs:[rax+zmm31*8+0x12345678], fpustate108 ptr fs:[rax+zmm31*8+0x12345678], fpustate108 ptr fs:[rax+zmm31*8+0x12345678]{rn-sae}"
//		- it's not possible to have 5 `fpustate108 ptr fs:[rax+zmm31*8+0x12345678]` operands
//		  so we'll never get a formatted string this long if there's no symbol resolver.
//
// It's also the size of the unchecked write area: the formatter writes whole `FastString`s (and 4 bytes when it writes 2 hex
// digits) so it can write up to 19 bytes past the end of the formatted text, but real formatted instructions are much shorter
// (see the comment above), so all writes of an instruction fit in `MAX_FMT_INSTR_LEN` bytes (same as Rust which reserves
// `MAX_FMT_INSTR_LEN` bytes). This is verified by the tests and by `verify_output_has_enough_bytes_left()` (enabled by default).
constexpr std::size_t MAX_FMT_INSTR_LEN = sizeof("es xacquire xrelease lock notrack repe repne ") - 1 + MAX_MNEMONIC_STRING_LEN +
										  sizeof("{k3}{z}{eh}") - 1 +
										  (IcedConstants::MAX_OP_COUNT * (2 /*", "*/ + sizeof("fpustate108 ptr fs:[rax+zmm31*8+0x12345678]") - 1)) -
										  1 /*','*/ + sizeof("{rn-sae}{float16}") - 1;
static_assert(MAX_FMT_INSTR_LEN == MAX_MNEMONIC_STRING_LEN + sizeof("es xacquire xrelease lock notrack repe repne  "
																	"fpustate108 ptr fs:[rax+zmm31*8+0x12345678]{k3}{z}{eh}, "
																	"fpustate108 ptr fs:[rax+zmm31*8+0x12345678], "
																	"fpustate108 ptr fs:[rax+zmm31*8+0x12345678], "
																	"fpustate108 ptr fs:[rax+zmm31*8+0x12345678], "
																	"fpustate108 ptr fs:[rax+zmm31*8+0x12345678]{rn-sae}{float16}") -
												 1,
			  "");
// Make sure it doesn't grow too much without us knowing about it (eg. if more operands are added)
static_assert(MAX_FMT_INSTR_LEN < 350, "");

// Must be the same as the generated `FastFmtFlags` values (verified by the library)
constexpr std::uint32_t FAST_FMT_FLAGS_FORCE_MEM_SIZE = 0x0000'0004;
constexpr std::uint32_t FAST_FMT_FLAGS_PSEUDO_OPS_KIND_SHIFT = 3;

// The fast formatter's tables. They're constant data (no heap, no startup code) defined by the library
// (generated: src/formatter/fast/{fmt_data,regs,mem_size_tbl}.cpp).

/// Register names (index = `Register` value): a length byte followed by `FastStringRegister::SIZE` chars
extern const std::uint8_t REGISTERS[IcedConstants::REGISTER_ENUM_COUNT][1 + FastStringRegister::SIZE];
/// All mnemonics: each mnemonic is a length byte followed by the chars. The last one is followed by padding so it's
/// possible to read `FastStringMnemonic::SIZE` bytes from any mnemonic.
extern const std::uint8_t MNEMONICS[];
/// Offset of each `Code`'s mnemonic in `MNEMONICS` (index = `Code` value)
extern const std::uint16_t MNEMONIC_OFFSETS[IcedConstants::CODE_ENUM_COUNT];
/// `FastFmtFlags` (index = `Code` value)
extern const std::uint8_t CODE_FLAGS[IcedConstants::CODE_ENUM_COUNT];
/// Memory size keywords (index = `MemorySize` value): a length byte followed by `FastStringMemorySize::SIZE` chars
extern const std::uint8_t MEMORY_SIZES[IcedConstants::MEMORY_SIZE_ENUM_COUNT][1 + FastStringMemorySize::SIZE];

/// Gets the pseudo op mnemonic if there's one.
///
/// # Arguments
///
/// - `code`: Code
/// - `pseudo_ops_num`: `FastFmtFlags` pseudo ops kind (`flags >> FAST_FMT_FLAGS_PSEUDO_OPS_KIND_SHIFT`), 1-based, must not be 0
/// - `imm8`: Immediate (`instruction.immediate8()`)
/// - `mnemonic`: Updated with the pseudo op mnemonic if it returns `true`
bool try_get_pseudo_op(Code code, std::uint32_t pseudo_ops_num, std::uint32_t imm8, FastStringMnemonic& mnemonic) noexcept;

/// Gets the MVEX register/memory conversion decorator (eg. `{cdab}`) or `nullptr` if there's none
const std::uint8_t* get_mvex_reg_mem_conv_string(Code code, MvexRegMemConv conv) noexcept;

/// The output of `SpecializedFormatter::format()` if it can't write directly to the caller's buffer (the buffer is smaller
/// than `MAX_FMT_INSTR_LEN + 1` bytes or a symbol resolver is used). The formatter writes to `scratch` (a buffer on the stack)
/// and copies the text to `output` when it's done or before it writes a symbol (symbols can be any length).
struct FastFmtOutput {
	/// The caller's buffer (can be null if `output_size` is 0)
	char* output;
	/// Size of `output` in bytes (incl. the terminating NUL char)
	std::size_t output_size;
	/// Length of the formatted text so far (can be > `output_size - 1` if the text is truncated)
	std::size_t length;
	/// Start of the scratch buffer (`MAX_FMT_INSTR_LEN + 1` bytes)
	std::uint8_t* scratch;
};

/// Appends `size` chars to the output. Chars that don't fit (the last byte is reserved for the NUL char) are dropped but
/// counted (`out.length` is always updated).
void fast_fmt_append(FastFmtOutput& out, const void* data, std::size_t size) noexcept;
/// Writes the terminating NUL char (if `output_size != 0`) and returns the full length of the formatted text
std::size_t fast_fmt_finish(FastFmtOutput& out) noexcept;

// Padding so we can read 4 bytes at every index 0-0xFF inclusive
inline constexpr char HEX_GROUP2_UPPER[0x200 + 2 + 1] =
	"000102030405060708090A0B0C0D0E0F"
	"101112131415161718191A1B1C1D1E1F"
	"202122232425262728292A2B2C2D2E2F"
	"303132333435363738393A3B3C3D3E3F"
	"404142434445464748494A4B4C4D4E4F"
	"505152535455565758595A5B5C5D5E5F"
	"606162636465666768696A6B6C6D6E6F"
	"707172737475767778797A7B7C7D7E7F"
	"808182838485868788898A8B8C8D8E8F"
	"909192939495969798999A9B9C9D9E9F"
	"A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
	"B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
	"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
	"D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
	"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
	"F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"
	"__";

inline constexpr char HEX_DIGITS_UPPER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
inline constexpr char HEX_DIGITS_LOWER[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

inline constexpr std::array<std::uint8_t, 1 + 4> SCALE_NUMBERS[4] = {
	mk_fast_str_data<4>("*1"),
	mk_fast_str_data<4>("*2"),
	mk_fast_str_data<4>("*4"),
	mk_fast_str_data<4>("*8"),
};

// Index = RoundingControl
inline constexpr std::array<std::uint8_t, 1 + 8> RC_SAE_STRINGS[IcedConstants::ROUNDING_CONTROL_ENUM_COUNT] = {
	mk_fast_str_data<8>(""),
	mk_fast_str_data<8>("{rn-sae}"),
	mk_fast_str_data<8>("{rd-sae}"),
	mk_fast_str_data<8>("{ru-sae}"),
	mk_fast_str_data<8>("{rz-sae}"),
};

// Index = RoundingControl
inline constexpr std::array<std::uint8_t, 1 + 4> RC_STRINGS[IcedConstants::ROUNDING_CONTROL_ENUM_COUNT] = {
	mk_fast_str_data<4>(""),
	mk_fast_str_data<4>("{rn}"),
	mk_fast_str_data<4>("{rd}"),
	mk_fast_str_data<4>("{ru}"),
	mk_fast_str_data<4>("{rz}"),
};

inline constexpr auto STR_XACQUIRE = mk_fast_str_data<12>("xacquire ");
inline constexpr auto STR_XRELEASE = mk_fast_str_data<12>("xrelease ");
inline constexpr auto STR_LOCK = mk_fast_str_data<8>("lock ");
inline constexpr auto STR_NOTRACK = mk_fast_str_data<8>("notrack ");
inline constexpr auto STR_REPE = mk_fast_str_data<8>("repe ");
inline constexpr auto STR_REP = mk_fast_str_data<4>("rep ");
inline constexpr auto STR_BND = mk_fast_str_data<4>("bnd ");
inline constexpr auto STR_REPNE = mk_fast_str_data<8>("repne ");
inline constexpr auto STR_OFFSET = mk_fast_str_data<8>("offset ");
inline constexpr auto STR_EH = mk_fast_str_data<4>("{eh}");
inline constexpr auto STR_Z = mk_fast_str_data<4>("{z}");
inline constexpr auto STR_COMMA_SPACE = mk_fast_str_data<4>(", ");
inline constexpr auto STR_SAE = mk_fast_str_data<8>("{sae}");
inline constexpr auto STR_HEX_PREFIX = mk_fast_str_data<4>("0x");
inline constexpr auto STR_SPACE_PAREN = mk_fast_str_data<4>(" (");

} // namespace iced_x86::internal::fast
