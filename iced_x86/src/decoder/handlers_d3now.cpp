// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/handlers_d3now.hpp"
#include "iced_x86/decoder_options.hpp"
#include "internal/decoder/d3now_code_values.hpp"

namespace iced_x86::internal {

static_assert(sizeof(D3NOW_CODE_VALUES) / sizeof(D3NOW_CODE_VALUES[0]) == 0x100, "");

void OpCodeHandler_D3NOW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::Legacy));
	static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
	//instruction.set_op0_kind(OpKind::Register);
	instruction.set_op0_register(static_cast<Register>(decoder.state.reg + reg_u32(Register::MM0)));
	if (decoder.state.mod_ == 3) {
		static_assert(static_cast<std::uint32_t>(OpKind::Register) == 0, "");
		//instruction.set_op1_kind(OpKind::Register);
		instruction.set_op1_register(static_cast<Register>(decoder.state.rm + reg_u32(Register::MM0)));
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		decoder.read_op_mem(instruction);
	}
	std::size_t ib = decoder.read_u8();
	Code code = D3NOW_CODE_VALUES[ib];
	switch (code) {
	case Code::D3NOW_Pfrcpv_mm_mmm64:
	case Code::D3NOW_Pfrsqrtv_mm_mmm64:
		if ((decoder.options & DecoderOptions::CYRIX) == 0 || decoder.bitness == 64)
			code = Code::INVALID;
		break;
	default:
		break;
	}
	instruction.set_code(code);
	if (code == Code::INVALID)
		decoder.set_invalid_instruction();
}

} // namespace iced_x86::internal
