// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/handlers_mvex.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/mvex_tuple_type_lut_kind.hpp"
#include "iced_x86/rounding_control.hpp"

namespace iced_x86::internal {

// Rust: write_eviction_hint!()
static ICED_FORCE_INLINE void write_eviction_hint(DecoderCore& decoder, Instruction& instruction) noexcept {
	if ((decoder.state.flags & StateFlags::MVEX_EH) != 0)
		instruction.set_is_mvex_eviction_hint(true);
}

// Rust: write_mem_conv!()
static ICED_FORCE_INLINE void write_mem_conv(DecoderCore& decoder, Instruction& instruction, const MvexInfo& mvex, std::uint32_t sss) noexcept {
	if ((static_cast<std::uint32_t>(mvex.invalid_conv_fns) & (1U << sss) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 1 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast1), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 2 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast4), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 3 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvFloat16), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 4 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint8), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 5 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint8), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 6 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint16), "");
	static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 7 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint16), "");
	static_assert(StateFlags::MVEX_SSS_MASK == 7, "");
	ICED_DEBUG_ASSERT(sss <= 7);
	InstructionInternal::internal_set_mvex_reg_mem_conv(instruction, static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + sss);
}

// Rust: write_reg_conv_and_er_sae!()
static ICED_FORCE_INLINE void write_reg_conv_and_er_sae(DecoderCore& decoder, Instruction& instruction, const MvexInfo& mvex,
														 std::uint32_t sss) noexcept {
	if ((decoder.state.flags & StateFlags::MVEX_EH) != 0) {
		if (mvex.can_use_suppress_all_exceptions()) {
			if ((sss & 4) != 0)
				instruction.set_suppress_all_exceptions(true);
			if (mvex.can_use_rounding_control()) {
				static_assert(static_cast<std::uint32_t>(RoundingControl::None) == 0, "");
				static_assert(static_cast<std::uint32_t>(RoundingControl::RoundToNearest) == 1, "");
				static_assert(static_cast<std::uint32_t>(RoundingControl::RoundDown) == 2, "");
				static_assert(static_cast<std::uint32_t>(RoundingControl::RoundUp) == 3, "");
				static_assert(static_cast<std::uint32_t>(RoundingControl::RoundTowardZero) == 4, "");
				InstructionInternal::internal_set_rounding_control(instruction, (sss & 3) + static_cast<std::uint32_t>(RoundingControl::RoundToNearest));
			}
		}
		else if (mvex.no_sae_rc() && (sss & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
	}
	else {
		if ((static_cast<std::uint32_t>(mvex.invalid_swizzle_fns) & (1U << sss) & decoder.invalid_check_mask) != 0)
			decoder.set_invalid_instruction();
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 1 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleCdab), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 2 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleBadc), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 3 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleDacb), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 4 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleAaaa), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 5 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleBbbb), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 6 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleCccc), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + 7 == static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleDddd), "");
		static_assert(StateFlags::MVEX_SSS_MASK == 7, "");
		ICED_DEBUG_ASSERT(sss <= 7);
		InstructionInternal::internal_set_mvex_reg_mem_conv(instruction, static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone) + sss);
	}
}

// Only called by the MVEX opcode handlers and they only pass in 0<=sss<=7
static ICED_FORCE_INLINE TupleType get_tuple_type(MvexTupleTypeLutKind lut, std::uint32_t sss) noexcept {
	static_assert(StateFlags::MVEX_SSS_MASK == 7, "");
	ICED_DEBUG_ASSERT(sss <= 7);
	static_assert(sizeof(MVEX_TUPLE_TYPE_LUT) / sizeof(MVEX_TUPLE_TYPE_LUT[0]) ==
					  IcedConstants::MVEX_TUPLE_TYPE_LUT_KIND_ENUM_COUNT * (static_cast<std::size_t>(StateFlags::MVEX_SSS_MASK) + 1),
				  "");
	// valid index, see above
	return MVEX_TUPLE_TYPE_LUT[static_cast<std::size_t>(lut) * (static_cast<std::size_t>(StateFlags::MVEX_SSS_MASK) + 1) + static_cast<std::size_t>(sss)];
}

void OpCodeHandler_EH::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_EH>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	const OpCodeHandler* handler = this_.handlers[(decoder.state.flags & StateFlags::MVEX_EH) != 0 ? 1 : 0];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_MVEX_M::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_M>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_op_mask(Register::None); // It's ignored (see ctor)
	instruction.set_code(this_.code);
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_MV::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_MV>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);
	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		if (mvex.can_use_eviction_hint())
			write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_VW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VW>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_HWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_HWIb>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.vvvv + reg_u32(Register::ZMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
	instruction.set_immediate8(static_cast<std::uint8_t>(decoder.read_u8()));
}

