// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "iced_x86/code.hpp"
#include "iced_x86/code_ext.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/fast_formatter_options.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/internal/fast_fmt.hpp"
#include "iced_x86/internal/fmt_utils_all.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include "iced_x86/rounding_control.hpp"
#include "iced_x86/symbol_resolver.hpp"

namespace iced_x86 {

/// A struct that allows you to hard code some formatter options which can make
/// the formatter faster and use less code. Derive from it and hide (re-define) the
/// constants/functions you want to change.
///
/// The derived struct can return hard coded values and/or return a value from the
/// passed in options. If it returns the value from the passed in options, that option can
/// be modified at runtime by calling `formatter.options_mut().set_<option>(new_value)`,
/// else it's ignored and calling that method has no effect (except wasting CPU cycles).
///
/// Every function must be a pure function and must return a value from the `options` input or
/// a literal (`true` or `false`). Returning a literal is recommended since the compiler can
/// remove unused formatter code.
///
/// # Fastest possible disassembly
///
/// For fastest possible disassembly, you should set `ENABLE_DB_DW_DD_DQ` to `false`
/// and you should also hide `verify_output_has_enough_bytes_left()` and return `false`.
///
/// ```cpp
/// struct MyTraitOptions : iced_x86::SpecializedFormatterTraitOptions {
///     // If you never create a db/dw/dd/dq 'instruction', we don't need this feature.
///     static constexpr bool ENABLE_DB_DW_DD_DQ = false;
///     // For a few percent faster code, you can also hide `verify_output_has_enough_bytes_left()` and return `false`
///     // static constexpr bool verify_output_has_enough_bytes_left() noexcept { return false; }
/// };
/// using MyFormatter = iced_x86::SpecializedFormatter<MyTraitOptions>;
///
/// // Assume this is a big array and not just one instruction
/// const std::uint8_t bytes[] = {0x62, 0xF2, 0x4F, 0xDD, 0x72, 0x50, 0x01};
/// iced_x86::Decoder decoder(64, bytes, iced_x86::DecoderOptions::NONE);
///
/// char output[MyFormatter::MAX_FMT_INSTR_LEN + 1];
/// iced_x86::Instruction instruction;
/// MyFormatter formatter;
/// while (decoder.can_decode()) {
///     decoder.decode_out(instruction);
///     std::size_t len = formatter.format(instruction, output);
///     // do something with 'output' here (NUL terminated, `len` chars)
/// }
/// ```
///
/// See `SpecializedFormatter<TraitOptions>` for more examples
struct SpecializedFormatterTraitOptions {
	/// NOT PART OF THE PUBLIC API.
	/// It's used by the formatter to detect `FastFormatter` so its speed doesn't regress
	/// when we optimize `SpecializedFormatter` with hard coded options.
	static constexpr bool INTERNAL_IS_FAST_FORMATTER = false;

	/// Enables support for a symbol resolver. This is disabled by default. If this
	/// is disabled, you must not pass in a symbol resolver to the constructor.
	///
	/// For fastest code, this should be *disabled*, not enabled.
	static constexpr bool ENABLE_SYMBOL_RESOLVER = false;

	/// Enables support for formatting `db`, `dw`, `dd`, `dq`.
	///
	/// For fastest code, this should be *disabled*, not enabled.
	static constexpr bool ENABLE_DB_DW_DD_DQ = false;

	/// The formatter writes to a buffer that has at least `MAX_FMT_INSTR_LEN + 1` bytes (the caller's buffer or
	/// a buffer on the stack if the caller's buffer is smaller or if a symbol resolver is used). This is enough
	/// space for all formatted instructions (+ the extra bytes written by its fast string copies).
	///
	/// *No formatted instruction will ever get close to being `MAX_FMT_INSTR_LEN` (315) bytes long!*
	///
	/// If this function returns `true`, every write is verified (it calls `std::abort()` if there's not
	/// enough space, this should never happen). If it returns `false`, the formatter won't verify that it has
	/// enough bytes left when writing to its output buffer.
	///
	/// For fastest code, this method should return `false`. Default is `true`.
	static constexpr bool verify_output_has_enough_bytes_left() noexcept { return true; }

	/// Add a space after the operand separator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov rax, rcx`
	/// 👍 | `false` | `mov rax,rcx`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool space_after_operand_separator(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return false;
	}

	/// Show `RIP+displ` or the virtual address
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `mov eax,[rip+12345678h]`
	/// _ | `false` | `mov eax,[1029384756AFBECDh]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool rip_relative_addresses(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return true;
	}

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// 👍 | `false` | `vcmpsd xmm2,xmm6,xmm3,5h`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool use_pseudo_ops(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return false;
	}

	/// Show the original value after the symbol name
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[myfield (12345678)]`
	/// 👍 | `false` | `mov eax,[myfield]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool show_symbol_address(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return false;
	}

	/// Always show the effective segment register. If the option is `false`, only show the segment register if
	/// there's a segment override prefix.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ds:[ecx]`
	/// 👍 | `false` | `mov eax,[ecx]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool always_show_segment_register(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return false;
	}

	/// Always show the size of memory operands
	///
	/// Default | Value | Example | Example
	/// --------|-------|---------|--------
	/// _ | `true` | `mov eax,dword ptr [ebx]` | `add byte ptr [eax],0x12`
	/// 👍 | `false` | `mov eax,[ebx]` | `add byte ptr [eax],0x12`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool always_show_memory_size(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return false;
	}

	/// Use uppercase hex digits
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0xFF`
	/// _ | `false` | `0xff`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool uppercase_hex(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return true;
	}

	/// Use a hex prefix (`0x`) or a hex suffix (`h`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0x5A`
	/// _ | `false` | `5Ah`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool use_hex_prefix(const FastFormatterOptions& options) noexcept {
		static_cast<void>(options);
		return true;
	}
};

/// Default `SpecializedFormatter<TraitOptions>` options. It doesn't override any constant or function
struct DefaultSpecializedFormatterTraitOptions : SpecializedFormatterTraitOptions {};

/// Default `FastFormatter` options
struct DefaultFastFormatterTraitOptions : SpecializedFormatterTraitOptions {
	/// DO NOT USE: NOT PART OF THE PUBLIC API
	static constexpr bool INTERNAL_IS_FAST_FORMATTER = true;

	/// Set to `true` so symbol resolvers can be used
	static constexpr bool ENABLE_SYMBOL_RESOLVER = true;

	/// Enables support for formatting `db`, `dw`, `dd`, `dq`.
	///
	/// For fastest code, this should be *disabled*, not enabled.
	static constexpr bool ENABLE_DB_DW_DD_DQ = true;

	/// Add a space after the operand separator
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov rax, rcx`
	/// 👍 | `false` | `mov rax,rcx`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool space_after_operand_separator(const FastFormatterOptions& options) noexcept { return options.space_after_operand_separator(); }

	/// Show `RIP+displ` or the virtual address
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[rip+12345678h]`
	/// 👍 | `false` | `mov eax,[1029384756AFBECDh]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool rip_relative_addresses(const FastFormatterOptions& options) noexcept { return options.rip_relative_addresses(); }

	/// Use pseudo instructions
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `vcmpnltsd xmm2,xmm6,xmm3`
	/// _ | `false` | `vcmpsd xmm2,xmm6,xmm3,5h`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool use_pseudo_ops(const FastFormatterOptions& options) noexcept { return options.use_pseudo_ops(); }

	/// Show the original value after the symbol name
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,[myfield (12345678)]`
	/// 👍 | `false` | `mov eax,[myfield]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool show_symbol_address(const FastFormatterOptions& options) noexcept { return options.show_symbol_address(); }

	/// Always show the effective segment register. If the option is `false`, only show the segment register if
	/// there's a segment override prefix.
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `mov eax,ds:[ecx]`
	/// 👍 | `false` | `mov eax,[ecx]`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool always_show_segment_register(const FastFormatterOptions& options) noexcept { return options.always_show_segment_register(); }

