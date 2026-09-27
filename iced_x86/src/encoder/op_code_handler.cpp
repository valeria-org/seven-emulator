// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/encoder/op_code_handler.hpp"
#include "iced_x86/encoder.hpp"
#include "iced_x86/mvex_eh_bit.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/rounding_control.hpp"
#include "internal/encoder/enc_flags3.hpp"
#include "internal/encoder/encoder_flags.hpp"
#include "internal/encoder/encoder_internal.hpp"
#include "internal/encoder/evex_op_code_table.hpp"
#include "internal/encoder/legacy_op_code_table.hpp"
#include "internal/encoder/mvex_op_code_table.hpp"
#include "internal/encoder/vex_op_code_table.hpp"
#include "internal/encoder/xop_op_code_table.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instruction_internal.hpp"
#include "internal/mandatory_prefix_byte.hpp"
#include "internal/mvex/mvex.hpp"
#include "internal/tuple_type_tbl.hpp"
#include <limits>

namespace iced_x86::internal {

static_assert(sizeof(EncOpCodeHandler) <= 28, "EncOpCodeHandler should be small since there's one handler per Code value");

namespace {

using E = EncoderInternal;

// ---------------------------------------------------------------------------
// InvalidHandler

void invalid_encode(const EncOpCodeHandler*, Encoder& encoder, const Instruction&) { E::set_error_message_str(encoder, INVALID_HANDLER_ERROR_MESSAGE); }

// ---------------------------------------------------------------------------
// DeclareDataHandler

void declare_data_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const std::size_t length = instruction.declare_data_len() * self->u.declare_data.elem_size;
	for (std::size_t i = 0; i < length; i++) {
		auto value = instruction.try_get_declare_byte_value(i);
		if (value.is_ok())
			E::write_byte_internal(encoder, value.value());
		else {
			E::set_error_message_str(encoder, "Invalid db/dw/dd/dq data length");
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// ZeroBytesHandler

void zero_bytes_encode(const EncOpCodeHandler*, Encoder&, const Instruction&) {}

// ---------------------------------------------------------------------------
// LegacyHandler

void legacy_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const EncOpCodeHandler::LegacyData& d = self->u.legacy;
	std::uint32_t b = d.mandatory_prefix;
	E::write_prefixes(encoder, instruction, b != 0xF3);
	if (b != 0)
		E::write_byte_internal(encoder, b);

	static_assert(EncoderFlags::B == 0x01, "");
	static_assert(EncoderFlags::X == 0x02, "");
	static_assert(EncoderFlags::R == 0x04, "");
	static_assert(EncoderFlags::W == 0x08, "");
	static_assert(EncoderFlags::REX == 0x40, "");
	b = E::encoder_flags(encoder);
	b &= 0x4F;
	if (b != 0) {
		if ((E::encoder_flags(encoder) & EncoderFlags::HIGH_LEGACY_8_BIT_REGS) != 0)
			E::set_error_message_str(encoder,
				"Registers AH, CH, DH, BH can't be used if there's a REX prefix. Use AL, CL, DL, BL, SPL, BPL, SIL, DIL, R8L-R15L instead.");
		b |= 0x40;
		E::write_byte_internal(encoder, b);
	}

	b = d.table_byte1;
	if (b != 0) {
		E::write_byte_internal(encoder, b);
		b = d.table_byte2;
		if (b != 0)
			E::write_byte_internal(encoder, b);
	}
}

// ---------------------------------------------------------------------------
// VexHandler

void vex_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const EncOpCodeHandler::VexData& d = self->u.vex;
	E::write_prefixes(encoder, instruction, true);
	const std::uint32_t encoder_flags = E::encoder_flags(encoder);

	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::None) == 0, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::P66) == 1, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::PF3) == 2, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::PF2) == 3, "");
	std::uint32_t b = d.last_byte;
	b |= (~encoder_flags >> (EncoderFlags::VVVVV_SHIFT - 3)) & 0x78;

	if ((E::prevent_vex2(encoder) | static_cast<std::uint32_t>(d.w1) | (d.table - static_cast<std::uint32_t>(VexOpCodeTable::MAP0F)) |
			(encoder_flags & (EncoderFlags::X | EncoderFlags::B | EncoderFlags::W))) != 0) {
		E::write_byte_internal(encoder, 0xC4);
		static_assert(static_cast<std::uint32_t>(VexOpCodeTable::MAP0F) == 1, "");
		static_assert(static_cast<std::uint32_t>(VexOpCodeTable::MAP0F38) == 2, "");
		static_assert(static_cast<std::uint32_t>(VexOpCodeTable::MAP0F3A) == 3, "");
		std::uint32_t b2 = d.table;
		static_assert(EncoderFlags::B == 1, "");
		static_assert(EncoderFlags::X == 2, "");
		static_assert(EncoderFlags::R == 4, "");
		b2 |= (~encoder_flags & 7) << 5;
		E::write_byte_internal(encoder, b2);
		b |= d.mask_w_l & E::internal_vex_wig_lig(encoder);
		E::write_byte_internal(encoder, b);
	}
	else {
		E::write_byte_internal(encoder, 0xC5);
		static_assert(EncoderFlags::R == 4, "");
		b |= (~encoder_flags & 4) << 5;
		b |= d.mask_l & E::internal_vex_lig(encoder);
		E::write_byte_internal(encoder, b);
	}
}

