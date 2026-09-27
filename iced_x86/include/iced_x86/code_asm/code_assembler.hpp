// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/block_encoder.hpp"
#include "iced_x86/code_asm/code_assembler_base.hpp"
#include "iced_x86/code_asm/code_assembler_fns.hpp"
#include "iced_x86/code_asm/code_label.hpp"
#include "iced_x86/code_asm/mem.hpp"
#include "iced_x86/code_asm/mem_ptr.hpp"
#include "iced_x86/code_asm/op_state.hpp"
#include "iced_x86/code_asm/reg.hpp"
#include "iced_x86/code_asm/registers.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/memory_operand.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace iced_x86::code_asm {

/// Result of assembling the instructions, see `CodeAssembler::assemble_options()`
class CodeAssemblerResult {
public:
	/// Inner `BlockEncoder` result
	BlockEncoderResult inner;

	/// Gets the address of a label
	///
	/// You should pass `BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS` to `CodeAssembler::assemble_options()`
	/// or this method will fail.
	///
	/// Fails if the label is invalid.
	///
	/// @param label The label
	[[nodiscard]] Result<std::uint64_t> label_ip(const CodeLabel& label) const;
};

/// Creates and encodes instructions. It's easier to use this class than to call `Instruction::with*()` functions.
///
/// The instruction methods (eg. `mov()`, thousands of overloads) and the option and error methods (eg. `bitness()`,
/// `has_error()`) are declared in the base classes (`internal::CodeAssemblerBase`, `internal::CodeAssemblerFns0`, ...).
///
/// # Integer arguments
///
/// The instruction methods have `std::int32_t`/`std::uint32_t` (or `std::int64_t`/`std::uint64_t`) overloads for
/// immediates (Rust: `i32`/`u32`/`i64`/`u64` trait impls). Other integer types (eg. `long long`, `unsigned long`,
/// `std::size_t`, `-1LL`, `0x1234ULL`) are converted to the type with the same size and signedness (types smaller than
/// `int` are promoted to `int`), so any integer type can be passed as long as the instruction supports an immediate of
/// that size, eg. `a.mov(rax, 0x1234'5678'9ABC'DEF0ULL)` works but `a.add(rax, 1LL)` doesn't compile since `add()`
/// only takes 32-bit immediates (same as `a.add(rax, std::int64_t{1})`: use `1` instead).
///
/// # Errors
///
/// All instruction methods (eg. `mov()`), prefix methods, `db()`, `set_label()` etc return `CodeAssembler&` so calls
/// can be chained. Errors are *sticky*: the first error (eg. an invalid operand combination) is stored in the
/// assembler and all later calls that add instructions or labels are ignored until the error is cleared (see
/// `has_error()`, `error()`, `clear_error()`). `assemble()` and `assemble_options()` fail with the stored error.
///
/// # Examples
///
/// ```cpp
/// #include "iced_x86/code_asm.hpp"
/// using namespace iced_x86;
/// using namespace iced_x86::code_asm;
///
/// CodeAssembler a(64);
///
/// // Anytime you add something to a register (or subtract from it), you create a
/// // memory operand. You can also call word_ptr(), dword_bcst() etc to create memory
/// // operands.
/// auto r = rax; // register
/// auto m1 = rax + 0; // memory with no size hint
/// auto m2 = ptr(rax); // memory with no size hint
/// auto m3 = rax + rcx * 4 - 123; // memory with no size hint
/// // To create a memory operand with only a displacement or only a base register,
/// // you can call one of the memory fns:
/// auto m4 = qword_ptr(123); // memory with a qword size hint
/// auto m5 = dword_bcst(rcx); // memory (broadcast) with a dword size hint
/// // To add a segment override, call the segment methods:
/// auto m6 = ptr(rax).fs(); // fs:[rax]
///
/// // Each mnemonic is a method
/// a.push(rcx);
/// // There are a few exceptions where you must append `_<opcount>` to the mnemonic to
/// // get the instruction you need:
/// a.ret();
/// a.ret_1(123);
/// // Use byte_ptr(), word_bcst(), etc to force the arg to a memory operand and to add a
/// // size hint
/// a.xor_(byte_ptr(rdx + r14 * 4 + 123), 0x10);
/// // Prefixes are also methods
/// a.rep().stosd();
/// // Immediates can be any integer type (eg. `int`, `unsigned`, `long long`, `std::uint64_t`):
/// a.mov(rax, 0x1234'5678'9ABC'DEF0ULL);
///
/// // Create labels that can be referenced by code
/// CodeLabel loop_lbl1 = a.create_label();
/// CodeLabel after_loop1 = a.create_label();
/// a.mov(ecx, 10);
/// a.set_label(loop_lbl1);
/// a.dec(ecx);
/// a.jp(after_loop1);
/// a.jne(loop_lbl1);
/// a.set_label(after_loop1);
///
/// // It's possible to reference labels with RIP-relative addressing
/// CodeLabel skip_data = a.create_label();
/// CodeLabel data = a.create_label();
/// a.jmp(skip_data);
/// a.set_label(data);
/// a.db({0x90, 0xCC, 0xF1, 0x90});
/// a.set_label(skip_data);
/// a.lea(rax, ptr(data));
///
/// // AVX512 opmasks, {z}, {sae}, {er} and broadcasting are also supported:
/// a.vsqrtps(zmm16.k2().z(), dword_bcst(rcx));
/// a.vsqrtps(zmm1.k2().z(), zmm23.rd_sae());
/// // Sometimes, the encoder doesn't know if you want VEX or EVEX encoding.
/// // You can force EVEX globally like so:
/// a.set_prefer_vex(false);
/// a.vucomiss(xmm31, xmm15.sae());
/// a.vucomiss(xmm31, ptr(rcx));
/// // or call vex()/evex() to override the encoding option:
/// a.evex().vucomiss(xmm31, xmm15.sae());
/// a.vex().vucomiss(xmm15, xmm14);
///
/// // Encode all added instructions
/// Result<std::vector<std::uint8_t>> bytes = a.assemble(0x12345678);
/// if (bytes.is_err()) {
///     // a.has_error() is true if an instruction method failed, else the block encoder failed
///     std::puts(bytes.error().message());
/// }
/// else {
///     assert(bytes.value().size() == 82);
/// }
/// // If you don't want to encode them, you can get all instructions by calling
/// // one of these methods:
/// const std::vector<Instruction>& instrs = a.instructions(); // Get a reference to the internal vector
/// assert(instrs.size() == 19);
/// std::vector<Instruction> instrs2 = a.take_instructions(); // Take ownership of the vector with all instructions
/// assert(instrs2.size() == 19);
/// assert(a.instructions().empty());
/// ```
class CodeAssembler : public internal::CodeAssemblerFnsAll {
public:
	/// Creates a new instance.
	///
	/// If `bitness` is invalid, the error is stored in the instance (see `has_error()`) and `assemble()` fails.
	/// Use `create()` if you prefer to get a `Result`.
	///
	/// @param bitness 16, 32, or 64
	explicit CodeAssembler(std::uint32_t bitness);