	/// Always show the size of memory operands
	///
	/// Default | Value | Example | Example
	/// --------|-------|---------|--------
	/// _ | `true` | `mov eax,dword ptr [ebx]` | `add byte ptr [eax],0x12`
	/// 👍 | `false` | `mov eax,[ebx]` | `add byte ptr [eax],0x12`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool always_show_memory_size(const FastFormatterOptions& options) noexcept { return options.always_show_memory_size(); }

	/// Use uppercase hex digits
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// 👍 | `true` | `0xFF`
	/// _ | `false` | `0xff`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool uppercase_hex(const FastFormatterOptions& options) noexcept { return options.uppercase_hex(); }

	/// Use a hex prefix (`0x`) or a hex suffix (`h`)
	///
	/// Default | Value | Example
	/// --------|-------|--------
	/// _ | `true` | `0x5A`
	/// 👍 | `false` | `5Ah`
	///
	/// # Arguments
	///
	/// * `options`: Current formatter options
	static constexpr bool use_hex_prefix(const FastFormatterOptions& options) noexcept { return options.use_hex_prefix(); }
};

/// Fast specialized formatter with less formatting options and with a masm-like syntax.
/// Use it if formatting speed is more important than being able to re-assemble formatted instructions.
///
/// The `TraitOptions` template parameter is a `SpecializedFormatterTraitOptions` (derived) struct. It can
/// be used to hard code options so the compiler can create a smaller and faster formatter.
/// See also `FastFormatter` which allows changing the options at runtime at the cost of
/// being a little bit slower and using a little bit more code.
///
/// This formatter is ~3.3x faster than the gas/intel/masm/nasm formatters (the time includes decoding + formatting).
///
/// # Examples
///
/// ```cpp
/// const std::uint8_t bytes[] = {0x62, 0xF2, 0x4F, 0xDD, 0x72, 0x50, 0x01};
/// iced_x86::Decoder decoder(64, bytes, iced_x86::DecoderOptions::NONE);
/// auto instr = decoder.decode();
///
/// // If you like the default options, you can also use DefaultSpecializedFormatterTraitOptions
/// // instead of deriving your own options struct.
/// struct MyTraitOptions : iced_x86::SpecializedFormatterTraitOptions {
///     static constexpr bool space_after_operand_separator(const iced_x86::FastFormatterOptions&) noexcept {
///         // We hard code the value to `true` which means it's not possible to
///         // change this option at runtime, i.e., this will do nothing:
///         //      formatter.options_mut().set_space_after_operand_separator(false);
///         return true;
///     }
///     static constexpr bool rip_relative_addresses(const iced_x86::FastFormatterOptions& options) noexcept {
///         // Since we return the input, we can change this value at runtime, i.e.,
///         // this works:
///         //      formatter.options_mut().set_rip_relative_addresses(false);
///         return options.rip_relative_addresses();
///     }
/// };
/// using MyFormatter = iced_x86::SpecializedFormatter<MyTraitOptions>;
///
/// char output[MyFormatter::MAX_FMT_INSTR_LEN + 1];
/// MyFormatter formatter;
/// formatter.format(instr, output);
/// // output == "vcvtne2ps2bf16 zmm2{k5}{z}, zmm6, dword bcst [rax+0x4]"
/// ```
///
/// # Using a symbol resolver
///
/// The symbol resolver is disabled by default, but it's easy to enable it (or you can just use `FastFormatter`)
///
/// ```cpp
/// struct MyTraitOptions : iced_x86::SpecializedFormatterTraitOptions {
///     static constexpr bool ENABLE_SYMBOL_RESOLVER = true;
/// };
/// using MyFormatter = iced_x86::SpecializedFormatter<MyTraitOptions>;
///
/// class MySymbolResolver final : public iced_x86::SymbolResolver {
/// public:
///     std::optional<iced_x86::SymbolResult> symbol(const iced_x86::Instruction&, std::uint32_t, std::optional<std::uint32_t>,
///                                                  std::uint64_t address, std::uint32_t) override {
///         if (address == 0x5AA55AA5)
///             // The 'address' arg is the address of the symbol and doesn't have to be identical
///             // to the 'address' arg passed to symbol(). If it's different from the input
///             // address, the formatter will add +N or -N, eg. '[rax+symbol+123]'
///             return iced_x86::SymbolResult::with_str(address, "my_data");
///         return std::nullopt;
///     }
/// };
///
/// auto formatter = MyFormatter::try_with_options(std::make_unique<MySymbolResolver>()).value();
/// // mov rcx,[rdx+my_data]
/// ```
///
/// Symbols can have any length so the formatted instruction can be longer than `MAX_FMT_INSTR_LEN` chars: `format()`
/// returns the full length and truncates the output if the buffer is too small (same as `snprintf()`).
///
/// Far branches (`selector:offset`): same as Rust, the symbol resolver is called for the offset and if it returns a
/// symbol, it's called for the selector. The formatter doesn't allocate memory so it can't copy the first result (the
/// second call can invalidate its borrowed strings), it calls the symbol resolver again for the offset instead (Rust
/// copies the first result to the heap). The symbol resolver should return the same symbol if it's called again.
template <typename TraitOptions>
class SpecializedFormatter {
public:
	/// Max length of a formatted instruction (not including the terminating NUL char) if no symbol resolver is used.
	/// A buffer of `MAX_FMT_INSTR_LEN + 1` bytes (eg. `char buffer[iced_x86::FastFormatter::MAX_FMT_INSTR_LEN + 1]`, a small stack
	/// or static buffer) is always big enough, see `format()`.
	static constexpr std::size_t MAX_FMT_INSTR_LEN = internal::fast::MAX_FMT_INSTR_LEN;

	/// Creates a new instance of this formatter. It doesn't allocate any memory (the formatter's tables are constant data).
	SpecializedFormatter() noexcept : options_(), symbol_resolver_(), limit_(nullptr), output_(nullptr) {}

	/// Creates a new instance of this formatter
	///
	/// # Errors
	///
	/// Fails if `TraitOptions::ENABLE_SYMBOL_RESOLVER` is `false` and `symbol_resolver` isn't null
	///
	/// # Arguments
	///
	/// - `symbol_resolver`: Symbol resolver or null
	static Result<SpecializedFormatter> try_with_options(std::unique_ptr<SymbolResolver> symbol_resolver) {
		if (!TraitOptions::ENABLE_SYMBOL_RESOLVER && symbol_resolver)
			return IcedError("TraitOptions::ENABLE_SYMBOL_RESOLVER is disabled so symbol resolvers aren't supported");
		SpecializedFormatter formatter;
		formatter.symbol_resolver_ = std::move(symbol_resolver);
		return Result<SpecializedFormatter>(std::move(formatter));
	}

	SpecializedFormatter(SpecializedFormatter&&) noexcept = default;
	SpecializedFormatter& operator=(SpecializedFormatter&&) noexcept = default;
	SpecializedFormatter(const SpecializedFormatter&) = delete;
	SpecializedFormatter& operator=(const SpecializedFormatter&) = delete;

	/// Gets the formatter options (immutable)
	///
	/// Note that the `TraitOptions` template parameter can override any option and hard code them,
	/// see `SpecializedFormatterTraitOptions`
	const FastFormatterOptions& options() const noexcept { return options_; }

	/// Gets the formatter options (mutable)
	///
	/// Note that the `TraitOptions` template parameter can override any option and hard code them,
	/// see `SpecializedFormatterTraitOptions`
	FastFormatterOptions& options_mut() noexcept { return options_; }