// ---------------------------------------------------------------------------
// XopHandler

void xop_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const EncOpCodeHandler::XopData& d = self->u.xop;
	E::write_prefixes(encoder, instruction, true);
	E::write_byte_internal(encoder, 0x8F);

	const std::uint32_t encoder_flags = E::encoder_flags(encoder);
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::None) == 0, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::P66) == 1, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::PF3) == 2, "");
	static_assert(static_cast<std::uint32_t>(MandatoryPrefixByte::PF2) == 3, "");

	std::uint32_t b = d.table;
	static_assert(EncoderFlags::B == 1, "");
	static_assert(EncoderFlags::X == 2, "");
	static_assert(EncoderFlags::R == 4, "");
	b |= (~encoder_flags & 7) << 5;
	E::write_byte_internal(encoder, b);
	b = d.last_byte;
	b |= (~encoder_flags >> (EncoderFlags::VVVVV_SHIFT - 3)) & 0x78;
	E::write_byte_internal(encoder, b);
}

// ---------------------------------------------------------------------------
// EvexHandler

void evex_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const EncOpCodeHandler::EvexData& d = self->u.evex;
	E::write_prefixes(encoder, instruction, true);
	const std::uint32_t encoder_flags = E::encoder_flags(encoder);

	E::write_byte_internal(encoder, 0x62);

	static_assert(static_cast<std::uint32_t>(EvexOpCodeTable::MAP0F) == 1, "");
	static_assert(static_cast<std::uint32_t>(EvexOpCodeTable::MAP0F38) == 2, "");
	static_assert(static_cast<std::uint32_t>(EvexOpCodeTable::MAP0F3A) == 3, "");
	static_assert(static_cast<std::uint32_t>(EvexOpCodeTable::MAP5) == 5, "");
	static_assert(static_cast<std::uint32_t>(EvexOpCodeTable::MAP6) == 6, "");
	std::uint32_t b = d.table;
	static_assert(EncoderFlags::B == 1, "");
	static_assert(EncoderFlags::X == 2, "");
	static_assert(EncoderFlags::R == 4, "");
	b |= (encoder_flags & 7) << 5;
	static_assert(EncoderFlags::R2 == 0x0000'0200, "");
	b |= (encoder_flags >> (9 - 4)) & 0x10;
	b ^= ~0xFU;
	E::write_byte_internal(encoder, b);

	b = d.p1_bits;
	b |= (~encoder_flags >> (EncoderFlags::VVVVV_SHIFT - 3)) & 0x78;
	b |= d.mask_w & E::internal_evex_wig(encoder);
	E::write_byte_internal(encoder, b);

	b = InstructionInternal::internal_op_mask(instruction);
	if (b != 0) {
		if ((self->enc_flags3 & EncFlags3::OP_MASK_REGISTER) == 0)
			E::set_error_message_str(encoder, "The instruction doesn't support opmask registers");
	}
	else {
		if ((self->enc_flags3 & EncFlags3::REQUIRE_OP_MASK_REGISTER) != 0)
			E::set_error_message_str(encoder, "The instruction must use an opmask register");
	}
	b |= (encoder_flags >> (EncoderFlags::VVVVV_SHIFT + 4 - 3)) & 8;
	if (instruction.suppress_all_exceptions()) {
		if ((self->enc_flags3 & EncFlags3::SUPPRESS_ALL_EXCEPTIONS) == 0)
			E::set_error_message_str(encoder, "The instruction doesn't support suppress-all-exceptions");
		b |= 0x10;
	}
	const RoundingControl rc = instruction.rounding_control();
	if (rc != RoundingControl::None) {
		if ((self->enc_flags3 & EncFlags3::ROUNDING_CONTROL) == 0)
			E::set_error_message_str(encoder, "The instruction doesn't support rounding control");
		b |= 0x10;
		static_assert(static_cast<std::uint32_t>(RoundingControl::RoundToNearest) == 1, "");
		static_assert(static_cast<std::uint32_t>(RoundingControl::RoundDown) == 2, "");
		static_assert(static_cast<std::uint32_t>(RoundingControl::RoundUp) == 3, "");
		static_assert(static_cast<std::uint32_t>(RoundingControl::RoundTowardZero) == 4, "");
		b |= (static_cast<std::uint32_t>(rc) - static_cast<std::uint32_t>(RoundingControl::RoundToNearest)) << 5;
	}
	else if ((self->enc_flags3 & EncFlags3::SUPPRESS_ALL_EXCEPTIONS) == 0 || !instruction.suppress_all_exceptions())
		b |= d.ll_bits;
	if ((encoder_flags & EncoderFlags::BROADCAST) != 0)
		b |= 0x10;
	else if (instruction.is_broadcast())
		E::set_error_message_str(encoder, "The instruction doesn't support broadcasting");
	if (instruction.zeroing_masking()) {
		if ((self->enc_flags3 & EncFlags3::ZEROING_MASKING) == 0)
			E::set_error_message_str(encoder, "The instruction doesn't support zeroing masking");
		b |= 0x80;
	}
	b ^= 8;
	b |= d.mask_ll & E::internal_evex_lig(encoder);
	E::write_byte_internal(encoder, b);
}

