// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Code assembler: easier creating of instructions (eg. `a.mov(eax, ecx)`) than using `Instruction::with*()` functions.
//
// ```cpp
// #include "iced_x86/code_asm.hpp"
// using namespace iced_x86::code_asm;
//
// CodeAssembler a(64);
// a.push(rcx);
// a.mov(rax, qword_ptr(rcx + rdx * 8 + 0x10));
// a.ret();
// auto bytes = a.assemble(0x12345678);
// ```
//
// See `iced_x86::code_asm::CodeAssembler` for more info.

#pragma once

#include "iced_x86/code_asm/code_assembler.hpp"
#include "iced_x86/code_asm/code_label.hpp"
#include "iced_x86/code_asm/mem.hpp"
#include "iced_x86/code_asm/mem_ptr.hpp"
#include "iced_x86/code_asm/memory_operand_size.hpp"
#include "iced_x86/code_asm/op_state.hpp"
#include "iced_x86/code_asm/reg.hpp"
#include "iced_x86/code_asm/registers.hpp"