	/// Formats the whole instruction: prefixes, mnemonic, operands. The formatted instruction is written to `output` and
	/// it's always NUL terminated (unless `output_size` is 0). It never allocates memory.
	///
	/// Returns the length of the formatted instruction (not including the NUL char). Same as `snprintf()`: if the return
	/// value is `>= output_size`, the output was truncated (`output` has the first `output_size - 1` chars + a NUL char).
	/// Call it again with a buffer of at least `return value + 1` bytes to get the whole string.
	///
	/// If no symbol resolver is used, the formatted instruction is never longer than `MAX_FMT_INSTR_LEN` chars, so a buffer
	/// of `MAX_FMT_INSTR_LEN + 1` bytes is always big enough. That's also the fast path: if `output_size > MAX_FMT_INSTR_LEN`
	/// and there's no symbol resolver, the formatter writes directly to `output` without checking the size of each write.
	/// Otherwise (smaller buffer or a symbol resolver is used, symbols can have any length) it formats to a buffer on
	/// the stack (`MAX_FMT_INSTR_LEN + 1` bytes) and copies the text to `output` (and the symbol names, they're copied
	/// directly to `output`).
	///
	/// Bytes after the NUL char are unspecified: the formatter can write to any byte of `output[0..output_size)`.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output buffer (can be null if `output_size` is 0)
	/// - `output_size`: Size of `output` in bytes (incl. the NUL char)
	std::size_t format(const Instruction& instruction, char* output, std::size_t output_size) {
		if (ICED_X86_INTERNAL_LIKELY(output_size > MAX_FMT_INSTR_LEN && !has_symbol_resolver())) {
			auto* const dst = reinterpret_cast<std::uint8_t*>(output);
			// The last byte is reserved for the NUL char
			std::uint8_t* const dst_end = format_core(instruction, dst, dst + (output_size - 1));
			*dst_end = 0;
			return static_cast<std::size_t>(dst_end - dst);
		}
		return format_slow(instruction, output, output_size);
	}

	/// Formats the whole instruction: prefixes, mnemonic, operands. See `format(const Instruction&, char*, std::size_t)`
	///
	/// ```cpp
	/// char buffer[iced_x86::FastFormatter::MAX_FMT_INSTR_LEN + 1];
	/// std::size_t len = formatter.format(instruction, buffer);
	/// ```
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: Output buffer
	template <std::size_t N>
	std::size_t format(const Instruction& instruction, char (&output)[N]) {
		return format(instruction, output, N);
	}

	/// Formats the whole instruction: prefixes, mnemonic, operands and appends it to `output`.
	///
	/// This is a convenience wrapper (Rust API parity) around `format(const Instruction&, char*, std::size_t)`. It's not
	/// part of the library (the formatter itself never uses `std::string`): it formats to a `MAX_FMT_INSTR_LEN + 1` byte
	/// buffer on the stack and appends it to `output`. If the formatted instruction is longer (only possible if a symbol
	/// resolver returns long symbols), it formats the instruction again directly into `output` (the symbol resolver is
	/// called again). Prefer the buffer API if you don't need a `std::string`.
	///
	/// # Arguments
	///
	/// - `instruction`: Instruction
	/// - `output`: The formatted instruction is appended to this string
	void format(const Instruction& instruction, std::string& output) {
		char buffer[MAX_FMT_INSTR_LEN + 1];
		const std::size_t len = format(instruction, buffer, sizeof(buffer));
		if (ICED_X86_INTERNAL_LIKELY(len < sizeof(buffer)))
			output.append(buffer, len);
		else {
			const std::size_t old_size = output.size();
			output.resize(old_size + len + 1);
			const std::size_t len2 = format(instruction, &output[old_size], len + 1);
			// Remove the NUL char (the new length could be different if the symbol resolver returned another symbol)
			output.resize(old_size + (len2 < len ? len2 : len));
		}
	}

private:
	using FastString4 = internal::fast::FastString4;
	using FastString8 = internal::fast::FastString8;
	using FastString12 = internal::fast::FastString12;
	using FastStringMnemonic = internal::fast::FastStringMnemonic;
	using FastStringMemorySize = internal::fast::FastStringMemorySize;
	using FastStringRegister = internal::fast::FastStringRegister;

	static constexpr bool SHOW_USELESS_PREFIXES = true;

	bool has_symbol_resolver() const noexcept {
		if constexpr (TraitOptions::ENABLE_SYMBOL_RESOLVER)
			return static_cast<bool>(symbol_resolver_);
		else
			return false;
	}

	// Formats the instruction to `dst_next_p`, all writes must be < `limit`. Returns the end of the formatted text
	std::uint8_t* format_core(const Instruction& instruction, std::uint8_t* dst_next_p, std::uint8_t* limit);
	// Formats the instruction to a scratch buffer on the stack (small `output` buffer or a symbol resolver is used)
	ICED_X86_INTERNAL_NOINLINE std::size_t format_slow(const Instruction& instruction, char* output, std::size_t output_size);

	ICED_X86_INTERNAL_FORCE_INLINE void verify_bytes_left(const std::uint8_t* dst_next_p, std::size_t num_bytes) const noexcept {
		if (TraitOptions::verify_output_has_enough_bytes_left()) {
			// Verify that there's enough bytes left. This should never fail.
			if (static_cast<std::size_t>(limit_ - dst_next_p) < num_bytes)
				internal::fast::fast_fmt_assert_failed();
		}
	}

	// Writes the whole FastString (SIZE bytes) and returns the new position (after the string)
	template <std::size_t SIZE>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_fast_str(std::uint8_t* dst_next_p, const std::uint8_t* len_data) const noexcept {
		verify_bytes_left(dst_next_p, SIZE);
		std::memcpy(dst_next_p, len_data + 1, SIZE);
		return dst_next_p + len_data[0];
	}

	template <std::size_t SIZE>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_fast_str(std::uint8_t* dst_next_p, internal::fast::FastString<SIZE> s) const noexcept {
		return write_fast_str<SIZE>(dst_next_p, s.len_data);
	}

	template <std::size_t N>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_fast_str(std::uint8_t* dst_next_p, const std::array<std::uint8_t, N>& s) const noexcept {
		return write_fast_str<N - 1>(dst_next_p, s.data());
	}

	template <bool CHECK_LIMIT>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_fast_ascii_char(std::uint8_t* dst_next_p, char ch) const noexcept {
		if (CHECK_LIMIT)
			verify_bytes_left(dst_next_p, 1);
		*dst_next_p = static_cast<std::uint8_t>(ch);
		return dst_next_p + 1;
	}

	// Writes 2 hex digits (reads and writes 4 bytes) and returns the new position (after the 2 hex digits)
	template <bool CHECK_LIMIT>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_fast_hex2_rw_4bytes(std::uint8_t* dst_next_p, std::uint32_t value,
																			std::uint32_t lower_or_value) const noexcept {
		constexpr std::size_t DATA_LEN = 4;
		constexpr std::size_t REAL_LEN = 2;
		if (CHECK_LIMIT)
			verify_bytes_left(dst_next_p, DATA_LEN);
		// lower_or_value == 0 if we should use uppercase hex digits or 0x2020_2020 to use lowercase hex digits.
		std::uint32_t tmp;
		std::memcpy(&tmp, internal::fast::HEX_GROUP2_UPPER + static_cast<std::size_t>(value) * REAL_LEN, DATA_LEN);
		tmp |= lower_or_value;
		std::memcpy(dst_next_p, &tmp, DATA_LEN);
		return dst_next_p + REAL_LEN;
	}

	// Only one caller so inline it
	static ICED_X86_INTERNAL_FORCE_INLINE bool show_segment_prefix(const Instruction& instruction, std::uint32_t op_count) noexcept {
		for (std::uint32_t i = 0; i < op_count; i++) {
			switch (instruction.op_kind(i)) {
			case OpKind::Register:
			case OpKind::NearBranch16:
			case OpKind::NearBranch32:
			case OpKind::NearBranch64:
			case OpKind::FarBranch16:
			case OpKind::FarBranch32:
			case OpKind::Immediate8:
			case OpKind::Immediate8_2nd:
			case OpKind::Immediate16:
			case OpKind::Immediate32:
			case OpKind::Immediate64:
			case OpKind::Immediate8to16:
			case OpKind::Immediate8to32:
			case OpKind::Immediate8to64:
			case OpKind::Immediate32to64:
			case OpKind::MemoryESDI:
			case OpKind::MemoryESEDI:
			case OpKind::MemoryESRDI:
				break;

			case OpKind::MemorySegSI:
			case OpKind::MemorySegESI:
			case OpKind::MemorySegRSI:
			case OpKind::MemorySegDI:
			case OpKind::MemorySegEDI:
			case OpKind::MemorySegRDI:
			case OpKind::Memory:
			default:
				return false;
			}
		}

		return SHOW_USELESS_PREFIXES;
	}

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* format_register(std::uint8_t* dst_next_p, Register register_) const noexcept {
		return write_fast_str<FastStringRegister::SIZE>(dst_next_p, internal::fast::REGISTERS[static_cast<std::size_t>(register_)]);
	}

