// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_size.hpp"
#include "iced_x86/constant_offsets.hpp"
#include "iced_x86/decoder_error.hpp"
#include "iced_x86/decoder_options.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/internal/macros.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/tuple_type.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <vector>

namespace iced_x86 {

class Decoder;

namespace internal {

struct OpCodeHandler;
class DecoderCore;

// Same as Rust's `OpCodeHandlerDecodeFn`. The first arg is always the handler itself.
using OpCodeHandlerDecodeFn = void (*)(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept;

// All handlers derive from this struct. Rust stores `(decode_fn, &handler)` tuples in the tables (`HandlerEntry`); here the
// decode fn is also the first field of the handler.
struct OpCodeHandler {
	OpCodeHandlerDecodeFn decode;
	bool has_modrm;

	constexpr OpCodeHandler(OpCodeHandlerDecodeFn decode_fn, bool has_modrm_) noexcept : decode(decode_fn), has_modrm(has_modrm_) {}
};

// A handler and its decode fn (same as Rust's `(OpCodeHandlerDecodeFn, &'static OpCodeHandler)` tuples). The decode fn is
// stored next to the handler pointer (it's also `handler->decode`) so the indirect call target doesn't depend on a load
// from the handler.
struct HandlerEntry {
	OpCodeHandlerDecodeFn decode;
	const OpCodeHandler* handler;
};

// Opaque declarations of the generated internal enums (src/internal/decoder/op_size.hpp, src/internal/vector_length.hpp)
enum class OpSize : std::uint8_t;
enum class VectorLength : std::uint32_t;

// This is `std::uint32_t` since we need the decoder field near other fields that also get cleared in `decode_out()`.
enum class DecoderMandatoryPrefix : std::uint32_t {
	PNP = 0,
	P66 = 1,
	PF3 = 2,
	PF2 = 3,
};

struct DecoderState {
	std::uint32_t modrm = 0; // 0-0xFF
	std::uint32_t mod_ = 0;  // 0-3
	std::uint32_t reg = 0;   // 0-7
	std::uint32_t rm = 0;    // 0-7

	// ***************************
	// These fields are cleared in decode_out() and should be close so the compiler can optimize clearing them.
	std::uint32_t extra_register_base = 0;       // R << 3
	std::uint32_t extra_index_register_base = 0; // X << 3
	std::uint32_t extra_base_register_base = 0;  // B << 3
	std::uint32_t extra_index_register_base_vsib = 0;
	std::uint32_t flags = 0; // StateFlags
	DecoderMandatoryPrefix mandatory_prefix = DecoderMandatoryPrefix::PNP;

	std::uint32_t vvvv = 0;               // V`vvvv. Not stored in inverted form. If 16/32-bit mode, bits [4:3] are cleared
	std::uint32_t vvvv_invalid_check = 0; // vvvv bits, even in 16/32-bit mode.
	// ***************************
	std::uint32_t mem_index = 0; // (mod << 3 | rm) and an index into the mem handler tables if mod <= 2
	VectorLength vector_length{};
	std::uint32_t aaa = 0;
	std::uint32_t extra_register_base_evex = 0;      // EVEX/MVEX.R' << 4
	std::uint32_t extra_base_register_base_evex = 0; // EVEX/MVEX.XB << 3
	// The order of these 4 fields is important. They're accessed as a u32 (decode_out()) so should be 4 byte aligned.
	OpSize address_size{};
	OpSize operand_size{};
	std::uint8_t segment_prio = 0; // 0=ES/CS/SS/DS, 1=FS/GS
	std::uint8_t dummy = 0;
	// =================

	// Only used by debug asserts (it's always `EncodingKind::Legacy` in release builds)
	inline std::uint32_t encoding() const noexcept;
	inline std::uint32_t sss() const noexcept;
};

// The decoder implementation. The handlers (`src/decoder/handlers*.cpp`) get a reference to this class.
// Don't use it, use `Decoder` instead.
class DecoderCore {
	friend class iced_x86::Decoder;

public:
	static constexpr std::size_t MAX_READ_SIZE = 8;
	// Copies of the `StateFlags` (src/internal/decoder/state_flags.hpp) used by the inline code in this header
	static constexpr std::uint32_t SF_IP_REL64 = 0x0000'0001;
	static constexpr std::uint32_t SF_IP_REL32 = 0x0000'0002;
	static constexpr std::uint32_t SF_HAS_REX = 0x0000'0008;
	static constexpr std::uint32_t SF_IS_INVALID = 0x0000'0040;
	static constexpr std::uint32_t SF_W = 0x0000'0080;
	static constexpr std::uint32_t SF_LOCK = 0x0000'1000;
	static constexpr std::uint32_t SF_NO_MORE_BYTES = 0x0000'4000;
	// `OpSize::Size64`
	static constexpr std::uint8_t OP_SIZE64 = 2;

	// The hot fields are first so most accesses (`this` + offset) only need an 8-bit displacement

	DecoderState state;

	// Next bytes to read if there's enough bytes left to read.
	// This can be 1 byte past the last byte of `data`.
	// Invariant: data <= data_ptr <= max_data_ptr <= data + data_len == data_ptr_end
	// Invariant: {data_ptr,max_data_ptr,data_ptr_end} + max(MAX_READ_SIZE, MAX_INSTRUCTION_LENGTH) doesn't overflow
	std::uintptr_t data_ptr;
	// Set to min(data_ptr + IcedConstants::MAX_INSTRUCTION_LENGTH, data_ptr_end) and is guaranteed to not overflow
	// Initialized in decode_out() to at most 15 bytes after data_ptr so read_uXX() fails quickly after at most 15 read bytes
	// (1MB prefixes won't cause it to read 1MB prefixes, it will stop after at most 15).
	std::uintptr_t max_data_ptr;
	const HandlerEntry* handlers_map0;

	// DecoderOptions
	std::uint32_t options;
	// All 1s if we should check for invalid instructions, else 0
	std::uint32_t invalid_check_mask;
	// StateFlags::W if 64-bit mode, 0 if 16/32-bit mode
	std::uint32_t is64b_mode_and_w;
	// 7 in 16/32-bit mode, 15 in 64-bit mode
	std::uint32_t reg15_mask;
	// 0 in 16/32-bit mode, 0E0h in 64-bit mode
	std::uint32_t mask_e0;
	std::uint32_t rex_mask;
	// The order of these 4 fields is important. They're accessed as a u32 (decode_out()) so should be 4 byte aligned.
	OpSize default_address_size;
	OpSize default_operand_size;
	std::uint8_t segment_prio; // Always 0
	std::uint8_t dummy;        // Padding so we can read 4 bytes, see decode_out()
	// =================
	OpSize default_inverted_address_size;
	OpSize default_inverted_operand_size;
	// true if 64-bit mode, false if 16/32-bit mode
	bool is64b_mode;
	CodeSize default_code_size;
	// Offset of displacement in the instruction. Only used by get_constant_offsets() to return the offset of the displ
	std::uint8_t displ_index;
	std::uint32_t bitness;

	// Current RIP value
	std::uint64_t ip;
	// This is `data + data_len` (1 byte past the last valid byte).
	// This is guaranteed to be >= data_ptr (see the ctor), in other words, it can't overflow to 0
	std::uintptr_t data_ptr_end;
	// Initialized to start of data (data_ptr) when decode_out() is called. Used to calculate current IP/offset (when decoding) if needed.
	std::uintptr_t instr_start_data_ptr;

	// MAP0 is only used by MVEX
	const HandlerEntry* handlers_vex_map0;
	const HandlerEntry* handlers_vex[3];
	const HandlerEntry* handlers_evex[6];
	const HandlerEntry* handlers_xop[3];
	const HandlerEntry* handlers_mvex[3];

	// Input data provided by the user. When there's no more bytes left to read we'll return a NoMoreBytes error
	const std::uint8_t* data;
	std::size_t data_len;

	// The following methods are defined in src/internal/decoder/decoder_core.hpp (hot inline methods) and src/decoder/decoder.cpp

	inline std::size_t read_u8() noexcept;
	inline std::size_t read_u16() noexcept;
	inline std::size_t read_u32() noexcept;
	inline std::uint64_t read_u64() noexcept;
	// Returns false (and doesn't update the state flags) if there's not enough bytes left
	inline bool try_read_u8(std::size_t& value) noexcept;
	inline bool try_read_u16(std::size_t& value) noexcept;
	inline bool try_read_u32(std::size_t& value) noexcept;

	inline void reset_rex_prefix_state() noexcept;
	inline void call_opcode_handlers_map0_table(Instruction& instruction) noexcept;
	inline std::uint32_t current_ip32() const noexcept;
	inline std::uint64_t current_ip64() const noexcept;
	inline void clear_mandatory_prefix(Instruction& instruction) noexcept;
	inline void set_xacquire_xrelease(Instruction& instruction, std::uint32_t flags) noexcept;
	void set_xacquire_xrelease_core(Instruction& instruction, std::uint32_t flags) noexcept;
	inline void clear_mandatory_prefix_f3(Instruction& instruction) const noexcept;
	inline void clear_mandatory_prefix_f2(Instruction& instruction) const noexcept;
	inline void set_invalid_instruction() noexcept;
	inline void decode_table2(HandlerEntry entry, Instruction& instruction) noexcept;
	inline void read_modrm() noexcept;
	void vex2(Instruction& instruction) noexcept;
	void vex3(Instruction& instruction) noexcept;
	void xop(Instruction& instruction) noexcept;
	void evex_mvex(Instruction& instruction) noexcept;
	inline std::uint32_t read_op_seg_reg() noexcept;
	// Same as Rust's `read_op_mem_stmt!()` (without the stmts)
	inline void read_op_mem(Instruction& instruction) noexcept;
	inline void read_op_mem_sib(Instruction& instruction) noexcept;
	inline void read_op_mem_mpx(Instruction& instruction) noexcept;
	inline void read_op_mem_tuple_type(Instruction& instruction, TupleType tuple_type) noexcept;
	inline void read_op_mem_vsib(Instruction& instruction, Register vsib_index, TupleType tuple_type) noexcept;
	void read_op_mem_16(Instruction& instruction, TupleType tuple_type) noexcept;
	inline bool read_op_mem_32_or_64(Instruction& instruction) noexcept;
	inline std::uint32_t disp8n(TupleType tuple_type) const noexcept;
	inline bool read_op_mem_32_or_64_vsib(Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;

	static bool read_op_mem_0(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_0_4(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_0_5(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_1(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_1_4(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_2(Instruction& instruction, DecoderCore& self) noexcept;
	static bool read_op_mem_2_4(Instruction& instruction, DecoderCore& self) noexcept;

	static bool read_op_mem_vsib_0(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_0_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_0_5(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_1(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_1_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_2(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;
	static bool read_op_mem_vsib_2_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept;

	// The hot part of decode_out() (inlined into the caller, like Rust's decode_out() gets inlined with LTO).
	// Only the table dispatch is inline, the handlers are out of line. Rarely used code is in decode_out_slow().
	ICED_X86_INTERNAL_HOT_INLINE void decode_out_inline(Instruction& instruction) noexcept;
	void decode_out_slow(Instruction& instruction, std::uintptr_t instr_start, std::uint64_t orig_ip) noexcept;

protected:
	DecoderCore() noexcept = default;
	ConstantOffsets get_constant_offsets_impl(const Instruction& instruction) const noexcept;
	static const char* init(DecoderCore& self, std::uint32_t bitness, const std::uint8_t* data, std::size_t data_len, std::uint64_t ip,
							std::uint32_t options) noexcept;
};

ICED_X86_INTERNAL_ALWAYS_INLINE std::size_t DecoderCore::read_u8() noexcept {
	const std::uintptr_t ptr = data_ptr;
	// This doesn't overflow data_ptr (verified in ctor)
	if (ICED_X86_INTERNAL_LIKELY(ptr < max_data_ptr)) {
		std::size_t result = *reinterpret_cast<const std::uint8_t*>(ptr);
		data_ptr = ptr + 1;
		return result;
	}
	state.flags |= SF_IS_INVALID | SF_NO_MORE_BYTES;
	return 0;
}

ICED_X86_INTERNAL_ALWAYS_INLINE void DecoderCore::read_modrm() noexcept {
	std::uint32_t m = static_cast<std::uint32_t>(read_u8());
	state.modrm = m;
#if defined(__GNUC__) && !defined(__clang__)
	// GCC's SLP vectorizer (-O3) merges the modrm/reg/mod/rm stores into SIMD shuffles + one 16-byte store which is
	// slower. This empty asm reads and writes `modrm` so the stores can't be merged. (The library is also compiled with
	// -fno-tree-slp-vectorize but this code is inlined into the caller.)
	__asm__("" : "+m"(state.modrm));
#endif
	state.reg = (m >> 3) & 7;
	state.mod_ = m >> 6;
	state.rm = m & 7;
	state.mem_index = (state.mod_ << 3) | state.rm;
}

ICED_X86_INTERNAL_ALWAYS_INLINE void DecoderCore::decode_table2(HandlerEntry entry, Instruction& instruction) noexcept {
	if (entry.handler->has_modrm)
		read_modrm();
	entry.decode(entry.handler, *this, instruction);
}

// Same as Rust's `decode_out_ptr()` (the rarely used code is in `decode_out_slow()`)
ICED_X86_INTERNAL_HOT_INLINE void DecoderCore::decode_out_inline(Instruction& instruction) noexcept {
	instruction = Instruction();

	state.extra_register_base = 0;
	state.extra_index_register_base = 0;
	state.extra_base_register_base = 0;
	state.extra_index_register_base_vsib = 0;
	state.flags = 0;
	state.mandatory_prefix = DecoderMandatoryPrefix::PNP;
	// These don't need to be cleared, but they're here so the compiler can re-use the
	// same XMM reg to clear the previous 2 u32s (including these 2 u32s).
	state.vvvv = 0;
	state.vvvv_invalid_check = 0;

	// We only need to write addr/op size fields and init segment_prio to 0.
	// The fields are consecutive so we can read all 4 fields (including dummy) and write all 4 fields at the same time.
	std::memcpy(&state.address_size, &default_address_size, 4);

	const std::uintptr_t instr_start = data_ptr;
	instr_start_data_ptr = instr_start;
	// The ctor has verified that the two expressions used in min() don't overflow and are >= data_ptr.
	// The calculated value is a valid pointer in `data` or at most 1 byte past the last valid byte.
	const std::uintptr_t max_ptr = instr_start + IcedConstants::MAX_INSTRUCTION_LENGTH;
	max_data_ptr = max_ptr < data_ptr_end ? max_ptr : data_ptr_end;

	std::size_t b = read_u8();
	HandlerEntry handler = handlers_map0[b];
	if ((static_cast<std::uint32_t>(b) & rex_mask) == 0x40) {
		assert(is64b_mode);
		handler = handlers_map0[read_u8()];
		std::uint32_t flags = state.flags | SF_HAS_REX;
		if ((b & 8) != 0) {
			flags |= SF_W;
			state.operand_size = static_cast<OpSize>(OP_SIZE64);
		}
		state.flags = flags;
		state.extra_register_base = (static_cast<std::uint32_t>(b) & 4) << 1;
		state.extra_index_register_base = (static_cast<std::uint32_t>(b) & 2) << 2;
		state.extra_base_register_base = (static_cast<std::uint32_t>(b) & 1) << 3;
	}
	decode_table2(handler, instruction);

	std::uint32_t instr_len = static_cast<std::uint32_t>(data_ptr) - static_cast<std::uint32_t>(instr_start);
	assert(instr_len <= IcedConstants::MAX_INSTRUCTION_LENGTH); // Could be 0 if there were no bytes available
	instruction.len_ = static_cast<std::uint8_t>(instr_len);
	std::uint64_t orig_ip = ip;
	std::uint64_t ip_ = orig_ip + instr_len;
	ip = ip_;
	instruction.next_rip_ = ip_;
	instruction.flags1_ |= static_cast<std::uint32_t>(default_code_size) << Instruction::F1_CODE_SIZE_SHIFT;

	std::uint32_t flags = state.flags;
	if ((flags & (SF_IS_INVALID | SF_LOCK | SF_IP_REL64 | SF_IP_REL32)) != 0) {
		// RIP rel ops are common, but invalid/lock bits are usually never set
		if ((flags & (SF_IP_REL64 | SF_IS_INVALID | SF_LOCK)) == SF_IP_REL64)
			instruction.mem_displ_ += ip_;
		else
			decode_out_slow(instruction, instr_start, orig_ip);
	}
}

// Used by the `Decoder` C array (`std::uint8_t[N]`) and `std::array<std::uint8_t, N>` overloads
template <typename T>
struct IsByteArray : std::false_type {};
template <std::size_t N>
struct IsByteArray<std::uint8_t[N]> : std::true_type {};
template <std::size_t N>
struct IsByteArray<std::array<std::uint8_t, N>> : std::true_type {};
// `A` is the deduced type of a forwarding reference (`A&&`): an lvalue `std::uint8_t[N]` or `std::array<std::uint8_t, N>`
// (braced lists can't be deduced so they don't match either)
template <typename A>
using EnableIfByteArrayLvalue =
	std::enable_if_t<std::is_lvalue_reference_v<A> && IsByteArray<std::remove_cv_t<std::remove_reference_t<A>>>::value, int>;
template <typename A>
using EnableIfByteArrayRvalue =
	std::enable_if_t<!std::is_lvalue_reference_v<A> && IsByteArray<std::remove_cv_t<std::remove_reference_t<A>>>::value, int>;

} // namespace internal

/// Decodes 16/32/64-bit x86 instructions
///
/// The decoder doesn't own the data, it must be valid (and not modified) until the decoder isn't used anymore.
/// The data can be passed as a pointer and a size, a C array (`std::uint8_t[N]`), a `std::array<std::uint8_t, N>` or
/// a `std::vector<std::uint8_t>`. Temporary arrays/vectors are rejected at compile time (deleted overloads).
///
/// # Examples
///
/// ```cpp
/// // xchg ah,[rdx+rsi+16h]
/// // xacquire lock add dword ptr [rax],5Ah
/// // vmovdqu64 zmm18{k3}{z},zmm11
/// static const std::uint8_t bytes[] = {0x86, 0x64, 0x32, 0x16, 0xF0, 0xF2, 0x83, 0x00, 0x5A, 0x62, 0xC1, 0xFE, 0xCB, 0x6F, 0xD3};
/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
/// for (const Instruction& instr : decoder) {
///     // ...
/// }
/// ```
class Decoder : private internal::DecoderCore {
public:
	class iterator;

	/// Creates a decoder
	///
	/// # Panics
	///
	/// Aborts if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `data`: Data to decode
	/// * `size`: Size of `data` in bytes
	/// * `options`: Decoder options, `0` or eg. `DecoderOptions::NO_INVALID_CHECK | DecoderOptions::AMD`
	///
	/// # Examples
	///
	/// ```cpp
	/// // xchg ah,[rdx+rsi+16h]
	/// // xacquire lock add dword ptr [rax],5Ah
	/// // vmovdqu64 zmm18{k3}{z},zmm11
	/// static const std::uint8_t bytes[] = {0x86, 0x64, 0x32, 0x16, 0xF0, 0xF2, 0x83, 0x00, 0x5A, 0x62, 0xC1, 0xFE, 0xCB, 0x6F, 0xD3};
	/// Decoder decoder(64, bytes, sizeof(bytes), DecoderOptions::NONE);
	/// decoder.set_ip(0x1234'5678);
	///
	/// Instruction instr1 = decoder.decode();
	/// assert(instr1.code() == Code::Xchg_rm8_r8);
	/// assert(instr1.mnemonic() == Mnemonic::Xchg);
	/// assert(instr1.len() == 4);
	///
	/// Instruction instr2 = decoder.decode();
	/// assert(instr2.code() == Code::Add_rm32_imm8);
	/// assert(instr2.mnemonic() == Mnemonic::Add);
	/// assert(instr2.len() == 5);
	///
	/// Instruction instr3 = decoder.decode();
	/// assert(instr3.code() == Code::EVEX_Vmovdqu64_zmm_k1z_zmmm512);
	/// assert(instr3.mnemonic() == Mnemonic::Vmovdqu64);
	/// assert(instr3.len() == 6);
	/// ```
	///
	/// It's sometimes useful to decode some invalid instructions, eg. `lock add esi,ecx`.
	/// Pass in `DecoderOptions::NO_INVALID_CHECK` to the constructor and the decoder
	/// will decode some invalid encodings.
	///
	/// ```cpp
	/// // lock add esi,ecx   ; lock not allowed
	/// static const std::uint8_t bytes[] = {0xF0, 0x01, 0xCE};
	/// Decoder decoder(64, bytes, sizeof(bytes), DecoderOptions::NONE);
	/// decoder.set_ip(0x1234'5678);
	/// Instruction instr = decoder.decode();
	/// assert(instr.code() == Code::INVALID);
	///
	/// // We want to decode some instructions with invalid encodings
	/// Decoder decoder2(64, bytes, sizeof(bytes), DecoderOptions::NO_INVALID_CHECK);
	/// decoder2.set_ip(0x1234'5678);
	/// instr = decoder2.decode();
	/// assert(instr.code() == Code::Add_rm32_r32);
	/// assert(instr.has_lock_prefix());
	/// ```
	Decoder(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint32_t options) noexcept;

	/// Creates a decoder, see `Decoder(bitness, data, size, options)`
	///
	/// # Panics
	///
	/// Aborts if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `data`: Data to decode. It must stay alive until the decoder isn't used anymore.
	/// * `options`: Decoder options, `0` or eg. `DecoderOptions::NO_INVALID_CHECK | DecoderOptions::AMD`
	Decoder(std::uint32_t bitness, const std::vector<std::uint8_t>& data, std::uint32_t options) noexcept
		: Decoder(bitness, data.data(), data.size(), options) {}
	// The decoder doesn't own the data so it can't be a temporary
	Decoder(std::uint32_t bitness, std::vector<std::uint8_t>&& data, std::uint32_t options) = delete;
	/// Creates a decoder, see `Decoder(bitness, data, size, options)`.
	/// `data` is a C array (`std::uint8_t[N]`) or a `std::array<std::uint8_t, N>`. It must stay alive until the decoder isn't
	/// used anymore.
	template <typename A, internal::EnableIfByteArrayLvalue<A> = 0>
	Decoder(std::uint32_t bitness, A&& data, std::uint32_t options) noexcept
		: Decoder(bitness, std::data(data), std::size(data), options) {}
	// The decoder doesn't own the data so it can't be a temporary
	template <typename A, internal::EnableIfByteArrayRvalue<A> = 0>
	Decoder(std::uint32_t bitness, A&& data, std::uint32_t options) = delete;

	/// Creates a decoder
	///
	/// # Panics
	///
	/// Aborts if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `data`: Data to decode
	/// * `size`: Size of `data` in bytes
	/// * `ip`: `RIP` value
	/// * `options`: Decoder options, `0` or eg. `DecoderOptions::NO_INVALID_CHECK | DecoderOptions::AMD`
	///
	/// # Examples
	///
	/// ```cpp
	/// // xchg ah,[rdx+rsi+16h]
	/// // xacquire lock add dword ptr [rax],5Ah
	/// // vmovdqu64 zmm18{k3}{z},zmm11
	/// static const std::uint8_t bytes[] = {0x86, 0x64, 0x32, 0x16, 0xF0, 0xF2, 0x83, 0x00, 0x5A, 0x62, 0xC1, 0xFE, 0xCB, 0x6F, 0xD3};
	/// auto decoder = Decoder::with_ip(64, bytes, sizeof(bytes), 0x1234'5678, DecoderOptions::NONE);
	///
	/// Instruction instr1 = decoder.decode();
	/// assert(instr1.code() == Code::Xchg_rm8_r8);
	/// assert(instr1.len() == 4);
	///
	/// Instruction instr2 = decoder.decode();
	/// assert(instr2.code() == Code::Add_rm32_imm8);
	/// assert(instr2.len() == 5);
	///
	/// Instruction instr3 = decoder.decode();
	/// assert(instr3.code() == Code::EVEX_Vmovdqu64_zmm_k1z_zmmm512);
	/// assert(instr3.len() == 6);
	/// ```
	static Decoder with_ip(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint64_t ip, std::uint32_t options) noexcept;

	/// Creates a decoder, see `with_ip(bitness, data, size, ip, options)`
	static Decoder with_ip(std::uint32_t bitness, const std::vector<std::uint8_t>& data, std::uint64_t ip, std::uint32_t options) noexcept {
		return with_ip(bitness, data.data(), data.size(), ip, options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	static Decoder with_ip(std::uint32_t bitness, std::vector<std::uint8_t>&& data, std::uint64_t ip, std::uint32_t options) = delete;
	/// Creates a decoder, see `with_ip(bitness, data, size, ip, options)`.
	/// `data` is a C array (`std::uint8_t[N]`) or a `std::array<std::uint8_t, N>`. It must stay alive until the decoder isn't
	/// used anymore.
	template <typename A, internal::EnableIfByteArrayLvalue<A> = 0>
	static Decoder with_ip(std::uint32_t bitness, A&& data, std::uint64_t ip, std::uint32_t options) noexcept {
		return with_ip(bitness, std::data(data), std::size(data), ip, options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	template <typename A, internal::EnableIfByteArrayRvalue<A> = 0>
	static Decoder with_ip(std::uint32_t bitness, A&& data, std::uint64_t ip, std::uint32_t options) = delete;

	/// Creates a decoder
	///
	/// # Errors
	///
	/// Fails if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `data`: Data to decode
	/// * `size`: Size of `data` in bytes
	/// * `options`: Decoder options, `0` or eg. `DecoderOptions::NO_INVALID_CHECK | DecoderOptions::AMD`
	///
	/// # Examples
	///
	/// ```cpp
	/// static const std::uint8_t bytes[] = {0x86, 0x64, 0x32, 0x16};
	/// auto result = Decoder::try_new(64, bytes, sizeof(bytes), DecoderOptions::NONE);
	/// assert(result.is_ok());
	/// Decoder& decoder = result.value();
	/// assert(decoder.decode().code() == Code::Xchg_rm8_r8);
	///
	/// assert(Decoder::try_new(128, bytes, sizeof(bytes), DecoderOptions::NONE).is_err());
	/// ```
	static Result<Decoder> try_new(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint32_t options) noexcept;

	/// Creates a decoder, see `try_new(bitness, data, size, options)`
	static Result<Decoder> try_new(std::uint32_t bitness, const std::vector<std::uint8_t>& data, std::uint32_t options) noexcept {
		return try_new(bitness, data.data(), data.size(), options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	static Result<Decoder> try_new(std::uint32_t bitness, std::vector<std::uint8_t>&& data, std::uint32_t options) = delete;
	/// Creates a decoder, see `try_new(bitness, data, size, options)`.
	/// `data` is a C array (`std::uint8_t[N]`) or a `std::array<std::uint8_t, N>`. It must stay alive until the decoder isn't
	/// used anymore.
	template <typename A, internal::EnableIfByteArrayLvalue<A> = 0>
	static Result<Decoder> try_new(std::uint32_t bitness, A&& data, std::uint32_t options) noexcept {
		return try_new(bitness, std::data(data), std::size(data), options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	template <typename A, internal::EnableIfByteArrayRvalue<A> = 0>
	static Result<Decoder> try_new(std::uint32_t bitness, A&& data, std::uint32_t options) = delete;

	/// Creates a decoder
	///
	/// # Errors
	///
	/// Fails if `bitness` is not one of 16, 32, 64.
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	/// * `data`: Data to decode
	/// * `size`: Size of `data` in bytes
	/// * `ip`: `RIP` value
	/// * `options`: Decoder options, `0` or eg. `DecoderOptions::NO_INVALID_CHECK | DecoderOptions::AMD`
	static Result<Decoder> try_with_ip(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint64_t ip,
									   std::uint32_t options) noexcept;

	/// Creates a decoder, see `try_with_ip(bitness, data, size, ip, options)`
	static Result<Decoder> try_with_ip(std::uint32_t bitness, const std::vector<std::uint8_t>& data, std::uint64_t ip, std::uint32_t options) noexcept {
		return try_with_ip(bitness, data.data(), data.size(), ip, options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	static Result<Decoder> try_with_ip(std::uint32_t bitness, std::vector<std::uint8_t>&& data, std::uint64_t ip, std::uint32_t options) = delete;
	/// Creates a decoder, see `try_with_ip(bitness, data, size, ip, options)`.
	/// `data` is a C array (`std::uint8_t[N]`) or a `std::array<std::uint8_t, N>`. It must stay alive until the decoder isn't
	/// used anymore.
	template <typename A, internal::EnableIfByteArrayLvalue<A> = 0>
	static Result<Decoder> try_with_ip(std::uint32_t bitness, A&& data, std::uint64_t ip, std::uint32_t options) noexcept {
		return try_with_ip(bitness, std::data(data), std::size(data), ip, options);
	}
	// The decoder doesn't own the data so it can't be a temporary
	template <typename A, internal::EnableIfByteArrayRvalue<A> = 0>
	static Result<Decoder> try_with_ip(std::uint32_t bitness, A&& data, std::uint64_t ip, std::uint32_t options) = delete;

	/// Gets the current `IP`/`EIP`/`RIP` value, see also `position()`
	std::uint64_t ip() const noexcept { return DecoderCore::ip; }

	/// Sets the current `IP`/`EIP`/`RIP` value, see also `set_position()`
	///
	/// This method only updates the IP value, it does not change the data position, use `set_position()` to change the position.
	///
	/// # Arguments
	///
	/// * `new_value`: New IP
	void set_ip(std::uint64_t new_value) noexcept { DecoderCore::ip = new_value; }

	/// Gets the bitness (16, 32 or 64)
	std::uint32_t bitness() const noexcept { return DecoderCore::bitness; }

	/// Gets the max value that can be passed to `set_position()`. This is the size of the data that gets
	/// decoded to instructions and it's the length of the data that was passed to the constructor.
	std::size_t max_position() const noexcept { return data_len; }

	/// Gets the current data position. This value is always <= `max_position()`.
	/// When `position()` == `max_position()`, it's not possible to decode more
	/// instructions and `can_decode()` returns `false`.
	std::size_t position() const noexcept { return static_cast<std::size_t>(data_ptr - reinterpret_cast<std::uintptr_t>(data)); }

	/// Sets the current data position, which is the index into the data passed to the constructor.
	/// This value is always <= `max_position()`
	///
	/// # Errors
	///
	/// Fails if the new position is invalid.
	///
	/// # Arguments
	///
	/// * `new_pos`: New position and must be <= `max_position()`
	///
	/// # Examples
	///
	/// ```cpp
	/// // nop and pause
	/// static const std::uint8_t bytes[] = {0x90, 0xF3, 0x90};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	///
	/// assert(decoder.position() == 0);
	/// assert(decoder.max_position() == 3);
	/// Instruction instr = decoder.decode();
	/// assert(decoder.position() == 1);
	/// assert(instr.code() == Code::Nopd);
	///
	/// instr = decoder.decode();
	/// assert(decoder.position() == 3);
	/// assert(instr.code() == Code::Pause);
	///
	/// // Start all over again
	/// decoder.set_position(0).value();
	/// decoder.set_ip(0x1234'5678);
	/// assert(decoder.position() == 0);
	/// assert(decoder.decode().code() == Code::Nopd);
	/// assert(decoder.decode().code() == Code::Pause);
	/// assert(decoder.position() == 3);
	/// ```
	Result<void> set_position(std::size_t new_pos) noexcept {
		if (new_pos > data_len)
			return IcedError("Invalid position");
		// - We verified the new offset above.
		// - Referencing 1 byte past the last valid byte is safe as long as we don't dereference it.
		data_ptr = reinterpret_cast<std::uintptr_t>(data) + new_pos;
		return Result<void>();
	}

	/// Same as `set_position()`
	Result<void> try_set_position(std::size_t new_pos) noexcept { return set_position(new_pos); }

	/// Returns `true` if there's at least one more byte to decode. It doesn't verify that the
	/// next instruction is valid, it only checks if there's at least one more byte to read.
	/// See also `position()` and `max_position()`
	///
	/// It's not required to call this method. If this method returns `false`, then `decode_out()`
	/// and `decode()` will return an instruction whose `code()` == `Code::INVALID`.
	///
	/// # Examples
	///
	/// ```cpp
	/// // nop and an incomplete instruction
	/// static const std::uint8_t bytes[] = {0x90, 0xF3, 0x0F};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	///
	/// // 3 bytes left to read
	/// assert(decoder.can_decode());
	/// Instruction instr = decoder.decode();
	/// assert(instr.code() == Code::Nopd);
	///
	/// // 2 bytes left to read
	/// assert(decoder.can_decode());
	/// instr = decoder.decode();
	/// // Not enough bytes left to decode a full instruction
	/// assert(instr.code() == Code::INVALID);
	///
	/// // 0 bytes left to read
	/// assert(!decoder.can_decode());
	/// ```
	bool can_decode() const noexcept { return data_ptr != data_ptr_end; }

	/// Gets the last decoder error. Unless you need to know the reason it failed,
	/// it's better to check `instruction.is_invalid()`.
	DecoderError last_error() const noexcept;

	/// Decodes and returns the next instruction, see also `decode_out(Instruction&)`
	/// which avoids copying the decoded instruction to the caller's return variable.
	/// See also `last_error()`.
	///
	/// # Examples
	///
	/// ```cpp
	/// // xrelease lock add [rax],ebx
	/// static const std::uint8_t bytes[] = {0xF0, 0xF3, 0x01, 0x18};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	/// Instruction instr = decoder.decode();
	///
	/// assert(instr.code() == Code::Add_rm32_r32);
	/// assert(instr.mnemonic() == Mnemonic::Add);
	/// assert(instr.len() == 4);
	/// assert(instr.op_count() == 2);
	///
	/// assert(instr.op0_kind() == OpKind::Memory);
	/// assert(instr.memory_base() == Register::RAX);
	/// assert(instr.memory_index() == Register::None);
	/// assert(instr.memory_index_scale() == 1);
	/// assert(instr.memory_displacement64() == 0);
	/// assert(instr.memory_segment() == Register::DS);
	/// assert(instr.segment_prefix() == Register::None);
	/// assert(instr.memory_size() == MemorySize::UInt32);
	///
	/// assert(instr.op1_kind() == OpKind::Register);
	/// assert(instr.op1_register() == Register::EBX);
	///
	/// assert(instr.has_lock_prefix());
	/// assert(instr.has_xrelease_prefix());
	/// ```
	ICED_X86_INTERNAL_HOT_INLINE Instruction decode() noexcept {
		Instruction instruction;
		decode_out_inline(instruction);
		return instruction;
	}

	/// Decodes the next instruction. The difference between this method and `decode()` is that this
	/// method doesn't need to copy the result to the caller's return variable (saves 40 bytes of copying).
	/// See also `last_error()`.
	///
	/// # Arguments
	///
	/// * `instruction`: Updated with the decoded instruction. All fields are initialized (it's an `out` argument)
	///
	/// # Examples
	///
	/// ```cpp
	/// // xrelease lock add [rax],ebx
	/// static const std::uint8_t bytes[] = {0xF0, 0xF3, 0x01, 0x18};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	/// Instruction instr;
	/// decoder.decode_out(instr);
	///
	/// assert(instr.code() == Code::Add_rm32_r32);
	/// assert(instr.len() == 4);
	/// assert(instr.has_lock_prefix());
	/// assert(instr.has_xrelease_prefix());
	/// ```
	ICED_X86_INTERNAL_HOT_INLINE void decode_out(Instruction& instruction) noexcept { decode_out_inline(instruction); }

	/// Gets the offsets of the constants (memory displacement and immediate) in the decoded instruction.
	/// The caller can check if there are any relocations at those addresses.
	///
	/// # Arguments
	///
	/// * `instruction`: The latest instruction that was decoded by this decoder
	///
	/// # Examples
	///
	/// ```cpp
	/// // nop
	/// // xor dword ptr [rax-5AA5EDCCh],5Ah
	/// //                  00  01  02  03  04  05  06
	/// //                \opc\mrm\displacement___\imm
	/// static const std::uint8_t bytes[] = {0x90, 0x83, 0xB3, 0x34, 0x12, 0x5A, 0xA5, 0x5A};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	/// assert(decoder.decode().code() == Code::Nopd);
	/// Instruction instr = decoder.decode();
	/// ConstantOffsets co = decoder.get_constant_offsets(instr);
	///
	/// assert(co.has_displacement());
	/// assert(co.displacement_offset() == 2);
	/// assert(co.displacement_size() == 4);
	/// assert(co.has_immediate());
	/// assert(co.immediate_offset() == 6);
	/// assert(co.immediate_size() == 1);
	/// // It's not an instruction with two immediates (e.g. enter)
	/// assert(!co.has_immediate2());
	/// assert(co.immediate_offset2() == 0);
	/// assert(co.immediate_size2() == 0);
	/// ```
	ConstantOffsets get_constant_offsets(const Instruction& instruction) const noexcept { return get_constant_offsets_impl(instruction); }

	/// Returns an iterator that decodes instructions until there's no more data to decode,
	/// i.e., until `can_decode()` returns `false`.
	///
	/// The iterator decodes directly into an `Instruction` stored in the iterator (no copying), so
	/// `*it` is only valid until the iterator is incremented.
	///
	/// # Examples
	///
	/// ```cpp
	/// // nop and pause
	/// static const std::uint8_t bytes[] = {0x90, 0xF3, 0x90};
	/// auto decoder = Decoder::with_ip(64, bytes, 0x1234'5678, DecoderOptions::NONE);
	///
	/// for (const Instruction& instr : decoder) {
	///     // Nopd, Pause
	///     std::printf("code: %s\n", to_string(instr.code()));
	/// }
	/// ```
	iterator begin() noexcept;
	/// End iterator, see `begin()`
	iterator end() noexcept;

private:
	Decoder() noexcept = default;
};

/// Decoder iterator (an input iterator), see `Decoder::begin()`
class Decoder::iterator {
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = Instruction;
	using difference_type = std::ptrdiff_t;
	using pointer = const Instruction*;
	using reference = const Instruction&;

	/// Creates an end iterator
	iterator() noexcept : decoder_(nullptr), instruction_() {}

	/// Gets the current instruction
	reference operator*() const noexcept { return instruction_; }
	/// Gets the current instruction
	pointer operator->() const noexcept { return &instruction_; }

	/// Decodes the next instruction
	iterator& operator++() noexcept {
		next();
		return *this;
	}

	/// Two iterators are equal if both are end iterators or if they're the same iterator
	bool operator==(const iterator& other) const noexcept { return decoder_ == other.decoder_; }
	/// Two iterators are equal if both are end iterators or if they're the same iterator
	bool operator!=(const iterator& other) const noexcept { return decoder_ != other.decoder_; }

private:
	friend class Decoder;
	explicit iterator(Decoder* decoder) noexcept : decoder_(decoder), instruction_() { next(); }

	void next() noexcept {
		if (decoder_ != nullptr) {
			if (decoder_->can_decode())
				decoder_->decode_out(instruction_);
			else
				decoder_ = nullptr;
		}
	}

	Decoder* decoder_;
	Instruction instruction_;
};

inline Decoder::iterator Decoder::begin() noexcept { return iterator(this); }
inline Decoder::iterator Decoder::end() noexcept { return iterator(); }

} // namespace iced_x86
