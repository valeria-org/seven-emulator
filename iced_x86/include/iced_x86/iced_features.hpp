// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

namespace iced_x86 {

/// Gets the available features.
///
/// The C++ library is always built with all features (it's a static library so the linker only
/// includes the code that is used), so all methods return `true`.
struct IcedFeatures {
	/// `true` if the gas (AT&amp;T) formatter is available
	static constexpr bool has_gas() noexcept { return true; }

	/// `true` if the Intel (xed) formatter is available
	static constexpr bool has_intel() noexcept { return true; }

	/// `true` if the masm formatter is available
	static constexpr bool has_masm() noexcept { return true; }

	/// `true` if the nasm formatter is available
	static constexpr bool has_nasm() noexcept { return true; }

	/// `true` if the fast formatter is available
	static constexpr bool has_fast_fmt() noexcept { return true; }

	/// `true` if the decoder is available
	static constexpr bool has_decoder() noexcept { return true; }

	/// `true` if the encoder is available
	static constexpr bool has_encoder() noexcept { return true; }

	/// `true` if the block encoder is available
	static constexpr bool has_block_encoder() noexcept { return true; }

	/// `true` if the opcode info is available
	static constexpr bool has_op_code_info() noexcept { return true; }

	/// `true` if the instruction info code is available
	static constexpr bool has_instruction_info() noexcept { return true; }
};

} // namespace iced_x86