	template <bool UPPERCASE_HEX, bool USE_HEX_PREFIX>
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* format_number_impl(std::uint8_t* dst_next_p, std::uint64_t value) const noexcept;

	std::uint8_t* format_number(std::uint8_t* dst_next_p, std::uint64_t value) const noexcept;

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* write_symbol(std::uint8_t* dst_next_p, std::uint64_t address,
															   const SymbolResult& symbol) const {
		return write_symbol2(dst_next_p, address, symbol, true);
	}

	ICED_X86_INTERNAL_COLD std::uint8_t* write_symbol2(std::uint8_t* dst_next_p, std::uint64_t address, const SymbolResult& symbol,
													   bool write_minus_if_signed) const;

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* format_memory_else_block(std::uint8_t* dst_next_p, bool need_plus, std::uint32_t displ_size,
																		   std::int64_t displ, std::uint32_t addr_size) const noexcept {
		if (!need_plus || (displ_size != 0 && displ != 0)) {
			if (need_plus) {
				char c;
				if (addr_size == 8) {
					if (displ < 0) {
						displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
						c = '-';
					}
					else
						c = '+';
				}
				else if (addr_size == 4) {
					if (static_cast<std::int32_t>(displ) < 0) {
						displ = static_cast<std::int64_t>(static_cast<std::uint32_t>(0U - static_cast<std::uint32_t>(displ)));
						c = '-';
					}
					else
						c = '+';
				}
				else {
					if (static_cast<std::int16_t>(displ) < 0) {
						displ = static_cast<std::int64_t>(static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(displ)));
						c = '-';
					}
					else
						c = '+';
				}
				dst_next_p = write_fast_ascii_char<true>(dst_next_p, c);
			}
			dst_next_p = format_number(dst_next_p, static_cast<std::uint64_t>(displ));
		}
		return dst_next_p;
	}

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* format_memory_code(std::uint8_t* dst_next_p, const Instruction& instruction,
																	 std::uint32_t operand, Register seg_reg, Register base_reg, Register index_reg,
																	 std::uint32_t scale, std::uint32_t displ_size, std::int64_t displ,
																	 std::uint32_t addr_size);

	ICED_X86_INTERNAL_NOINLINE std::uint8_t* format_memory(std::uint8_t* dst_next_p, const Instruction& instruction,
														   std::uint32_t operand, Register seg_reg, Register base_reg, Register index_reg, std::uint32_t scale,
														   std::uint32_t displ_size, std::int64_t displ, std::uint32_t addr_size) {
		return format_memory_code(dst_next_p, instruction, operand, seg_reg, base_reg, index_reg, scale, displ_size, displ, addr_size);
	}

	// This speeds up SpecializedFormatter but slows down FastFormatter so detect which
	// formatter it is. Both paths are tested (same tests).
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* call_format_memory(std::uint8_t* dst_next_p, const Instruction& instruction,
																	 std::uint32_t operand, Register seg_reg, Register base_reg, Register index_reg,
																	 std::uint32_t scale, std::uint32_t displ_size, std::int64_t displ,
																	 std::uint32_t addr_size) {
		if constexpr (TraitOptions::INTERNAL_IS_FAST_FORMATTER) {
			// Less code: call a method
			return format_memory(dst_next_p, instruction, operand, seg_reg, base_reg, index_reg, scale, displ_size, displ, addr_size);
		}
		else {
			// The options are all most likely hard coded so inline and specialize the 'method call'
			return format_memory_code(dst_next_p, instruction, operand, seg_reg, base_reg, index_reg, scale, displ_size, displ, addr_size);
		}
	}

	// The symbol resolver code is in separate (not inlined) methods so the normal code (no symbols) is fast and uses less stack space
	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* fmt_near_branch(std::uint8_t* dst_next_p, const Instruction& instruction,
																  std::uint32_t operand, std::uint32_t imm_size, std::uint64_t imm) {
		if constexpr (TraitOptions::ENABLE_SYMBOL_RESOLVER) {
			if (symbol_resolver_)
				return fmt_near_branch_symbol(dst_next_p, instruction, operand, imm_size, imm);
		}
		return format_number(dst_next_p, imm);
	}
	ICED_X86_INTERNAL_NOINLINE std::uint8_t* fmt_near_branch_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																	std::uint32_t operand, std::uint32_t imm_size, std::uint64_t imm);

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* fmt_far_branch(std::uint8_t* dst_next_p, const Instruction& instruction,
																 std::uint32_t operand, OpKind op_kind) {
		std::uint32_t imm_size;
		std::uint64_t imm64;
		if (op_kind == OpKind::FarBranch32) {
			imm_size = 4;
			imm64 = instruction.far_branch32();
		}
		else {
			imm_size = 2;
			imm64 = instruction.far_branch16();
		}
		if constexpr (TraitOptions::ENABLE_SYMBOL_RESOLVER) {
			if (symbol_resolver_)
				return fmt_far_branch_symbol(dst_next_p, instruction, operand, imm_size, imm64);
		}
		else {
			static_cast<void>(operand);
			static_cast<void>(imm_size);
		}
		dst_next_p = format_number(dst_next_p, instruction.far_branch_selector());
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, ':');
		dst_next_p = format_number(dst_next_p, imm64);
		return dst_next_p;
	}
	ICED_X86_INTERNAL_NOINLINE std::uint8_t* fmt_far_branch_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																   std::uint32_t operand, std::uint32_t imm_size, std::uint64_t imm64);

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* fmt_imm(std::uint8_t* dst_next_p, const Instruction& instruction,
														  std::uint32_t operand, std::uint64_t imm, std::uint32_t imm_size) {
		if constexpr (TraitOptions::ENABLE_SYMBOL_RESOLVER) {
			if (symbol_resolver_)
				return fmt_imm_symbol(dst_next_p, instruction, operand, imm, imm_size);
		}
		return format_number(dst_next_p, imm);
	}
	ICED_X86_INTERNAL_NOINLINE std::uint8_t* fmt_imm_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
															std::uint32_t operand, std::uint64_t imm, std::uint32_t imm_size);

	ICED_X86_INTERNAL_NOINLINE std::uint8_t* format_memory_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																  std::uint32_t operand, std::uint64_t abs_addr, std::uint32_t addr_size, bool need_plus,
																  std::uint32_t displ_size, std::int64_t displ);

	ICED_X86_INTERNAL_FORCE_INLINE std::uint32_t get_address_size_in_bytes(Register base_reg, Register index_reg, std::uint32_t displ_size,
																			CodeSize code_size) const noexcept {
		const std::uint32_t size = static_cast<std::uint32_t>(internal::REG_TO_ADDR_SIZE[static_cast<std::size_t>(base_reg)]) |
								   static_cast<std::uint32_t>(internal::REG_TO_ADDR_SIZE[static_cast<std::size_t>(index_reg)]);
		if (size != 0)
			return size;
		if (displ_size >= 2)
			return displ_size;
		switch (code_size) {
		case CodeSize::Code64:
			return 8;
		case CodeSize::Code32:
			return 4;
		case CodeSize::Code16:
			return 2;
		default:
			return 8;
		}
	}

	ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* fmt_memory(std::uint8_t* dst_next_p, const Instruction& instruction,
															 std::uint32_t operand, Code code) {
		const std::uint32_t displ_size = instruction.memory_displ_size();
		const Register base_reg = instruction.memory_base();
		Register index_reg = instruction.memory_index();
		const std::uint32_t addr_size = get_address_size_in_bytes(base_reg, index_reg, displ_size, instruction.code_size());
		const std::int64_t displ = addr_size == 8 ? static_cast<std::int64_t>(instruction.memory_displacement64())
												  : static_cast<std::int64_t>(instruction.memory_displacement32());
		if (code == Code::Xlat_m8)
			index_reg = Register::None;
		// scale: 1,2,4,8 -> 0,1,2,3
		const std::uint32_t scale = instruction.memory_index_scale();
		const std::uint32_t scale_index = (scale >> 1) - (scale >> 3);
		dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), base_reg, index_reg, scale_index,
										displ_size, displ, addr_size);
		if (instruction.is_mvex_eviction_hint())
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_EH);
		return dst_next_p;
	}

	FastFormatterOptions options_;
	std::unique_ptr<SymbolResolver> symbol_resolver_;
	// End of the output buffer (only used if `TraitOptions::verify_output_has_enough_bytes_left()` is `true`)
	std::uint8_t* limit_;
	// The output if `format_slow()` is used (always if there's a symbol resolver), else null
	internal::fast::FastFmtOutput* output_;
};