	/// Creates a new instance
	///
	/// Fails if `bitness` is invalid
	///
	/// @param bitness 16, 32, or 64
	[[nodiscard]] static Result<CodeAssembler> create(std::uint32_t bitness);

	/// Adds an `XACQUIRE` prefix to the next added instruction
	CodeAssembler& xacquire() noexcept {
		prefix_flags_ |= PrefixFlags::REPNE;
		return *this;
	}

	/// Adds an `XRELEASE` prefix to the next added instruction
	CodeAssembler& xrelease() noexcept {
		prefix_flags_ |= PrefixFlags::REPE;
		return *this;
	}

	/// Adds a `LOCK` prefix to the next added instruction
	CodeAssembler& lock() noexcept {
		prefix_flags_ |= PrefixFlags::LOCK;
		return *this;
	}

	/// Adds a `REP` prefix to the next added instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.rep().stosq();
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xF3, 0x48, 0xAB, 0x90}));
	/// ```
	CodeAssembler& rep() noexcept {
		prefix_flags_ |= PrefixFlags::REPE;
		return *this;
	}

	/// Adds a `REPE`/`REPZ` prefix to the next added instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.repe().cmpsb();
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xF3, 0xA6, 0x90}));
	/// ```
	CodeAssembler& repe() noexcept {
		prefix_flags_ |= PrefixFlags::REPE;
		return *this;
	}

	/// Adds a `REPE`/`REPZ` prefix to the next added instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.repz().cmpsb();
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xF3, 0xA6, 0x90}));
	/// ```
	CodeAssembler& repz() noexcept { return repe(); }

	/// Adds a `REPNE`/`REPNZ` prefix to the next added instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.repne().scasb();
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xF2, 0xAE, 0x90}));
	/// ```
	CodeAssembler& repne() noexcept {
		prefix_flags_ |= PrefixFlags::REPNE;
		return *this;
	}

	/// Adds a `REPNE`/`REPNZ` prefix to the next added instruction
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.repnz().scasb();
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xF2, 0xAE, 0x90}));
	/// ```
	CodeAssembler& repnz() noexcept { return repne(); }

	/// Adds a `BND` prefix to the next added instruction
	CodeAssembler& bnd() noexcept {
		prefix_flags_ |= PrefixFlags::REPNE;
		return *this;
	}

	/// Adds a `NOTRACK` prefix to the next added instruction
	CodeAssembler& notrack() noexcept {
		prefix_flags_ |= PrefixFlags::NOTRACK;
		return *this;
	}

	/// Prefer `VEX` encoding if the next instruction can be `VEX` and `EVEX` encoded
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// // This instruction can be VEX and EVEX encoded
	/// a.vex().vaddpd(xmm1, xmm2, xmm3);
	/// a.evex().vaddpd(xmm1, xmm2, xmm3);
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xC5, 0xE9, 0x58, 0xCB, 0x62, 0xF1, 0xED, 0x08, 0x58, 0xCB}));
	/// ```
	CodeAssembler& vex() noexcept {
		prefix_flags_ |= PrefixFlags::PREFER_VEX;
		return *this;
	}

	/// Prefer `EVEX` encoding if the next instruction can be `VEX` and `EVEX` encoded
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// // This instruction can be VEX and EVEX encoded
	/// a.vex().vaddpd(xmm1, xmm2, xmm3);
	/// a.evex().vaddpd(xmm1, xmm2, xmm3);
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0xC5, 0xE9, 0x58, 0xCB, 0x62, 0xF1, 0xED, 0x08, 0x58, 0xCB}));
	/// ```
	CodeAssembler& evex() noexcept {
		prefix_flags_ |= PrefixFlags::PREFER_EVEX;
		return *this;
	}

	/// Gets all added instructions, see also `take_instructions()` and `assemble()`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.push(rcx);
	/// a.xor_(rcx, rdx);
	/// assert(a.instructions().size() == 2);
	/// assert(a.instructions()[0] == Instruction::with1(Code::Push_r64, Register::RCX).value());
	/// assert(a.instructions()[1] == Instruction::with2(Code::Xor_rm64_r64, Register::RCX, Register::RDX).value());
	/// ```
	[[nodiscard]] const std::vector<Instruction>& instructions() const noexcept { return instructions_; }

	/// Takes ownership of all instructions and returns them. Instruction state is also reset (see `reset()`),
	/// including the error (check `has_error()` before calling this method).
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.push(rcx);
	/// a.xor_(rcx, rdx);
	/// assert(a.instructions().size() == 2);
	/// std::vector<Instruction> instrs = a.take_instructions();
	/// assert(a.instructions().empty());
	/// assert(instrs.size() == 2);
	/// ```
	[[nodiscard]] std::vector<Instruction> take_instructions();

	/// Resets all instructions, labels, prefixes and the error so this instance can be re-used
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.push(rcx);
	/// a.xor_(rcx, rdx);
	/// assert(a.instructions().size() == 2);
	/// a.reset();
	/// assert(a.instructions().empty());
	/// ```
	void reset() noexcept;

	/// Creates a label that can be referenced by instructions
	[[nodiscard]] CodeLabel create_label() noexcept {
		current_label_id_++;
		return CodeLabel(current_label_id_);
	}

	/// Initializes the label to the next instruction
	///
	/// Sets the error (see `has_error()`) if the label wasn't created by `create_label()`, if this method was called
	/// multiple times for the same label, or if the next instruction already has a label.
	///
	/// @param label Label created by `create_label()`. It's updated by this method.
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// CodeLabel label1 = a.create_label();
	/// a.push(rcx);
	/// // The address of this label is the next added instruction
	/// a.set_label(label1);
	/// a.xor_(rcx, rdx);
	/// // Target is the `xor rcx, rdx` instruction
	/// a.je(label1);
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0x51, 0x48, 0x31, 0xD1, 0x74, 0xFB, 0x90}));
	/// ```
	CodeAssembler& set_label(CodeLabel& label);

	/// Creates an anonymous label that can be referenced by calling `bwd()` and `fwd()`
	///
	/// Sets the error (see `has_error()`) if the next instruction already has an anonymous label
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.push(rcx);
	/// // The address of this label is the next added instruction
	/// a.anonymous_label();
	/// a.xor_(rcx, rdx);
	/// // Target is the `xor rcx, rdx` instruction
	/// a.je(a.bwd().value());
	/// // Target is the `sub eax, eax` instruction
	/// a.js(a.fwd().value());
	/// a.nop();
	/// // Create the label referenced by `fwd()` above
	/// a.anonymous_label();
	/// a.sub(eax, eax);
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0x51, 0x48, 0x31, 0xD1, 0x74, 0xFB, 0x78, 0x01, 0x90, 0x29, 0xC0}));
	/// ```
	CodeAssembler& anonymous_label();

	/// Gets the previously created anonymous label created by `anonymous_label()`
	///
	/// Fails if no anonymous label has been created yet
	[[nodiscard]] Result<CodeLabel> bwd() const;

	/// Gets the next anonymous label created by a future call to `anonymous_label()`
	///
	/// It never fails. It returns a `Result` for consistency with `bwd()`.
	[[nodiscard]] Result<CodeLabel> fwd();

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Size of `data` in bytes
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.db({0x16, 0x85, 0x10, 0xA0, 0xFA, 0x9E, 0x11, 0xEB, 0x97, 0x34, 0x3B, 0x7E, 0xB7, 0x2B, 0x92, 0x63, 0x16, 0x85});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 20);
	/// ```
	CodeAssembler& db(const std::uint8_t* data, std::size_t size);
	/// Adds data, see `db(const std::uint8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& db(std::initializer_list<std::uint8_t> data) { return db(data.begin(), data.size()); }
	/// Adds data, see `db(const std::uint8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& db(const std::vector<std::uint8_t>& data) { return db(data.data(), data.size()); }
	/// Adds data, see `db(const std::uint8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& db(const std::uint8_t (&data)[N]) { return db(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.db_i({0x16, -0x7B, 0x10, -0x60, -0x06, -0x62, 0x11, -0x15, -0x69, 0x34, 0x3B, 0x7E, -0x49, 0x2B, -0x6E, 0x63, 0x16, -0x7B});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 20);
	/// ```
	CodeAssembler& db_i(const std::int8_t* data, std::size_t size);
	/// Adds data, see `db_i(const std::int8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& db_i(std::initializer_list<std::int8_t> data) { return db_i(data.begin(), data.size()); }
	/// Adds data, see `db_i(const std::int8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& db_i(const std::vector<std::int8_t>& data) { return db_i(data.data(), data.size()); }
	/// Adds data, see `db_i(const std::int8_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& db_i(const std::int8_t (&data)[N]) { return db_i(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dw({0x4068, 0x7956, 0xFA9F, 0x11EB, 0x9467, 0x77FA, 0x747C, 0xD088, 0x7D7E});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 20);
	/// ```
	CodeAssembler& dw(const std::uint16_t* data, std::size_t size);
	/// Adds data, see `dw(const std::uint16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dw(std::initializer_list<std::uint16_t> data) { return dw(data.begin(), data.size()); }
	/// Adds data, see `dw(const std::uint16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dw(const std::vector<std::uint16_t>& data) { return dw(data.data(), data.size()); }
	/// Adds data, see `dw(const std::uint16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dw(const std::uint16_t (&data)[N]) { return dw(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dw_i({0x4068, 0x7956, -0x0561, 0x11EB, -0x6B99, 0x77FA, 0x747C, -0x2F78, 0x7D7E});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 20);
	/// ```
	CodeAssembler& dw_i(const std::int16_t* data, std::size_t size);
	/// Adds data, see `dw_i(const std::int16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dw_i(std::initializer_list<std::int16_t> data) { return dw_i(data.begin(), data.size()); }
	/// Adds data, see `dw_i(const std::int16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dw_i(const std::vector<std::int16_t>& data) { return dw_i(data.data(), data.size()); }
	/// Adds data, see `dw_i(const std::int16_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dw_i(const std::int16_t (&data)[N]) { return dw_i(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dd({0x40687956, 0xFA9F11EB, 0x946777FA, 0x747CD088, 0x7D7E7C58});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 22);
	/// ```
	CodeAssembler& dd(const std::uint32_t* data, std::size_t size);
	/// Adds data, see `dd(const std::uint32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd(std::initializer_list<std::uint32_t> data) { return dd(data.begin(), data.size()); }
	/// Adds data, see `dd(const std::uint32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd(const std::vector<std::uint32_t>& data) { return dd(data.data(), data.size()); }
	/// Adds data, see `dd(const std::uint32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dd(const std::uint32_t (&data)[N]) { return dd(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dd_i({0x40687956, -0x0560EE15, -0x6B988806, 0x747CD088, 0x7D7E7C58});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 22);
	/// ```
	CodeAssembler& dd_i(const std::int32_t* data, std::size_t size);
	/// Adds data, see `dd_i(const std::int32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd_i(std::initializer_list<std::int32_t> data) { return dd_i(data.begin(), data.size()); }
	/// Adds data, see `dd_i(const std::int32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd_i(const std::vector<std::int32_t>& data) { return dd_i(data.data(), data.size()); }
	/// Adds data, see `dd_i(const std::int32_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dd_i(const std::int32_t (&data)[N]) { return dd_i(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dd_f32({3.14f, -1234.5678f, 1e12f, -3.14f, 1234.5678f, -1e12f});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 26);
	/// ```
	CodeAssembler& dd_f32(const float* data, std::size_t size);
	/// Adds data, see `dd_f32(const float*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd_f32(std::initializer_list<float> data) { return dd_f32(data.begin(), data.size()); }
	/// Adds data, see `dd_f32(const float*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dd_f32(const std::vector<float>& data) { return dd_f32(data.data(), data.size()); }
	/// Adds data, see `dd_f32(const float*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dd_f32(const float (&data)[N]) { return dd_f32(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dq({0x40687956FA9F11EB, 0x946777FA747CD088, 0x7D7E7C5814C2BA6E});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 26);
	/// ```
	CodeAssembler& dq(const std::uint64_t* data, std::size_t size);
	/// Adds data, see `dq(const std::uint64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq(std::initializer_list<std::uint64_t> data) { return dq(data.begin(), data.size()); }
	/// Adds data, see `dq(const std::uint64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq(const std::vector<std::uint64_t>& data) { return dq(data.data(), data.size()); }
	/// Adds data, see `dq(const std::uint64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dq(const std::uint64_t (&data)[N]) { return dq(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dq_i({0x40687956FA9F11EB, -0x6B9888058B832F78, 0x7D7E7C5814C2BA6E});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 26);
	/// ```
	CodeAssembler& dq_i(const std::int64_t* data, std::size_t size);
	/// Adds data, see `dq_i(const std::int64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq_i(std::initializer_list<std::int64_t> data) { return dq_i(data.begin(), data.size()); }
	/// Adds data, see `dq_i(const std::int64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq_i(const std::vector<std::int64_t>& data) { return dq_i(data.data(), data.size()); }
	/// Adds data, see `dq_i(const std::int64_t*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dq_i(const std::int64_t (&data)[N]) { return dq_i(data, N); }

	/// Adds data
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param data Data that will be added at the current position
	/// @param size Number of elements in `data`
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.int3();
	/// a.dq_f64({3.14, -1234.5678, 1e123});
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 26);
	/// ```
	CodeAssembler& dq_f64(const double* data, std::size_t size);
	/// Adds data, see `dq_f64(const double*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq_f64(std::initializer_list<double> data) { return dq_f64(data.begin(), data.size()); }
	/// Adds data, see `dq_f64(const double*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	CodeAssembler& dq_f64(const std::vector<double>& data) { return dq_f64(data.data(), data.size()); }
	/// Adds data, see `dq_f64(const double*, std::size_t)`
	///
	/// @param data Data that will be added at the current position
	template <std::size_t N>
	CodeAssembler& dq_f64(const double (&data)[N]) { return dq_f64(data, N); }

	/// Adds nops, preferring long nops
	///
	/// Sets the error (see `has_error()`) if an error was detected (eg. there are pending prefixes)
	///
	/// @param size Size in bytes of all nops
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.nops_with_size(17);
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert(bytes.size() == 17);
	/// ```
	CodeAssembler& nops_with_size(std::size_t size);

	/// Adds an instruction created by the decoder or by `Instruction::with*()` methods
	///
	/// Sets the error (see `has_error()`) if an error was detected
	///
	/// @param instruction Instruction to add
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// a.nop();
	/// a.add_instruction(Instruction::with1(Code::Push_r64, Register::RCX).value());
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0x90, 0x51}));
	/// ```
	CodeAssembler& add_instruction(const Instruction& instruction) { return add_instr(instruction); }

	/// Encodes all added instructions and returns the result
	///
	/// Fails if an error was detected (eg. an invalid instruction operand, see also `has_error()`)
	///
	/// @param ip Base address of all instructions
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// CodeLabel label1 = a.create_label();
	/// a.push(rcx);
	/// // The address of this label is the next added instruction
	/// a.set_label(label1);
	/// a.xor_(rcx, rdx);
	/// // Target is the `xor rcx, rdx` instruction
	/// a.je(label1);
	/// a.nop();
	/// auto bytes = a.assemble(0x12345678).value();
	/// assert((bytes == std::vector<std::uint8_t>{0x51, 0x48, 0x31, 0xD1, 0x74, 0xFB, 0x90}));
	/// ```
	[[nodiscard]] Result<std::vector<std::uint8_t>> assemble(std::uint64_t ip);

	/// Encodes all added instructions and returns the result
	///
	/// Fails if an error was detected (eg. an invalid instruction operand, see also `has_error()`)
	///
	/// @param ip Base address of all instructions
	/// @param options `BlockEncoderOptions` flags
	///
	/// # Examples
	///
	/// ```cpp
	/// CodeAssembler a(64);
	/// CodeLabel label1 = a.create_label();
	/// a.push(rcx);
	/// // The address of this label is the next added instruction
	/// a.set_label(label1);
	/// a.xor_(rcx, rdx);
	/// // Target is the `xor rcx, rdx` instruction
	/// a.je(label1);
	/// a.nop();
	/// auto result = a.assemble_options(0x12345678, BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS).value();
	/// assert(result.inner.rip == 0x12345678);
	/// // Get the address of the label, requires `BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS`
	/// assert(result.label_ip(label1).value() == 0x12345679);
	/// ```
	[[nodiscard]] Result<CodeAssemblerResult> assemble_options(std::uint64_t ip, std::uint32_t options);

	/// `CALL FAR` instruction
	///
	/// Instruction | Opcode | CPUID
	/// ------------|--------|------
	/// `CALL ptr16:16` | `o16 9A cd` | `8086+`
	/// `CALL ptr16:32` | `o32 9A cp` | `386+`
	///
	/// Sets the error (see `has_error()`) if an operand is invalid (basic checks only)
	///
	/// @param selector Selector/segment
	/// @param offset Offset within the segment
	CodeAssembler& call_far(std::uint16_t selector, std::uint32_t offset);

	/// `JMP FAR` instruction
	///
	/// Instruction | Opcode | CPUID
	/// ------------|--------|------
	/// `JMP ptr16:16` | `o16 EA cd` | `8086+`
	/// `JMP ptr16:32` | `o32 EA cp` | `386+`
	///
	/// Sets the error (see `has_error()`) if an operand is invalid (basic checks only)
	///
	/// @param selector Selector/segment
	/// @param offset Offset within the segment
	CodeAssembler& jmp_far(std::uint16_t selector, std::uint32_t offset);

	/// `XLATB` instruction
	///
	/// Instruction | Opcode | CPUID
	/// ------------|--------|------
	/// `XLATB` | `D7` | `8086+`
	///
	/// Sets the error (see `has_error()`) if an operand is invalid (basic checks only)
	CodeAssembler& xlatb();

private:
	bool decl_data_verify_no_prefixes();
	const std::uint8_t* get_nop_bytes(std::size_t size) const noexcept;
};

} // namespace iced_x86::code_asm
