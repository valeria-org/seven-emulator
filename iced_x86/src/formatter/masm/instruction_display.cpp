// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: `impl fmt::Display for Instruction` (instruction.rs). It's in its own file so the masm formatter is only
// linked if it's used.

#include <string>

#include "iced_x86/instruction.hpp"
#include "iced_x86/masm_formatter.hpp"

namespace iced_x86 {

std::string to_string(const Instruction& instruction) {
	// Rust uses the first available formatter: masm, nasm, intel, gas, fast. The C++ library always has the masm formatter.
	MasmFormatter formatter;
	std::string output;
	formatter.format(instruction, output);
	return output;
}

} // namespace iced_x86