/// Fast formatter with less formatting options and with a masm-like syntax.
/// Use it if formatting speed is more important than being able to re-assemble formatted instructions.
///
/// This is a variant of `SpecializedFormatter<TraitOptions>` and allows changing the
/// formatter options at runtime and the use of a symbol resolver. For fastest possible
/// disassembly and smallest code, the options should be hard coded, so see `SpecializedFormatter<TraitOptions>`.
///
/// This formatter is ~2.8x faster than the gas/intel/masm/nasm formatters (the time includes decoding + formatting).
///
/// # Examples
///
/// ```cpp
/// const std::uint8_t bytes[] = {0x62, 0xF2, 0x4F, 0xDD, 0x72, 0x50, 0x01};
/// iced_x86::Decoder decoder(64, bytes, iced_x86::DecoderOptions::NONE);
/// auto instr = decoder.decode();
///
/// char output[iced_x86::FastFormatter::MAX_FMT_INSTR_LEN + 1];
/// iced_x86::FastFormatter formatter;
/// formatter.options_mut().set_space_after_operand_separator(true);
/// formatter.format(instr, output);
/// // output == "vcvtne2ps2bf16 zmm2{k5}{z}, zmm6, dword bcst [rax+4h]"
/// ```
///
/// # Using a symbol resolver
///
/// ```cpp
/// auto formatter = iced_x86::FastFormatter::try_with_options(std::make_unique<MySymbolResolver>()).value();
/// ```
///
/// See `SpecializedFormatter<TraitOptions>` for a symbol resolver example
using FastFormatter = SpecializedFormatter<DefaultFastFormatterTraitOptions>;

// ---------------------------------------------------------------------------------------------------------------------
// Implementation

template <typename TraitOptions>
template <bool UPPERCASE_HEX, bool USE_HEX_PREFIX>
ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* SpecializedFormatter<TraitOptions>::format_number_impl(std::uint8_t* dst_next_p,
																									 std::uint64_t value) const noexcept {
	const char* hex_table = UPPERCASE_HEX ? internal::fast::HEX_DIGITS_UPPER : internal::fast::HEX_DIGITS_LOWER;
	constexpr std::uint32_t lower_or_value = UPPERCASE_HEX ? 0 : 0x2020'2020;

	if (USE_HEX_PREFIX)
		dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_HEX_PREFIX);

	if (value < 0x10) {
		if (USE_HEX_PREFIX) {
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, hex_table[value]);
			return dst_next_p;
		}
		else {
			// 1 (possible '0' prefix) + 1 (hex digit) + 1 ('h' suffix)
			verify_bytes_left(dst_next_p, 1 + 1 + 1);
			if (value > 9)
				dst_next_p = write_fast_ascii_char<false>(dst_next_p, '0');
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, hex_table[value]);
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, 'h');
			return dst_next_p;
		}
	}
	else if (value < 0x100) {
		if (USE_HEX_PREFIX) {
			dst_next_p = write_fast_hex2_rw_4bytes<true>(dst_next_p, static_cast<std::uint32_t>(value), lower_or_value);
			return dst_next_p;
		}
		else {
			// 1 (possible '0' prefix) + 2 (hex digits) + 2 since
			// write_fast_hex2_rw_4bytes() reads/writes 4 bytes and not 2.
			// '+2' also includes the 'h' suffix.
			verify_bytes_left(dst_next_p, 1 + 2 + 2);
			if (value > 0x9F)
				dst_next_p = write_fast_ascii_char<false>(dst_next_p, '0');
			dst_next_p = write_fast_hex2_rw_4bytes<false>(dst_next_p, static_cast<std::uint32_t>(value), lower_or_value);
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, 'h');
			return dst_next_p;
		}
	}
	else {
		std::size_t rshift = (64 - internal::fast::leading_zeros64(value) + 3) & ~static_cast<std::size_t>(3);

		// The first '1' is an optional '0' prefix.
		// `rshift / 4` == number of hex digits to copy. The last `+ 2` is the extra padding needed
		// since the write_fast_hex2_rw_4bytes() method reads and writes 4 bytes (2 hex digits + 2 bytes padding).
		// '+2' also includes the 'h' suffix.
		verify_bytes_left(dst_next_p, 1 + rshift / 4 + 2);

		if (!USE_HEX_PREFIX && ((value >> (rshift - 4)) & 0xF) > 9)
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, '0');

		// If odd number of hex digits
		if ((rshift & 4) != 0) {
			rshift -= 4;
			const auto digit = static_cast<std::size_t>((value >> rshift) & 0xF);
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, hex_table[digit]);
		}

		// If we're here, value >= 0x100 so rshift >= 8
		for (;;) {
			rshift -= 8;
			const auto digits2 = static_cast<std::uint32_t>((value >> rshift) & 0xFF);
			dst_next_p = write_fast_hex2_rw_4bytes<false>(dst_next_p, digits2, lower_or_value);

			if (rshift == 0)
				break;
		}

		if (!USE_HEX_PREFIX) {
			// We've verified that the buffer had `1 + rshift / 4 + 2` bytes left (see above).
			// The last `+2` is the padding that needed to be there. That's where
			// this 'h' gets written so we don't need to verify the length here
			// because it has at least 2 more bytes left.
			dst_next_p = write_fast_ascii_char<false>(dst_next_p, 'h');
		}

		return dst_next_p;
	}
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::format_number(std::uint8_t* dst_next_p, std::uint64_t value) const noexcept {
	if (TraitOptions::uppercase_hex(options_)) {
		if (TraitOptions::use_hex_prefix(options_)) {
			// 0x12AB
			return format_number_impl<true, true>(dst_next_p, value);
		}
		else {
			// 12ABh
			return format_number_impl<true, false>(dst_next_p, value);
		}
	}
	else {
		if (TraitOptions::use_hex_prefix(options_)) {
			// 0x12ab
			return format_number_impl<false, true>(dst_next_p, value);
		}
		else {
			// 12abh
			return format_number_impl<false, false>(dst_next_p, value);
		}
	}
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::write_symbol2(std::uint8_t* dst_next_p, std::uint64_t address,
																const SymbolResult& symbol, bool write_minus_if_signed) const {
	auto displ = static_cast<std::int64_t>(address - symbol.address);
	if ((symbol.flags & SymbolFlags::SIGNED) != 0) {
		if (write_minus_if_signed)
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, '-');
		displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
	}

	// Write the symbol. The symbol can be any length so copy everything we've written so far to the
	// output and then copy the symbol to it. The scratch buffer is then empty again.
	// There's a symbol resolver so format_slow() is used and output_ isn't null.
	assert(output_ != nullptr);
	internal::fast::FastFmtOutput& out = *output_;
	internal::fast::fast_fmt_append(out, out.scratch, static_cast<std::size_t>(dst_next_p - out.scratch));
	for (const auto& part : symbol.text) {
		const auto s = part.text.as_str();
		internal::fast::fast_fmt_append(out, s.data(), s.size());
	}
	dst_next_p = out.scratch;

	if (displ != 0) {
		char c;
		if (displ < 0) {
			displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
			c = '-';
		}
		else
			c = '+';
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, c);
		dst_next_p = format_number(dst_next_p, static_cast<std::uint64_t>(displ));
	}
	if (TraitOptions::show_symbol_address(options_)) {
		dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_SPACE_PAREN);
		dst_next_p = format_number(dst_next_p, address);
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, ')');
	}

	return dst_next_p;
}

