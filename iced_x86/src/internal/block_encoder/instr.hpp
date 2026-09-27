// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "iced_x86/code.hpp"
#include "iced_x86/constant_offsets.hpp"
#include "iced_x86/encoder.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "internal/block_encoder/block.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

// Rust uses a `Vec<(InstrBase, Box<dyn Instr>)>`. To avoid one heap allocation per instruction and virtual calls, the
// C++ code stores all instructions in one `std::vector<InstrEntry>`: `Instr` is a tagged struct (`InstrType`) and the
// instruction specific code is in `SimpleInstr`, `JccInstr`, etc (static functions, one .cpp file per kind).

struct BlockEncInt {
	std::uint32_t bitness;
	std::uint32_t options; // BlockEncoderOptions
	Encoder null_encoder;
	// Sorted in reverse order (highest IP first)
	std::vector<std::pair<std::uint64_t, std::size_t>> to_instr_index;
	bool has_multiple_zero_ip_instrs;

	BlockEncInt(std::uint32_t bitness_, std::uint32_t options_)
		: bitness(bitness_), options(options_), null_encoder(bitness_), to_instr_index(), has_multiple_zero_ip_instrs(false) {}

	bool fix_branches() const noexcept { return (options & BlockEncoderOptions::DONT_FIX_BRANCHES) == 0; }

	std::uint32_t get_instruction_size(const Instruction& instruction, std::uint64_t ip);
};

// This is a little bit ugly.
//
// TargetInstr::address() needs to get the ip of the target instr. We can't pass in the `all_instrs`
// slice since the caller is `&mut` from inside that very same slice. So we had to move the `ip` field
// from Instr to its own vec, which is stored in the `all_ips` slice.
//
// `ip` is also used by the Instr methods so need to be part of the context as well.
struct InstrContext {
	Block& block;
	std::uint64_t* all_ips;
	std::uint64_t ip;
};

// Data common among all `Instr` impls. It eliminates many slow virtual calls to getters.
struct InstrBase {
	std::uint32_t size;
	std::uint64_t orig_ip;
	// If it can't be optimized, this will be set to true
	bool done;
};

enum class TargetInstrKind : std::uint8_t {
	Uninitialized,
	Instruction,
	Address,
	IsOwner,
};

class TargetInstr {
public:
	constexpr TargetInstr() noexcept : value_(0), kind_(TargetInstrKind::Uninitialized) {}

	static constexpr TargetInstr new_instr(std::size_t instr_index) noexcept { return TargetInstr(TargetInstrKind::Instruction, instr_index); }
	static constexpr TargetInstr new_address(std::uint64_t address) noexcept { return TargetInstr(TargetInstrKind::Address, address); }
	static constexpr TargetInstr new_owner() noexcept { return TargetInstr(TargetInstrKind::IsOwner, 0); }

	bool is_in_block(const Block& block) const noexcept {
		switch (kind_) {
		case TargetInstrKind::Instruction:
			return block.is_in_block(static_cast<std::size_t>(value_));
		case TargetInstrKind::Address:
			return false;
		case TargetInstrKind::IsOwner:
			// The owner checks if the input block is part of its block, so return true
			return true;
		case TargetInstrKind::Uninitialized:
		default:
			ICED_UNREACHABLE();
		}
	}

	std::uint64_t address(const InstrContext& ctx) const noexcept {
		switch (kind_) {
		case TargetInstrKind::Instruction:
			return ctx.all_ips[static_cast<std::size_t>(value_)];
		case TargetInstrKind::Address:
			return value_;
		case TargetInstrKind::IsOwner:
			return ctx.ip;
		case TargetInstrKind::Uninitialized:
		default:
			ICED_UNREACHABLE();
		}
	}

private:
	constexpr TargetInstr(TargetInstrKind kind, std::uint64_t value) noexcept : value_(value), kind_(kind) {}

	// Instruction index or address
	std::uint64_t value_;
	TargetInstrKind kind_;
};

// Result of `Instr::encode()`: the constant offsets and `true` if it's the original instruction
struct InstrEncodeResult {
	ConstantOffsets constant_offsets;
	bool is_original_instruction;
};

enum class InstrType : std::uint8_t {
	Simple,
	SimpleBranch,
	Jmp,
	Jcc,
	Call,
	IpRelMemOp,
	Xbegin,
};

