// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/handlers.hpp"

namespace iced_x86::internal {

void OpCodeHandler_Invalid::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	decoder.set_invalid_instruction();
}

void OpCodeHandler_Simple::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Simple>(self_ptr);
	instruction.set_code(this_.code);
}

void OpCodeHandler_Simple::decode_int3(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	instruction.set_code(Code::Int3);
}

void OpCodeHandler_Group8x8::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Group8x8>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.reg <= 7);
	const OpCodeHandler* handler;
	if (decoder.state.mod_ == 3)
		handler = this_.table_high[decoder.state.reg];
	else
		handler = this_.table_low[decoder.state.reg];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Group8x64::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Group8x64>(self_ptr);
	const OpCodeHandler* handler;
	if (decoder.state.mod_ == 3) {
		// index <= 0x3F due to masking `modrm`
		handler = this_.table_high[decoder.state.modrm & 0x3F];
		if (is_null_instance_handler(handler)) {
			ICED_DEBUG_ASSERT(decoder.state.reg <= 7);
			handler = this_.table_low[decoder.state.reg];
		}
	}
	else {
		ICED_DEBUG_ASSERT(decoder.state.reg <= 7);
		handler = this_.table_low[decoder.state.reg];
	}
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Group::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Group>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.reg <= 7);
	HandlerEntry handler = this_.group_handlers[decoder.state.reg];
	handler.decode(handler.handler, decoder, instruction);
}

void OpCodeHandler_AnotherTable::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_AnotherTable>(self_ptr);
	std::size_t b;
	if (decoder.try_read_u8(b)) {
		HandlerEntry handler = this_.handlers[b];
		if (handler.handler->has_modrm) {
			std::size_t m_;
			if (!decoder.try_read_u8(m_)) {
				decoder.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
				return;
			}
			std::uint32_t m = static_cast<std::uint32_t>(m_);
			decoder.state.modrm = m;
			decoder.state.reg = (m >> 3) & 7;
			decoder.state.mod_ = m >> 6;
			decoder.state.rm = m & 7;
			decoder.state.mem_index = (decoder.state.mod_ << 3) | decoder.state.rm;
		}
		handler.decode(handler.handler, decoder, instruction);
		return;
	}
	decoder.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
}

void OpCodeHandler_MandatoryPrefix2::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_MandatoryPrefix2>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::VEX) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::EVEX) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::XOP) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	const OpCodeHandler* handler = this_.handlers[static_cast<std::size_t>(decoder.state.mandatory_prefix)];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_W::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_W>(self_ptr);
	ICED_DEBUG_ASSERT(decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::VEX) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::EVEX) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::XOP) ||
					  decoder.state.encoding() == static_cast<std::uint32_t>(EncodingKind::MVEX));
	const OpCodeHandler* handler = this_.handlers[(decoder.state.flags & StateFlags::W) != 0 ? 1 : 0];
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Bitness::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Bitness>(self_ptr);
	const OpCodeHandler* handler = decoder.is64b_mode ? this_.handler64 : this_.handler1632;
	if (handler->has_modrm)
		decoder.read_modrm();
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Bitness_DontReadModRM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Bitness_DontReadModRM>(self_ptr);
	const OpCodeHandler* handler = decoder.is64b_mode ? this_.handler64 : this_.handler1632;
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_RM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_RM>(self_ptr);
	const OpCodeHandler* handler = decoder.state.mod_ == 3 ? this_.reg : this_.mem;
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Options1632::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Options1632>(self_ptr);
	const OpCodeHandler* handler = this_.default_handler;
	std::uint32_t options = decoder.options;
	if (!decoder.is64b_mode && (decoder.options & this_.info_options) != 0) {
		for (const auto& info : this_.infos) {
			if ((options & info.options) != 0) {
				handler = info.handler;
				break;
			}
		}
	}
	if (handler->has_modrm)
		decoder.read_modrm();
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Options::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Options>(self_ptr);
	const OpCodeHandler* handler = this_.default_handler;
	std::uint32_t options = decoder.options;
	if ((decoder.options & this_.info_options) != 0) {
		for (const auto& info : this_.infos) {
			if ((options & info.options) != 0) {
				handler = info.handler;
				break;
			}
		}
	}
	if (handler->has_modrm)
		decoder.read_modrm();
	handler->decode(handler, decoder, instruction);
}

void OpCodeHandler_Options_DontReadModRM::decode(const OpCodeHandler* self_ptr, DecoderCore& decoder, Instruction& instruction) noexcept {
	const auto& this_ = handler_this<OpCodeHandler_Options_DontReadModRM>(self_ptr);
	const OpCodeHandler* handler = this_.default_handler;
	std::uint32_t options = decoder.options;
	if ((options & this_.flags) != 0)
		handler = this_.opt_handler;
	handler->decode(handler, decoder, instruction);
}

} // namespace iced_x86::internal