template <typename TraitOptions>
ICED_X86_INTERNAL_FORCE_INLINE std::uint8_t* SpecializedFormatter<TraitOptions>::format_memory_code(
	std::uint8_t* dst_next_p, const Instruction& instruction, std::uint32_t operand, Register seg_reg, Register base_reg,
	Register index_reg, std::uint32_t scale, std::uint32_t displ_size, std::int64_t displ, std::uint32_t addr_size) {
	std::uint64_t abs_addr;
	if (base_reg == Register::RIP) {
		abs_addr = static_cast<std::uint64_t>(displ);
		if (TraitOptions::rip_relative_addresses(options_))
			displ = static_cast<std::int64_t>(static_cast<std::uint64_t>(displ) - instruction.next_ip());
		else
			base_reg = Register::None;
		displ_size = 8;
	}
	else if (base_reg == Register::EIP) {
		abs_addr = static_cast<std::uint32_t>(displ);
		if (TraitOptions::rip_relative_addresses(options_))
			displ = static_cast<std::int32_t>(static_cast<std::uint32_t>(displ) - instruction.next_ip32());
		else
			base_reg = Register::None;
		displ_size = 4;
	}
	else
		abs_addr = static_cast<std::uint64_t>(displ);

	bool show_mem_size = TraitOptions::always_show_memory_size(options_);
	if (!show_mem_size) {
		const std::uint32_t flags = internal::fast::CODE_FLAGS[static_cast<std::size_t>(instruction.code())];
		show_mem_size = (flags & internal::fast::FAST_FMT_FLAGS_FORCE_MEM_SIZE) != 0 || instruction.is_broadcast();
	}
	if (show_mem_size) {
		dst_next_p = write_fast_str<FastStringMemorySize::SIZE>(dst_next_p, internal::fast::MEMORY_SIZES[static_cast<std::size_t>(instruction.memory_size())]);
	}

	bool show_seg = TraitOptions::always_show_segment_register(options_);
	if (!show_seg) {
		const Register seg_override = instruction.segment_prefix();
		if (seg_override != Register::None) {
			bool notrack_prefix = false;
			if (seg_override == Register::DS && internal::is_notrack_prefix_branch(instruction.code())) {
				const CodeSize code_size = instruction.code_size();
				notrack_prefix = !((code_size == CodeSize::Code16 || code_size == CodeSize::Code32) &&
								   (base_reg == Register::BP || base_reg == Register::EBP || base_reg == Register::ESP));
			}
			show_seg = !notrack_prefix && (SHOW_USELESS_PREFIXES || internal::show_segment_prefix_bool(Register::None, instruction, SHOW_USELESS_PREFIXES));
		}
	}
	if (show_seg) {
		dst_next_p = format_register(dst_next_p, seg_reg);
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, ':');
	}
	dst_next_p = write_fast_ascii_char<true>(dst_next_p, '[');

	bool need_plus = false;
	if (base_reg != Register::None) {
		dst_next_p = format_register(dst_next_p, base_reg);
		need_plus = true;
	}

	if (index_reg != Register::None) {
		if (need_plus)
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, '+');
		need_plus = true;

		dst_next_p = format_register(dst_next_p, index_reg);

		// [rsi] = base reg, [rsi*1] = index reg
		if (addr_size != 2 && (scale != 0 || base_reg == Register::None))
			dst_next_p = write_fast_str(dst_next_p, internal::fast::SCALE_NUMBERS[scale]);
	}

	if constexpr (TraitOptions::ENABLE_SYMBOL_RESOLVER) {
		if (symbol_resolver_)
			dst_next_p = format_memory_symbol(dst_next_p, instruction, operand, abs_addr, addr_size, need_plus, displ_size, displ);
		else
			dst_next_p = format_memory_else_block(dst_next_p, need_plus, displ_size, displ, addr_size);
	}
	else {
		static_cast<void>(operand);
		static_cast<void>(abs_addr);
		dst_next_p = format_memory_else_block(dst_next_p, need_plus, displ_size, displ, addr_size);
	}

	dst_next_p = write_fast_ascii_char<true>(dst_next_p, ']');
	return dst_next_p;
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::fmt_near_branch_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																		 std::uint32_t operand, std::uint32_t imm_size, std::uint64_t imm) {
	const auto symbol = symbol_resolver_->symbol(instruction, operand, operand, imm, imm_size);
	if (symbol)
		return write_symbol(dst_next_p, imm, *symbol);
	return format_number(dst_next_p, imm);
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::fmt_far_branch_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																		std::uint32_t operand, std::uint32_t imm_size, std::uint64_t imm64) {
	// Same as Rust: resolve the offset first and only resolve the selector if the offset has a symbol, then write
	// `selector:offset`. The 2nd call can invalidate the borrowed strings of the 1st result so Rust copies it (heap).
	// We don't allocate: the 1st result is only used to check if there's a symbol and the resolver is called again
	// for the offset after the selector has been written.
	if (!symbol_resolver_->symbol(instruction, operand, operand, static_cast<std::uint32_t>(imm64), imm_size)) {
		dst_next_p = format_number(dst_next_p, instruction.far_branch_selector());
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, ':');
		return format_number(dst_next_p, imm64);
	}
	{
		const auto selector_symbol = symbol_resolver_->symbol(instruction, operand + 1, operand, instruction.far_branch_selector(), 2);
		if (selector_symbol)
			dst_next_p = write_symbol(dst_next_p, instruction.far_branch_selector(), *selector_symbol);
		else
			dst_next_p = format_number(dst_next_p, instruction.far_branch_selector());
	}
	dst_next_p = write_fast_ascii_char<true>(dst_next_p, ':');
	const auto symbol = symbol_resolver_->symbol(instruction, operand, operand, static_cast<std::uint32_t>(imm64), imm_size);
	if (symbol)
		return write_symbol(dst_next_p, imm64, *symbol);
	return format_number(dst_next_p, imm64);
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::fmt_imm_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																 std::uint32_t operand, std::uint64_t imm, std::uint32_t imm_size) {
	const auto symbol = symbol_resolver_->symbol(instruction, operand, operand, imm, imm_size);
	if (symbol) {
		if ((symbol->flags & SymbolFlags::RELATIVE) == 0)
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_OFFSET);
		return write_symbol(dst_next_p, imm, *symbol);
	}
	return format_number(dst_next_p, imm);
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::format_memory_symbol(std::uint8_t* dst_next_p, const Instruction& instruction,
																	   std::uint32_t operand, std::uint64_t abs_addr, std::uint32_t addr_size, bool need_plus,
																	   std::uint32_t displ_size, std::int64_t displ) {
	const auto symbol = symbol_resolver_->symbol(instruction, operand, operand, abs_addr, addr_size);
	if (symbol) {
		if (need_plus) {
			const char c = (symbol->flags & SymbolFlags::SIGNED) != 0 ? '-' : '+';
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, c);
		}
		else if ((symbol->flags & SymbolFlags::SIGNED) != 0)
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, '-');

		return write_symbol2(dst_next_p, abs_addr, *symbol, false);
	}
	return format_memory_else_block(dst_next_p, need_plus, displ_size, displ, addr_size);
}

template <typename TraitOptions>
std::size_t SpecializedFormatter<TraitOptions>::format_slow(const Instruction& instruction, char* output, std::size_t output_size) {
	// Everything is written to this buffer (the formatted instruction without the symbols is never longer than
	// MAX_FMT_INSTR_LEN chars) and then copied to the output (with truncation). Symbols can be any length:
	// write_symbol2() copies the scratch buffer and the symbol to the output and then continues at the start
	// of the scratch buffer.
	std::uint8_t scratch[internal::fast::MAX_FMT_INSTR_LEN + 1];
	internal::fast::FastFmtOutput out{output, output_size, 0, scratch};
	output_ = &out;
	const std::uint8_t* const scratch_end = format_core(instruction, scratch, scratch + internal::fast::MAX_FMT_INSTR_LEN);
	output_ = nullptr;
	internal::fast::fast_fmt_append(out, scratch, static_cast<std::size_t>(scratch_end - scratch));
	return internal::fast::fast_fmt_finish(out);
}