// ---------------------------------------------------------------------------
// MvexHandler

void mvex_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	const EncOpCodeHandler::MvexData& d = self->u.mvex;
	E::write_prefixes(encoder, instruction, true);
	const std::uint32_t encoder_flags = E::encoder_flags(encoder);

	E::write_byte_internal(encoder, 0x62);

	static_assert(static_cast<std::uint32_t>(MvexOpCodeTable::MAP0F) == 1, "");
	static_assert(static_cast<std::uint32_t>(MvexOpCodeTable::MAP0F38) == 2, "");
	static_assert(static_cast<std::uint32_t>(MvexOpCodeTable::MAP0F3A) == 3, "");
	std::uint32_t b = d.table;
	static_assert(EncoderFlags::B == 1, "");
	static_assert(EncoderFlags::X == 2, "");
	static_assert(EncoderFlags::R == 4, "");
	b |= (encoder_flags & 7) << 5;
	static_assert(EncoderFlags::R2 == 0x0000'0200, "");
	b |= (encoder_flags >> (9 - 4)) & 0x10;
	b ^= ~0xFU;
	E::write_byte_internal(encoder, b);

	b = d.p1_bits;
	b |= (~encoder_flags >> (EncoderFlags::VVVVV_SHIFT - 3)) & 0x78;
	b |= d.mask_w & E::internal_mvex_wig(encoder);
	E::write_byte_internal(encoder, b);

	b = InstructionInternal::internal_op_mask(instruction);
	if (b != 0) {
		if ((self->enc_flags3 & EncFlags3::OP_MASK_REGISTER) == 0)
			E::set_error_message_str(encoder, "The instruction doesn't support opmask registers");
	}
	else {
		if ((self->enc_flags3 & EncFlags3::REQUIRE_OP_MASK_REGISTER) != 0)
			E::set_error_message_str(encoder, "The instruction must use an opmask register");
	}
	b |= (encoder_flags >> (EncoderFlags::VVVVV_SHIFT + 4 - 3)) & 8;
	const MvexInfo& mvex = get_mvex_info(instruction.code());
	const MvexRegMemConv conv = instruction.mvex_reg_mem_conv();
	// Memory ops can only be op0-op2, never op3 (imm8)
	if (instruction.op0_kind() == OpKind::Memory || instruction.op1_kind() == OpKind::Memory || instruction.op2_kind() == OpKind::Memory) {
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 1 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast1), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 2 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvBroadcast4), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 3 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvFloat16), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 4 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint8), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 5 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint8), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 6 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvUint16), "");
		static_assert(static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone) + 7 == static_cast<std::uint32_t>(MvexRegMemConv::MemConvSint16), "");
		if (conv >= MvexRegMemConv::MemConvNone && conv <= MvexRegMemConv::MemConvSint16)
			b |= (static_cast<std::uint32_t>(conv) - static_cast<std::uint32_t>(MvexRegMemConv::MemConvNone)) << 4;
		else if (conv == MvexRegMemConv::None) {
			// Nothing, treat it as MvexRegMemConv::MemConvNone
		}
		else
			E::set_error_message_str(encoder, "Memory operands must use a valid MvexRegMemConv variant, eg. MvexRegMemConv::MemConvNone");
		if (instruction.is_mvex_eviction_hint()) {
			if (mvex.can_use_eviction_hint())
				b |= 0x80;
			else
				E::set_error_message_str(encoder, "This instruction doesn't support eviction hint (`{eh}`)");
		}
	}
	else {
		if (instruction.is_mvex_eviction_hint())
			E::set_error_message_str(encoder, "Only memory operands can enable eviction hint (`{eh}`)");
		if (conv == MvexRegMemConv::None) {
			b |= 0x80;
			if (instruction.suppress_all_exceptions()) {
				b |= 0x40;
				if ((self->enc_flags3 & EncFlags3::SUPPRESS_ALL_EXCEPTIONS) == 0)
					E::set_error_message_str(encoder, "The instruction doesn't support suppress-all-exceptions");
			}
			const RoundingControl rc = instruction.rounding_control();
			if (rc == RoundingControl::None) {
				// Nothing
			}
			else {
				if ((self->enc_flags3 & EncFlags3::ROUNDING_CONTROL) == 0)
					E::set_error_message_str(encoder, "The instruction doesn't support rounding control");
				else {
					static_assert(static_cast<std::uint32_t>(RoundingControl::RoundToNearest) == 1, "");
					static_assert(static_cast<std::uint32_t>(RoundingControl::RoundDown) == 2, "");
					static_assert(static_cast<std::uint32_t>(RoundingControl::RoundUp) == 3, "");
					static_assert(static_cast<std::uint32_t>(RoundingControl::RoundTowardZero) == 4, "");
					b |= (static_cast<std::uint32_t>(rc) - static_cast<std::uint32_t>(RoundingControl::RoundToNearest)) << 4;
				}
			}
		}
		else if (conv >= MvexRegMemConv::RegSwizzleNone && conv <= MvexRegMemConv::RegSwizzleDddd) {
			if (instruction.suppress_all_exceptions())
				E::set_error_message_str(encoder, "Can't use {sae} with register swizzles");
			else if (instruction.rounding_control() != RoundingControl::None)
				E::set_error_message_str(encoder, "Can't use rounding control with register swizzles");
			b |= ((static_cast<std::uint32_t>(conv) - static_cast<std::uint32_t>(MvexRegMemConv::RegSwizzleNone)) & 7) << 4;
		}
		else
			E::set_error_message_str(encoder, "Register operands can't use memory up/down conversions");
	}
	if (mvex.eh_bit == MvexEHBit::EH1)
		b |= 0x80;
	b ^= 8;
	E::write_byte_internal(encoder, b);
}