// The `InstrKind` enums of the instruction wrappers
enum class SimpleBranchInstrKind : std::uint8_t {
	Unchanged,
	Short,
	Near,
	Long,
	Uninitialized,
};
enum class JmpInstrKind : std::uint8_t {
	Unchanged,
	Short,
	Near,
	Long,
	Uninitialized,
};
enum class JccInstrKind : std::uint8_t {
	Unchanged,
	Short,
	Near,
	Long,
	Uninitialized,
};
enum class IpRelMemOpInstrKind : std::uint8_t {
	Unchanged,
	Rip,
	Eip,
	Long,
	Uninitialized,
};
enum class XbeginInstrKind : std::uint8_t {
	Unchanged,
	Rel16,
	Rel32,
	Uninitialized,
};

// The fields that aren't part of all instructions (`Instr::u`)
struct SimpleBranchInstrData {
	SimpleBranchInstrKind instr_kind;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	std::uint8_t long_instruction_size;
	std::uint8_t native_instruction_size;
	Code native_code;
};
struct JmpInstrData {
	JmpInstrKind instr_kind;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
};
struct JccInstrData {
	JccInstrKind instr_kind;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
	std::uint8_t long_instruction_size64;
};
struct CallInstrData {
	std::uint8_t orig_instruction_size;
	bool use_orig_instruction;
};
struct IpRelMemOpInstrData {
	IpRelMemOpInstrKind instr_kind;
	std::uint8_t eip_instruction_size;
	std::uint8_t rip_instruction_size;
};
struct XbeginInstrData {
	XbeginInstrKind instr_kind;
	std::uint8_t short_instruction_size;
	std::uint8_t near_instruction_size;
};

// No pointer data (`None` in Rust)
constexpr std::uint32_t NO_POINTER_DATA = UINT32_MAX;

struct Instr {
	Instruction instruction;
	// Not used by SimpleInstr
	TargetInstr target_instr;
	// Index of the block's `BlockData` or `NO_POINTER_DATA`. Only used by SimpleBranchInstr, JmpInstr, JccInstr, CallInstr
	std::uint32_t pointer_data;
	InstrType type;
	std::uint8_t bitness;
	union {
		SimpleBranchInstrData simple_br;
		JmpInstrData jmp;
		JccInstrData jcc;
		CallInstrData call;
		IpRelMemOpInstrData ip_rel;
		XbeginInstrData xbegin;
	} u;

	Instr() noexcept : instruction(), target_instr(), pointer_data(NO_POINTER_DATA), type(InstrType::Simple), bitness(0), u() {}

	// Returns the target and the original target address
	std::pair<TargetInstr*, std::uint64_t> get_target_instr() noexcept;

	/// Returns `true` if the instruction was updated to a shorter instruction, `false` if nothing changed
	bool optimize(InstrBase& base, InstrContext& ctx, std::uint64_t gained);

	Result<InstrEncodeResult> encode(InstrBase& base, InstrContext& ctx);
};

struct InstrEntry {
	InstrBase base;
	Instr instr;
};

struct SimpleInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct SimpleBranchInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct JmpInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct JccInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct CallInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct IpRelMemOpInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

struct XbeginInstr {
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& self, const Instruction& instruction);
	static bool optimize(Instr& self, InstrBase& base, InstrContext& ctx, std::uint64_t gained);
	static Result<InstrEncodeResult> encode(Instr& self, InstrBase& base, InstrContext& ctx);
};

std::int64_t correct_diff(bool in_block, std::int64_t diff, std::uint64_t gained) noexcept;

struct InstrUtils {
	// 6 = FF 15 XXXXXXXX = call qword ptr [rip+mem_target]
	static constexpr std::uint32_t CALL_OR_JMP_POINTER_DATA_INSTRUCTION_SIZE64 = 6;

	static IcedError create_error_message(const char* error_message, const Instruction& instruction);
	static IcedError create_error_message(const IcedError& error, const Instruction& instruction) {
		return create_error_message(error.message(), instruction);
	}

	// Initializes `instr` (and `base.size`, `base.done`)
	static void create(BlockEncInt& block_encoder, InstrBase& base, Instr& instr, const Instruction& instruction);

	static Result<std::uint32_t> encode_branch_to_pointer_data(Block& block, bool is_call, std::uint64_t ip, std::uint32_t pointer_data,
		std::uint32_t min_size);

	static std::int64_t convert_diff_to_bitness_diff(std::uint8_t bitness, std::int64_t diff) noexcept {
		ICED_DEBUG_ASSERT(bitness == 16 || bitness == 32 || bitness == 64);
		switch (bitness) {
		case 16:
			return static_cast<std::int16_t>(diff);
		case 32:
			return static_cast<std::int32_t>(diff);
		default:
			return diff;
		}
	}
};

} // namespace iced_x86::internal