template <typename TraitOptions>
std::uint8_t* SpecializedFormatter<TraitOptions>::format_core(const Instruction& instruction, std::uint8_t* dst_next_p, std::uint8_t* limit) {
	if (TraitOptions::verify_output_has_enough_bytes_left())
		limit_ = limit;
	else
		static_cast<void>(limit);

	const Code code = instruction.code();
	FastStringMnemonic mnemonic{&internal::fast::MNEMONICS[internal::fast::MNEMONIC_OFFSETS[static_cast<std::size_t>(code)]]};
	std::uint32_t op_count = instruction.op_count();
	if (TraitOptions::use_pseudo_ops(options_)) {
		const std::uint32_t flags = internal::fast::CODE_FLAGS[static_cast<std::size_t>(code)];
		const std::uint32_t pseudo_ops_num = flags >> internal::fast::FAST_FMT_FLAGS_PSEUDO_OPS_KIND_SHIFT;
		if (pseudo_ops_num != 0 && instruction.op_kind(op_count - 1) == OpKind::Immediate8) {
			if (internal::fast::try_get_pseudo_op(code, pseudo_ops_num, instruction.immediate8(), mnemonic))
				op_count--;
		}
	}

	const Register prefix_seg_reg = instruction.segment_prefix();
	// ES..GS = 0..5, None = a big value
	const std::uint32_t prefix_seg = static_cast<std::uint32_t>(prefix_seg_reg) - static_cast<std::uint32_t>(Register::ES);
	static_assert(static_cast<std::uint32_t>(Register::None) == 0, "");
	if (prefix_seg < 6 || instruction.has_lock_prefix() || instruction.has_rep_prefix() || instruction.has_repne_prefix()) {
		constexpr std::uint32_t DS_REG = static_cast<std::uint32_t>(Register::DS) - static_cast<std::uint32_t>(Register::ES);
		const bool has_notrack_prefix = prefix_seg == DS_REG && internal::is_notrack_prefix_branch(code);
		if (!has_notrack_prefix && prefix_seg < 6 && show_segment_prefix(instruction, op_count)) {
			dst_next_p = format_register(dst_next_p, prefix_seg_reg);
			dst_next_p = write_fast_ascii_char<true>(dst_next_p, ' ');
		}

		bool has_xacquire_xrelease = false;
		if (instruction.has_xacquire_prefix()) {
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_XACQUIRE);
			has_xacquire_xrelease = true;
		}
		if (instruction.has_xrelease_prefix()) {
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_XRELEASE);
			has_xacquire_xrelease = true;
		}
		if (instruction.has_lock_prefix())
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_LOCK);
		if (has_notrack_prefix)
			dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_NOTRACK);
		if (!has_xacquire_xrelease) {
			if (instruction.has_repe_prefix() && (SHOW_USELESS_PREFIXES || internal::show_rep_or_repe_prefix_bool(code, SHOW_USELESS_PREFIXES))) {
				if (internal::is_repe_or_repne_instruction(code))
					dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_REPE);
				else
					dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_REP);
			}
			if (instruction.has_repne_prefix()) {
				if ((Code::Retnw_imm16 <= code && code <= Code::Retnq) || (Code::Call_rel16 <= code && code <= Code::Jmp_rel32_64) ||
					(Code::Call_rm16 <= code && code <= Code::Call_rm64) || (Code::Jmp_rm16 <= code && code <= Code::Jmp_rm64) ||
					code_ext::is_jcc_short_or_near(code))
					dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_BND);
				else if (SHOW_USELESS_PREFIXES || internal::show_repne_prefix_bool(code, SHOW_USELESS_PREFIXES))
					dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_REPNE);
			}
		}
	}

	dst_next_p = write_fast_str(dst_next_p, mnemonic);

	bool is_declare_data;
	OpKind declare_data_kind;
	if constexpr (!TraitOptions::ENABLE_DB_DW_DD_DQ) {
		is_declare_data = false;
		declare_data_kind = OpKind::Register;
	}
	else if ((static_cast<std::uint32_t>(code) - static_cast<std::uint32_t>(Code::DeclareByte)) <=
			 (static_cast<std::uint32_t>(Code::DeclareQword) - static_cast<std::uint32_t>(Code::DeclareByte))) {
		op_count = instruction.declare_data_len();
		is_declare_data = true;
		switch (code) {
		case Code::DeclareByte:
			declare_data_kind = OpKind::Immediate8;
			break;
		case Code::DeclareWord:
			declare_data_kind = OpKind::Immediate16;
			break;
		case Code::DeclareDword:
			declare_data_kind = OpKind::Immediate32;
			break;
		case Code::DeclareQword:
		default:
			declare_data_kind = OpKind::Immediate64;
			break;
		}
	}
	else {
		is_declare_data = false;
		declare_data_kind = OpKind::Register;
	}

	if (op_count > 0) {
		dst_next_p = write_fast_ascii_char<true>(dst_next_p, ' ');

		std::uint32_t mvex_rm_operand;
		if (IcedConstants::is_mvex(code)) {
			if (instruction.op_kind(op_count - 1) == OpKind::Immediate8)
				mvex_rm_operand = op_count - 2;
			else
				mvex_rm_operand = op_count - 1;
		}
		else
			mvex_rm_operand = 0xFFFF'FFFF;

		std::uint32_t operand = 0;
		for (;;) {
			const OpKind op_kind = TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data ? declare_data_kind : instruction.op_kind(operand);

			// This speeds up SpecializedFormatter since every option is hard coded, but makes FastFormatter
			// slower because every option is dynamic (more code). Detect FastFormatter and generate smaller
			// and faster code. Both paths are tested (same tests).
			// The whole point of this formatter is to be fast so unfortunately it can result in ugly code...
			if constexpr (TraitOptions::INTERNAL_IS_FAST_FORMATTER) {
				switch (op_kind) {
				case OpKind::Register:
					dst_next_p = format_register(dst_next_p, instruction.op_register(operand));
					break;

				case OpKind::NearBranch16:
				case OpKind::NearBranch32:
				case OpKind::NearBranch64: {
					std::uint32_t imm_size;
					std::uint64_t imm64;
					if (op_kind == OpKind::NearBranch64) {
						imm_size = 8;
						imm64 = instruction.near_branch64();
					}
					else if (op_kind == OpKind::NearBranch32) {
						imm_size = 4;
						imm64 = instruction.near_branch32();
					}
					else {
						imm_size = 2;
						imm64 = instruction.near_branch16();
					}
					dst_next_p = fmt_near_branch(dst_next_p, instruction, operand, imm_size, imm64);
					break;
				}

				case OpKind::FarBranch16:
				case OpKind::FarBranch32:
					dst_next_p = fmt_far_branch(dst_next_p, instruction, operand, op_kind);
					break;

				case OpKind::Immediate8:
				case OpKind::Immediate8_2nd: {
					std::uint8_t imm8;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm8 = instruction.get_declare_byte_value(operand);
					else if (op_kind == OpKind::Immediate8)
						imm8 = instruction.immediate8();
					else
						imm8 = instruction.immediate8_2nd();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm8, 1);
					break;
				}

				case OpKind::Immediate16:
				case OpKind::Immediate8to16: {
					std::uint16_t imm16;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm16 = instruction.get_declare_word_value(operand);
					else if (op_kind == OpKind::Immediate16)
						imm16 = instruction.immediate16();
					else
						imm16 = static_cast<std::uint16_t>(instruction.immediate8to16());
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm16, 2);
					break;
				}

				case OpKind::Immediate32:
				case OpKind::Immediate8to32: {
					std::uint32_t imm32;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm32 = instruction.get_declare_dword_value(operand);
					else if (op_kind == OpKind::Immediate32)
						imm32 = instruction.immediate32();
					else
						imm32 = static_cast<std::uint32_t>(instruction.immediate8to32());
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm32, 4);
					break;
				}

				case OpKind::Immediate64:
				case OpKind::Immediate8to64:
				case OpKind::Immediate32to64: {
					std::uint64_t imm64;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm64 = instruction.get_declare_qword_value(operand);
					else if (op_kind == OpKind::Immediate32to64)
						imm64 = static_cast<std::uint64_t>(instruction.immediate32to64());
					else if (op_kind == OpKind::Immediate8to64)
						imm64 = static_cast<std::uint64_t>(instruction.immediate8to64());
					else
						imm64 = instruction.immediate64();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm64, 8);
					break;
				}

				case OpKind::MemorySegSI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::SI, Register::None,
													0, 0, 0, 2);
					break;
				case OpKind::MemorySegESI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::ESI,
													Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemorySegRSI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::RSI,
													Register::None, 0, 0, 0, 8);
					break;
				case OpKind::MemorySegDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::DI, Register::None,
													0, 0, 0, 2);
					break;
				case OpKind::MemorySegEDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::EDI,
													Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemorySegRDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::RDI,
													Register::None, 0, 0, 0, 8);
					break;
				case OpKind::MemoryESDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::DI, Register::None, 0, 0, 0, 2);
					break;
				case OpKind::MemoryESEDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::EDI, Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemoryESRDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::RDI, Register::None, 0, 0, 0, 8);
					break;
				case OpKind::Memory:
				default:
					dst_next_p = fmt_memory(dst_next_p, instruction, operand, code);
					break;
				}
			}
			else {
				switch (op_kind) {
				case OpKind::Register:
					dst_next_p = format_register(dst_next_p, instruction.op_register(operand));
					break;

				case OpKind::NearBranch16:
					dst_next_p = fmt_near_branch(dst_next_p, instruction, operand, 2, instruction.near_branch16());
					break;

				case OpKind::NearBranch32:
					dst_next_p = fmt_near_branch(dst_next_p, instruction, operand, 4, instruction.near_branch32());
					break;

				case OpKind::NearBranch64:
					dst_next_p = fmt_near_branch(dst_next_p, instruction, operand, 8, instruction.near_branch64());
					break;

				case OpKind::FarBranch16:
				case OpKind::FarBranch32:
					dst_next_p = fmt_far_branch(dst_next_p, instruction, operand, op_kind);
					break;

				case OpKind::Immediate8: {
					std::uint8_t imm8;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm8 = instruction.get_declare_byte_value(operand);
					else
						imm8 = instruction.immediate8();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm8, 1);
					break;
				}

				case OpKind::Immediate8_2nd:
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, instruction.immediate8_2nd(), 1);
					break;

				case OpKind::Immediate16: {
					std::uint16_t imm16;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm16 = instruction.get_declare_word_value(operand);
					else
						imm16 = instruction.immediate16();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm16, 2);
					break;
				}

				case OpKind::Immediate8to16:
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, static_cast<std::uint16_t>(instruction.immediate8to16()), 2);
					break;

				case OpKind::Immediate32: {
					std::uint32_t imm32;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm32 = instruction.get_declare_dword_value(operand);
					else
						imm32 = instruction.immediate32();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm32, 4);
					break;
				}

				case OpKind::Immediate8to32:
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, static_cast<std::uint32_t>(instruction.immediate8to32()), 4);
					break;

				case OpKind::Immediate64: {
					std::uint64_t imm64;
					if (TraitOptions::ENABLE_DB_DW_DD_DQ && is_declare_data)
						imm64 = instruction.get_declare_qword_value(operand);
					else
						imm64 = instruction.immediate64();
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, imm64, 8);
					break;
				}

				case OpKind::Immediate8to64:
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, static_cast<std::uint64_t>(instruction.immediate8to64()), 8);
					break;

				case OpKind::Immediate32to64:
					dst_next_p = fmt_imm(dst_next_p, instruction, operand, static_cast<std::uint64_t>(instruction.immediate32to64()), 8);
					break;

				case OpKind::MemorySegSI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::SI, Register::None,
													0, 0, 0, 2);
					break;
				case OpKind::MemorySegESI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::ESI,
													Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemorySegRSI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::RSI,
													Register::None, 0, 0, 0, 8);
					break;
				case OpKind::MemorySegDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::DI, Register::None,
													0, 0, 0, 2);
					break;
				case OpKind::MemorySegEDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::EDI,
													Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemorySegRDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, instruction.memory_segment(), Register::RDI,
													Register::None, 0, 0, 0, 8);
					break;
				case OpKind::MemoryESDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::DI, Register::None, 0, 0, 0, 2);
					break;
				case OpKind::MemoryESEDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::EDI, Register::None, 0, 0, 0, 4);
					break;
				case OpKind::MemoryESRDI:
					dst_next_p = call_format_memory(dst_next_p, instruction, operand, Register::ES, Register::RDI, Register::None, 0, 0, 0, 8);
					break;
				case OpKind::Memory:
				default:
					dst_next_p = fmt_memory(dst_next_p, instruction, operand, code);
					break;
				}
			}

			if (operand == 0 && (instruction.has_op_mask() || instruction.zeroing_masking())) {
				if (instruction.has_op_mask()) {
					dst_next_p = write_fast_ascii_char<true>(dst_next_p, '{');
					dst_next_p = format_register(dst_next_p, instruction.op_mask());
					dst_next_p = write_fast_ascii_char<true>(dst_next_p, '}');
				}
				if (instruction.zeroing_masking())
					dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_Z);
			}
			if (mvex_rm_operand == operand) {
				const MvexRegMemConv conv = instruction.mvex_reg_mem_conv();
				if (conv != MvexRegMemConv::None) {
					const std::uint8_t* s = internal::fast::get_mvex_reg_mem_conv_string(code, conv);
					if (s != nullptr)
						dst_next_p = write_fast_str<12>(dst_next_p, s);
				}
			}

			operand++;
			if (operand >= op_count)
				break;

			if (TraitOptions::space_after_operand_separator(options_))
				dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_COMMA_SPACE);
			else
				dst_next_p = write_fast_ascii_char<true>(dst_next_p, ',');
		}
		if (instruction.rounding_control() != RoundingControl::None || instruction.suppress_all_exceptions()) {
			const RoundingControl rc = instruction.rounding_control();
			if (rc != RoundingControl::None) {
				if (IcedConstants::is_mvex(code) && !instruction.suppress_all_exceptions())
					dst_next_p = write_fast_str(dst_next_p, internal::fast::RC_STRINGS[static_cast<std::size_t>(rc)]);
				else
					dst_next_p = write_fast_str(dst_next_p, internal::fast::RC_SAE_STRINGS[static_cast<std::size_t>(rc)]);
			}
			else
				dst_next_p = write_fast_str(dst_next_p, internal::fast::STR_SAE);
		}
	}

	return dst_next_p;
}