// ---------------------------------------------------------------------------
// D3nowHandler

void d3now_encode(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction) {
	E::write_prefixes(encoder, instruction, true);
	E::write_byte_internal(encoder, 0x0F);
	E::imm_size(encoder) = ImmSize::Size1OpCode;
	E::immediate(encoder) = self->u.d3now.immediate;
}

// ---------------------------------------------------------------------------

} // namespace

std::optional<std::int8_t> evex_try_convert_to_disp8n(const EncOpCodeHandler* self, Encoder& encoder, const Instruction&, std::int32_t displ) {
	const std::int32_t n =
		static_cast<std::int32_t>(get_disp8n(self->u.evex.tuple_type, (E::encoder_flags(encoder) & EncoderFlags::BROADCAST) != 0));
	const std::int32_t res = displ / n;
	if (res * n == displ && std::numeric_limits<std::int8_t>::min() <= res && res <= std::numeric_limits<std::int8_t>::max())
		return static_cast<std::int8_t>(res);
	return std::nullopt;
}

std::optional<std::int8_t> mvex_try_convert_to_disp8n(const EncOpCodeHandler*, Encoder&, const Instruction& instruction, std::int32_t displ) {
	const MvexInfo& mvex = get_mvex_info(instruction.code());
	const MvexRegMemConv conv = instruction.mvex_reg_mem_conv();
	const std::size_t sss = (static_cast<std::size_t>(conv) - static_cast<std::size_t>(MvexRegMemConv::MemConvNone)) & 7;
	const TupleType tuple_type = MVEX_TUPLE_TYPE_LUT[static_cast<std::size_t>(mvex.tuple_type_lut_kind) * 8 + sss];

	const std::int32_t n = static_cast<std::int32_t>(get_disp8n(tuple_type, false));
	const std::int32_t res = displ / n;
	if (res * n == displ && std::numeric_limits<std::int8_t>::min() <= res && res <= std::numeric_limits<std::int8_t>::max())
		return static_cast<std::int8_t>(res);
	return std::nullopt;
}

const OpCodeHandlerEncodeFn OP_CODE_HANDLER_ENCODE_FNS[OP_CODE_HANDLER_KIND_COUNT] = {
	invalid_encode,
	declare_data_encode,
	zero_bytes_encode,
	legacy_encode,
	vex_encode,
	xop_encode,
	evex_encode,
	mvex_encode,
	d3now_encode,
};

} // namespace iced_x86::internal