void OpCodeHandler_MVEX_VWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VWIb>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if ((decoder.state.vvvv_invalid_check & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	instruction.set_op2_kind(OpKind::Immediate8);
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op1_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op1_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
	instruction.set_immediate8(static_cast<std::uint8_t>(decoder.read_u8()));
}

void OpCodeHandler_MVEX_VHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VHW>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(Register::ZMM0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	if (mvex.require_op_mask_register() && decoder.invalid_check_mask != 0 && decoder.state.aaa == 0)
		decoder.set_invalid_instruction();
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_VHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VHWIb>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(Register::ZMM0));
	instruction.set_op3_kind(OpKind::Immediate8);
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
	instruction.set_immediate8(static_cast<std::uint8_t>(decoder.read_u8()));
}

void OpCodeHandler_MVEX_VKW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VKW>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if ((decoder.state.vvvv & decoder.invalid_check_mask) > 7)
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op0_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	write_op1_reg(instruction, (decoder.state.vvvv & 7) + reg_u32(Register::K0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_KHW::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_KHW>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(Register::ZMM0));
	if (((decoder.state.extra_register_base | decoder.state.extra_register_base_evex) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_KHWIb::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_KHWIb>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	instruction.set_code(this_.code);

	write_op0_reg(instruction, decoder.state.reg + reg_u32(Register::K0));
	write_op1_reg(instruction, decoder.state.vvvv + reg_u32(Register::ZMM0));
	if (((decoder.state.extra_register_base | decoder.state.extra_register_base_evex) & decoder.invalid_check_mask) != 0)
		decoder.set_invalid_instruction();
	instruction.set_op3_kind(OpKind::Immediate8);
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3) {
		write_op2_reg(instruction, decoder.state.rm + decoder.state.extra_base_register_base_evex + reg_u32(Register::ZMM0));
		write_reg_conv_and_er_sae(decoder, instruction, mvex, sss);
	}
	else {
		instruction.set_op2_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_tuple_type(instruction, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
	instruction.set_immediate8(static_cast<std::uint8_t>(decoder.read_u8()));
}

void OpCodeHandler_MVEX_VSIB::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VSIB>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (decoder.invalid_check_mask != 0 && ((decoder.state.vvvv_invalid_check & 0xF) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_vsib(instruction, Register::ZMM0, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_VSIB_V::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_VSIB_V>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (decoder.invalid_check_mask != 0 && ((decoder.state.vvvv_invalid_check & 0xF) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	write_op1_reg(instruction,
				  decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex + reg_u32(Register::ZMM0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op0_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_vsib(instruction, Register::ZMM0, get_tuple_type(mvex.tuple_type_lut_kind, sss));
	}
}

void OpCodeHandler_MVEX_V_VSIB::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MVEX_V_VSIB>(self_ptr);
	// Checked here and not in the ctor since the ctor is only called at compile time (MVEX_INFO isn't constexpr)
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_op_mask_register());
	ICED_DEBUG_ASSERT(get_mvex_info(this_.code).can_use_eviction_hint());
	ICED_DEBUG_ASSERT(!get_mvex_info(this_.code).ignores_eviction_hint());
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	if (decoder.invalid_check_mask != 0 && ((decoder.state.vvvv_invalid_check & 0xF) != 0 || decoder.state.aaa == 0))
		decoder.set_invalid_instruction();
	instruction.set_code(this_.code);

	std::uint32_t reg_num = decoder.state.reg + decoder.state.extra_register_base + decoder.state.extra_register_base_evex;
	write_op0_reg(instruction, reg_num + reg_u32(Register::ZMM0));
	const MvexInfo& mvex = get_mvex_info(this_.code);
	std::uint32_t sss = decoder.state.sss();
	if (decoder.state.mod_ == 3)
		decoder.set_invalid_instruction();
	else {
		instruction.set_op1_kind(OpKind::Memory);
		write_eviction_hint(decoder, instruction);
		write_mem_conv(decoder, instruction, mvex, sss);
		decoder.read_op_mem_vsib(instruction, Register::ZMM0, get_tuple_type(mvex.tuple_type_lut_kind, sss));
		if (decoder.invalid_check_mask != 0) {
			if (reg_num == ((static_cast<std::uint32_t>(instruction.memory_index()) - reg_u32(Register::XMM0)) % IcedConstants::VMM_COUNT))
				decoder.set_invalid_instruction();
		}
	}
}

} // namespace iced_x86::internal