// NOT PART OF THE PUBLIC API. Explicit instantiation (`EXTERN` = `extern`: declaration) of the non-inline member functions.
// The whole class isn't instantiated so the inline functions (eg. the `std::string` convenience wrapper) are only
// instantiated by the code that uses them.
#define ICED_X86_INTERNAL_INSTANTIATE_SPECIALIZED_FORMATTER(EXTERN, TRAIT_OPTIONS) \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::format_core(const Instruction&, std::uint8_t*, std::uint8_t*); \
	EXTERN template std::size_t SpecializedFormatter<TRAIT_OPTIONS>::format_slow(const Instruction&, char*, std::size_t); \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::format_number(std::uint8_t*, std::uint64_t) const noexcept; \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::write_symbol2(std::uint8_t*, std::uint64_t, const SymbolResult&, bool) \
		const; \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::fmt_near_branch_symbol(std::uint8_t*, const Instruction&, std::uint32_t, \
																							   std::uint32_t, std::uint64_t); \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::fmt_far_branch_symbol(std::uint8_t*, const Instruction&, std::uint32_t, \
																							  std::uint32_t, std::uint64_t); \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::fmt_imm_symbol(std::uint8_t*, const Instruction&, std::uint32_t, \
																					   std::uint64_t, std::uint32_t); \
	EXTERN template std::uint8_t* SpecializedFormatter<TRAIT_OPTIONS>::format_memory_symbol( \
		std::uint8_t*, const Instruction&, std::uint32_t, std::uint64_t, std::uint32_t, bool, std::uint32_t, std::int64_t);

// The library contains these instantiations
ICED_X86_INTERNAL_INSTANTIATE_SPECIALIZED_FORMATTER(extern, DefaultFastFormatterTraitOptions)
ICED_X86_INTERNAL_INSTANTIATE_SPECIALIZED_FORMATTER(extern, DefaultSpecializedFormatterTraitOptions)

} // namespace iced_x86
