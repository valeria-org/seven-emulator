// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/decoder/decoder_core.hpp"
#include "internal/decoder/tables.hpp"

#include <algorithm>

namespace iced_x86 {

namespace internal {

namespace {
struct MemRegs16 {
	Register base;
	Register index;
};
} // namespace

static constexpr MemRegs16 MEM_REGS_16[8] = {
	{Register::BX, Register::SI},
	{Register::BX, Register::DI},
	{Register::BP, Register::SI},
	{Register::BP, Register::DI},
	{Register::SI, Register::None},
	{Register::DI, Register::None},
	{Register::BP, Register::None},
	{Register::BX, Register::None},
};

static_assert(static_cast<std::uint32_t>(OpSize::Size16) == 0 && static_cast<std::uint32_t>(OpSize::Size32) == 1 &&
				  static_cast<std::uint32_t>(OpSize::Size64) == 2,
			  "");
static_assert(sizeof(OpSize) == 1, "");
static_assert(sizeof(VectorLength) == 4, "");

// Returns an error message or nullptr if it succeeded
const char* DecoderCore::init(DecoderCore& self, std::uint32_t bitness, const std::uint8_t* data, std::size_t data_len, std::uint64_t ip,
							  std::uint32_t options) noexcept {
	bool is64b_mode;
	CodeSize default_code_size;
	OpSize default_operand_size;
	OpSize default_inverted_operand_size;
	OpSize default_address_size;
	OpSize default_inverted_address_size;
	switch (bitness) {
	case 64:
		is64b_mode = true;
		default_code_size = CodeSize::Code64;
		default_operand_size = OpSize::Size32;
		default_inverted_operand_size = OpSize::Size16;
		default_address_size = OpSize::Size64;
		default_inverted_address_size = OpSize::Size32;
		break;
	case 32:
		is64b_mode = false;
		default_code_size = CodeSize::Code32;
		default_operand_size = OpSize::Size32;
		default_inverted_operand_size = OpSize::Size16;
		default_address_size = OpSize::Size32;
		default_inverted_address_size = OpSize::Size16;
		break;
	case 16:
		is64b_mode = false;
		default_code_size = CodeSize::Code16;
		default_operand_size = OpSize::Size16;
		default_inverted_operand_size = OpSize::Size32;
		default_address_size = OpSize::Size16;
		default_inverted_address_size = OpSize::Size32;
		break;
	default:
		return "Invalid bitness";
	}
	std::uintptr_t data_start = reinterpret_cast<std::uintptr_t>(data);
	std::uintptr_t data_ptr_end = data_start + data_len;
	// Verify that max_data_ptr can never overflow and that data_ptr + N can't overflow.
	// Both of them can equal data_ptr_end (1 byte past the last valid byte).
	// When reading a u8/u16/u32..., we calculate data_ptr + {1,2,4,...MAX_READ_SIZE} so it must not overflow.
	// In decode_out(), we calculate data_ptr + MAX_INSTRUCTION_LENGTH so it must not overflow.
	if (data_ptr_end < data_start ||
		data_ptr_end + std::max<std::uintptr_t>(IcedConstants::MAX_INSTRUCTION_LENGTH, MAX_READ_SIZE) < data_start)
		return "Invalid slice";

	const DecoderTables& tables = get_decoder_tables();

	self.ip = ip;
	self.data_ptr = data_start;
	self.data_ptr_end = data_ptr_end;
	self.max_data_ptr = data_start;
	self.instr_start_data_ptr = data_start;
	self.handlers_map0 = tables.handlers_map0;
	self.handlers_vex_map0 = tables.handlers_vex_map0;
	self.handlers_vex[0] = tables.handlers_vex_0f;
	self.handlers_vex[1] = tables.handlers_vex_0f38;
	self.handlers_vex[2] = tables.handlers_vex_0f3a;
	self.handlers_evex[0] = tables.handlers_evex_0f;
	self.handlers_evex[1] = tables.handlers_evex_0f38;
	self.handlers_evex[2] = tables.handlers_evex_0f3a;
	self.handlers_evex[3] = tables.invalid_map;
	self.handlers_evex[4] = tables.handlers_evex_map5;
	self.handlers_evex[5] = tables.handlers_evex_map6;
	self.handlers_xop[0] = tables.handlers_xop_map8;
	self.handlers_xop[1] = tables.handlers_xop_map9;
	self.handlers_xop[2] = tables.handlers_xop_map10;
	self.handlers_mvex[0] = tables.handlers_mvex_0f;
	self.handlers_mvex[1] = tables.handlers_mvex_0f38;
	self.handlers_mvex[2] = tables.handlers_mvex_0f3a;
	self.state = DecoderState();
	self.options = options;
	self.invalid_check_mask = (options & DecoderOptions::NO_INVALID_CHECK) == 0 ? UINT32_MAX : 0;
	self.is64b_mode_and_w = is64b_mode ? StateFlags::W : 0;
	self.reg15_mask = is64b_mode ? 0xF : 0x7;
	self.mask_e0 = is64b_mode ? 0xE0 : 0;
	self.rex_mask = is64b_mode ? 0xF0 : 0;
	self.bitness = bitness;
	self.default_address_size = default_address_size;
	self.default_operand_size = default_operand_size;
	self.segment_prio = 0;
	self.dummy = 0;
	self.default_inverted_address_size = default_inverted_address_size;
	self.default_inverted_operand_size = default_inverted_operand_size;
	self.is64b_mode = is64b_mode;
	self.default_code_size = default_code_size;
	self.displ_index = 0;
	self.data = data;
	self.data_len = data_len;
	return nullptr;
}

// The rarely used code of decode_out() (decode_out_inline() in decoder.hpp): invalid instructions, LOCK prefix and
// IP_REL32 (IP_REL64 without IS_INVALID/LOCK is handled by decode_out_inline()).
void DecoderCore::decode_out_slow(Instruction& instruction, std::uintptr_t data_ptr_, std::uint64_t orig_ip) noexcept {
	static_assert(offsetof(DecoderState, operand_size) == offsetof(DecoderState, address_size) + 1, "");
	static_assert(offsetof(DecoderState, segment_prio) == offsetof(DecoderState, address_size) + 2, "");
	static_assert(offsetof(DecoderState, dummy) == offsetof(DecoderState, address_size) + 3, "");
	static_assert(offsetof(DecoderCore, default_operand_size) == offsetof(DecoderCore, default_address_size) + 1, "");
	static_assert(offsetof(DecoderCore, segment_prio) == offsetof(DecoderCore, default_address_size) + 2, "");
	static_assert(offsetof(DecoderCore, dummy) == offsetof(DecoderCore, default_address_size) + 3, "");

	ICED_DEBUG_ASSERT(data_ptr_ == instr_start_data_ptr);
	std::uint32_t flags = state.flags;
	std::uint64_t ip_ = ip;
	ICED_DEBUG_ASSERT((flags & (StateFlags::IS_INVALID | StateFlags::LOCK | StateFlags::IP_REL64 | StateFlags::IP_REL32)) != 0);
	ICED_DEBUG_ASSERT((flags & (StateFlags::IP_REL64 | StateFlags::IS_INVALID | StateFlags::LOCK)) != StateFlags::IP_REL64);
	if ((flags & StateFlags::IP_REL64) != 0)
		instruction.set_memory_displacement64(ip_ + instruction.memory_displacement64());
	if ((flags & StateFlags::IP_REL32) != 0) {
		std::uint64_t addr32 = ip_ + instruction.memory_displacement64();
		instruction.set_memory_displacement64(static_cast<std::uint32_t>(addr32));
	}

	if ((flags & StateFlags::IS_INVALID) != 0 ||
		(((flags & (StateFlags::LOCK | StateFlags::ALLOW_LOCK)) & invalid_check_mask) == StateFlags::LOCK)) {
		instruction = Instruction();
		static_assert(static_cast<std::uint32_t>(Code::INVALID) == 0, "");
		// instruction.set_code(Code::INVALID);

		if ((flags & StateFlags::NO_MORE_BYTES) != 0) {
			std::uintptr_t max_len = data_ptr_end - data_ptr_;
			// If max-instr-len bytes is available, it's never no-more-bytes, and always invalid-instr
			if (max_len >= IcedConstants::MAX_INSTRUCTION_LENGTH)
				flags &= ~StateFlags::NO_MORE_BYTES;
			// max_data_ptr is in `data` or at most 1 byte past the last valid byte
			data_ptr = max_data_ptr;
		}

		state.flags = flags | StateFlags::IS_INVALID;

		std::uint32_t instr_len2 = static_cast<std::uint32_t>(data_ptr) - static_cast<std::uint32_t>(data_ptr_);
		InstructionInternal::internal_set_len(instruction, instr_len2);
		std::uint64_t ip2 = orig_ip + instr_len2;
		ip = ip2;
		instruction.set_next_ip(ip2);
		InstructionInternal::internal_set_code_size(instruction, default_code_size);
	}
}

void DecoderCore::set_xacquire_xrelease_core(Instruction& instruction, std::uint32_t flags) noexcept {
	ICED_DEBUG_ASSERT(!((flags & HandlerFlags::XACQUIRE_XRELEASE_NO_LOCK) == 0 && !instruction.has_lock_prefix()));
	(void)flags;
	switch (state.mandatory_prefix) {
	case DecoderMandatoryPrefix::PF2:
		clear_mandatory_prefix_f2(instruction);
		instruction.set_has_xacquire_prefix(true);
		break;
	case DecoderMandatoryPrefix::PF3:
		clear_mandatory_prefix_f3(instruction);
		instruction.set_has_xrelease_prefix(true);
		break;
	default:
		break;
	}
}

void DecoderCore::vex2(Instruction& instruction) noexcept {
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	if ((((state.flags & StateFlags::HAS_REX) | static_cast<std::uint32_t>(state.mandatory_prefix)) & invalid_check_mask) != 0)
		set_invalid_instruction();
	// Undo what decode_out() did if it got a REX prefix
	state.flags &= ~StateFlags::W;
	state.extra_index_register_base = 0;
	state.extra_base_register_base = 0;

#ifndef NDEBUG
	state.flags |= static_cast<std::uint32_t>(EncodingKind::VEX) << StateFlags::ENCODING_SHIFT;
#endif

	std::size_t b_ = read_u8();
	HandlerEntry handler = handlers_vex[0][b_];

	std::uint32_t b = state.modrm;

	static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
	static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
	state.vector_length = static_cast<VectorLength>((b >> 2) & 1);

	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::P66) == 1, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF3) == 2, "");
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PF2) == 3, "");
	state.mandatory_prefix = static_cast<DecoderMandatoryPrefix>(b & 3);

	b = ~b;
	state.extra_register_base = (b >> 4) & 8;

	// Bit 6 can only be 0 if it's 16/32-bit mode, so we don't need to change the mask
	b = (b >> 3) & 0x0F;
	state.vvvv = b;
	state.vvvv_invalid_check = b;

	decode_table2(handler, instruction);
}

void DecoderCore::vex3(Instruction& instruction) noexcept {
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	if ((((state.flags & StateFlags::HAS_REX) | static_cast<std::uint32_t>(state.mandatory_prefix)) & invalid_check_mask) != 0)
		set_invalid_instruction();
	// Undo what decode_out() did if it got a REX prefix
	state.flags &= ~StateFlags::W;

#ifndef NDEBUG
	state.flags |= static_cast<std::uint32_t>(EncodingKind::VEX) << StateFlags::ENCODING_SHIFT;
#endif

	std::uint32_t b2 = static_cast<std::uint32_t>(read_u16());

	static_assert(StateFlags::W == 0x80, "");
	state.flags |= b2 & 0x80;

	static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
	static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
	state.vector_length = static_cast<VectorLength>((b2 >> 2) & 1);

	state.mandatory_prefix = static_cast<DecoderMandatoryPrefix>(b2 & 3);

	std::uint32_t b = (~b2 >> 3) & 0x0F;
	state.vvvv_invalid_check = b;
	state.vvvv = b & reg15_mask;
	std::uint32_t b1 = state.modrm;
	std::uint32_t b1x = ~b1 & mask_e0;
	state.extra_register_base = (b1x >> 4) & 8;
	state.extra_index_register_base = (b1x >> 3) & 8;
	state.extra_base_register_base = (b1x >> 2) & 8;

	std::size_t table_index = static_cast<std::size_t>(b1 & 0x1F) - 1;
	if (table_index < sizeof(handlers_vex) / sizeof(handlers_vex[0]))
		decode_table2(handlers_vex[table_index][b2 >> 8], instruction);
	else {
		if ((b1 & 0x1F) == 0) {
			decode_table2(handlers_vex_map0[b2 >> 8], instruction);
			return;
		}
		set_invalid_instruction();
	}
}

void DecoderCore::xop(Instruction& instruction) noexcept {
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	if ((((state.flags & StateFlags::HAS_REX) | static_cast<std::uint32_t>(state.mandatory_prefix)) & invalid_check_mask) != 0)
		set_invalid_instruction();
	// Undo what decode_out() did if it got a REX prefix
	state.flags &= ~StateFlags::W;

#ifndef NDEBUG
	state.flags |= static_cast<std::uint32_t>(EncodingKind::XOP) << StateFlags::ENCODING_SHIFT;
#endif

	std::uint32_t b2 = static_cast<std::uint32_t>(read_u16());

	static_assert(StateFlags::W == 0x80, "");
	state.flags |= b2 & 0x80;

	state.vector_length = static_cast<VectorLength>((b2 >> 2) & 1);

	state.mandatory_prefix = static_cast<DecoderMandatoryPrefix>(b2 & 3);

	std::uint32_t b = (~b2 >> 3) & 0x0F;
	state.vvvv_invalid_check = b;
	state.vvvv = b & reg15_mask;
	std::uint32_t b1 = state.modrm;
	std::uint32_t b1x = ~b1 & mask_e0;
	state.extra_register_base = (b1x >> 4) & 8;
	state.extra_index_register_base = (b1x >> 3) & 8;
	state.extra_base_register_base = (b1x >> 2) & 8;

	std::size_t table_index = static_cast<std::size_t>(b1 & 0x1F) - 8;
	if (table_index < sizeof(handlers_xop) / sizeof(handlers_xop[0]))
		decode_table2(handlers_xop[table_index][b2 >> 8], instruction);
	else
		set_invalid_instruction();
}

void DecoderCore::evex_mvex(Instruction& instruction) noexcept {
	static_assert(static_cast<std::uint32_t>(DecoderMandatoryPrefix::PNP) == 0, "");
	if ((((state.flags & StateFlags::HAS_REX) | static_cast<std::uint32_t>(state.mandatory_prefix)) & invalid_check_mask) != 0)
		set_invalid_instruction();
	// Undo what decode_out() did if it got a REX prefix
	state.flags &= ~StateFlags::W;

	std::uint32_t d = static_cast<std::uint32_t>(read_u32());
	if ((d & 4) != 0) {
		std::uint32_t p0 = state.modrm;
		if ((p0 & 8) == 0) {
#ifndef NDEBUG
			state.flags |= static_cast<std::uint32_t>(EncodingKind::EVEX) << StateFlags::ENCODING_SHIFT;
#endif

			state.mandatory_prefix = static_cast<DecoderMandatoryPrefix>(d & 3);

			static_assert(StateFlags::W == 0x80, "");
			state.flags |= d & 0x80;

			std::uint32_t p2 = d >> 8;
			std::uint32_t aaa = p2 & 7;
			state.aaa = aaa;
			InstructionInternal::internal_set_op_mask(instruction, aaa);
			if ((p2 & 0x80) != 0) {
				// invalid if aaa == 0 and if we check for invalid instructions (it's all 1s)
				if ((aaa ^ invalid_check_mask) == UINT32_MAX)
					set_invalid_instruction();
				state.flags |= StateFlags::Z;
				instruction.set_zeroing_masking(true);
			}

			static_assert(StateFlags::B == 0x10, "");
			state.flags |= p2 & 0x10;

			static_assert(static_cast<std::uint32_t>(VectorLength::L128) == 0, "");
			static_assert(static_cast<std::uint32_t>(VectorLength::L256) == 1, "");
			static_assert(static_cast<std::uint32_t>(VectorLength::L512) == 2, "");
			static_assert(static_cast<std::uint32_t>(VectorLength::Unknown) == 3, "");
			state.vector_length = static_cast<VectorLength>((p2 >> 5) & 3);

			std::uint32_t p1 = (~d >> 3) & 0x0F;
			if (is64b_mode) {
				std::uint32_t tmp = (~p2 & 8) << 1;
				state.extra_index_register_base_vsib = tmp;
				tmp += p1;
				state.vvvv = tmp;
				state.vvvv_invalid_check = tmp;
				std::uint32_t p0x = ~p0;
				state.extra_register_base = (p0x >> 4) & 8;
				state.extra_index_register_base = (p0x >> 3) & 8;
				state.extra_register_base_evex = p0x & 0x10;
				p0x >>= 2;
				state.extra_base_register_base_evex = p0x & 0x18;
				state.extra_base_register_base = p0x & 8;
			}
			else {
				state.vvvv_invalid_check = p1;
				state.vvvv = p1 & 0x07;
				static_assert(StateFlags::IS_INVALID == 0x40, "");
				state.flags |= (~p2 & 8) << 3;
			}

			std::size_t table_index = static_cast<std::size_t>(p0 & 7) - 1;
			if (table_index < sizeof(handlers_evex) / sizeof(handlers_evex[0])) {
				HandlerEntry handler = handlers_evex[table_index][static_cast<std::uint8_t>(d >> 16)];
				ICED_DEBUG_ASSERT(handler.handler->has_modrm);
				std::uint32_t m = d >> 24;
				state.modrm = m;
				state.reg = (m >> 3) & 7;
				state.mod_ = m >> 6;
				state.rm = m & 7;
				state.mem_index = (state.mod_ << 3) | state.rm;
				// Invalid if LL=3 and no rc
				static_assert(StateFlags::B > 3, "");
				ICED_DEBUG_ASSERT(static_cast<std::uint32_t>(state.vector_length) <= 3);
				if ((((state.flags & StateFlags::B) | static_cast<std::uint32_t>(state.vector_length)) & invalid_check_mask) == 3)
					set_invalid_instruction();
				handler.decode(handler.handler, *this, instruction);
			}
			else
				set_invalid_instruction();
		}
		else
			set_invalid_instruction();
	}
	else {
		if ((options & DecoderOptions::KNC) == 0 || !is64b_mode)
			set_invalid_instruction();
		else {
			std::uint32_t p0 = state.modrm;
#ifndef NDEBUG
			state.flags |= static_cast<std::uint32_t>(EncodingKind::MVEX) << StateFlags::ENCODING_SHIFT;
#endif

			state.mandatory_prefix = static_cast<DecoderMandatoryPrefix>(d & 3);

			static_assert(StateFlags::W == 0x80, "");
			state.flags |= d & 0x80;

			std::uint32_t p2 = d >> 8;
			std::uint32_t aaa = p2 & 7;
			state.aaa = aaa;
			InstructionInternal::internal_set_op_mask(instruction, aaa);

			static_assert(StateFlags::MVEX_SSS_SHIFT == 16, "");
			static_assert(StateFlags::MVEX_SSS_MASK == 7, "");
			static_assert(StateFlags::MVEX_EH == 1U << (StateFlags::MVEX_SSS_SHIFT + 3), "");
			state.flags |= (p2 & 0xF0) << (StateFlags::MVEX_SSS_SHIFT - 4);

			std::uint32_t p1 = (~d >> 3) & 0x0F;
			std::uint32_t tmp = (~p2 & 8) << 1;
			state.extra_index_register_base_vsib = tmp;
			tmp += p1;
			state.vvvv = tmp;
			state.vvvv_invalid_check = tmp;
			std::uint32_t p0x = ~p0;
			state.extra_register_base = (p0x >> 4) & 8;
			state.extra_index_register_base = (p0x >> 3) & 8;
			state.extra_register_base_evex = p0x & 0x10;
			p0x >>= 2;
			state.extra_base_register_base_evex = p0x & 0x18;
			state.extra_base_register_base = p0x & 8;

			std::size_t table_index = static_cast<std::size_t>(p0 & 0xF) - 1;
			if (table_index < sizeof(handlers_mvex) / sizeof(handlers_mvex[0])) {
				HandlerEntry handler = handlers_mvex[table_index][static_cast<std::uint8_t>(d >> 16)];
				ICED_DEBUG_ASSERT(handler.handler->has_modrm);
				std::uint32_t m = d >> 24;
				state.modrm = m;
				state.reg = (m >> 3) & 7;
				state.mod_ = m >> 6;
				state.rm = m & 7;
				state.mem_index = (state.mod_ << 3) | state.rm;
				handler.decode(handler.handler, *this, instruction);
			}
			else
				set_invalid_instruction();
		}
	}
}

// It's small enough that the compiler wants to inline it but almost no-one will
// disassemble code with 16-bit addressing.
ICED_NOINLINE void DecoderCore::read_op_mem_16(Instruction& instruction, TupleType tuple_type) noexcept {
	ICED_DEBUG_ASSERT(state.address_size == OpSize::Size16);
	ICED_DEBUG_ASSERT(state.rm <= 7);
	Register base_reg = MEM_REGS_16[state.rm].base;
	Register index_reg = MEM_REGS_16[state.rm].index;
	switch (state.mod_) {
	case 0:
		if (state.rm == 6) {
			InstructionInternal::internal_set_memory_displ_size(instruction, 2);
			displ_index = static_cast<std::uint8_t>(data_ptr);
			instruction.set_memory_displacement64(read_u16());
			base_reg = Register::None;
			ICED_DEBUG_ASSERT(index_reg == Register::None);
		}
		break;
	case 1: {
		InstructionInternal::internal_set_memory_displ_size(instruction, 1);
		displ_index = static_cast<std::uint8_t>(data_ptr);
		std::size_t b = read_u8();
		instruction.set_memory_displacement64(
			static_cast<std::uint16_t>(disp8n(tuple_type) * static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(b)))));
		break;
	}
	default:
		ICED_DEBUG_ASSERT(state.mod_ == 2);
		InstructionInternal::internal_set_memory_displ_size(instruction, 2);
		displ_index = static_cast<std::uint8_t>(data_ptr);
		instruction.set_memory_displacement64(read_u16());
		break;
	}
	instruction.set_memory_base(base_reg);
	instruction.set_memory_index(index_reg);
}

// Sign extends a byte to 64 bits (Rust: `b as i8 as u64`)
static ICED_FORCE_INLINE std::uint64_t sext8_64(std::size_t b) noexcept {
	return static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(b)));
}
// Sign extends a byte to 32 bits (Rust: `b as i8 as u32`)
static ICED_FORCE_INLINE std::uint32_t sext8_32(std::size_t b) noexcept {
	return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(b)));
}
// Sign extends a dword to 64 bits (Rust: `d as i32 as u64`)
static ICED_FORCE_INLINE std::uint64_t sext32_64(std::size_t d) noexcept {
	return static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(d))));
}

bool DecoderCore::read_op_mem_1(Instruction& instruction, DecoderCore& self) noexcept {
	InstructionInternal::internal_set_memory_displ_size(instruction, 1);
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t b;
	if (self.try_read_u8(b)) {
		std::uint64_t displ = sext8_64(b);
		if (self.state.address_size == OpSize::Size64) {
			instruction.set_memory_displacement64(displ);
			write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::RAX));
		}
		else {
			instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
			write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::EAX));
		}
		return false;
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return false;
}

bool DecoderCore::read_op_mem_1_4(Instruction& instruction, DecoderCore& self) noexcept {
	InstructionInternal::internal_set_memory_displ_size(instruction, 1);

	self.displ_index = static_cast<std::uint8_t>(self.data_ptr + 1);
	std::size_t w_;
	if (self.try_read_u16(w_)) {
		std::uint32_t w = static_cast<std::uint32_t>(w_);

		static_assert(static_cast<std::uint32_t>(InstrScale::Scale1) == 0, "");
		static_assert(static_cast<std::uint32_t>(InstrScale::Scale2) == 1, "");
		static_assert(static_cast<std::uint32_t>(InstrScale::Scale4) == 2, "");
		static_assert(static_cast<std::uint32_t>(InstrScale::Scale8) == 3, "");
		InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>((w >> 6) & 3));
		std::uint32_t index = ((w >> 3) & 7) + self.state.extra_index_register_base;
		if (self.state.address_size == OpSize::Size64) {
			constexpr std::uint32_t BASE_REG = reg_u32(Register::RAX);
			if (index != 4)
				write_index_reg(instruction, index + BASE_REG);

			write_base_reg(instruction, (w & 7) + self.state.extra_base_register_base + BASE_REG);
			std::uint64_t displ = sext8_64(w >> 8);
			instruction.set_memory_displacement64(displ);
		}
		else {
			constexpr std::uint32_t BASE_REG = reg_u32(Register::EAX);
			if (index != 4)
				write_index_reg(instruction, index + BASE_REG);

			write_base_reg(instruction, (w & 7) + self.state.extra_base_register_base + BASE_REG);
			std::uint64_t displ = sext8_32(w >> 8);
			instruction.set_memory_displacement64(displ);
		}

		return true;
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return true;
}

bool DecoderCore::read_op_mem_0(Instruction& instruction, DecoderCore& self) noexcept {
	if (self.state.address_size == OpSize::Size64)
		write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::RAX));
	else
		write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::EAX));

	return false;
}

bool DecoderCore::read_op_mem_0_5(Instruction& instruction, DecoderCore& self) noexcept {
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t d;
	if (self.try_read_u32(d)) {
		std::uint64_t displ = sext32_64(d);
		if (self.state.address_size == OpSize::Size64) {
			ICED_DEBUG_ASSERT(self.is64b_mode);
			self.state.flags |= StateFlags::IP_REL64;
			instruction.set_memory_displacement64(displ);
			InstructionInternal::internal_set_memory_displ_size(instruction, 4);
			instruction.set_memory_base(Register::RIP);
		}
		else if (self.is64b_mode) {
			self.state.flags |= StateFlags::IP_REL32;
			instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
			InstructionInternal::internal_set_memory_displ_size(instruction, 3);
			instruction.set_memory_base(Register::EIP);
		}
		else {
			instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
			InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		}

		return false;
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return false;
}

bool DecoderCore::read_op_mem_2_4(Instruction& instruction, DecoderCore& self) noexcept {
	std::size_t sib_;
	if (self.try_read_u8(sib_)) {
		std::uint32_t sib = static_cast<std::uint32_t>(sib_);
		self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
		std::size_t d;
		if (self.try_read_u32(d)) {
			std::uint64_t displ = sext32_64(d);

			InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>(sib >> 6));
			std::uint32_t index = ((sib >> 3) & 7) + self.state.extra_index_register_base;
			if (self.state.address_size == OpSize::Size64) {
				constexpr std::uint32_t BASE_REG = reg_u32(Register::RAX);
				if (index != 4)
					write_index_reg(instruction, index + BASE_REG);

				write_base_reg(instruction, (sib & 7) + self.state.extra_base_register_base + BASE_REG);
				InstructionInternal::internal_set_memory_displ_size(instruction, 4);
				instruction.set_memory_displacement64(displ);
			}
			else {
				constexpr std::uint32_t BASE_REG = reg_u32(Register::EAX);
				if (index != 4)
					write_index_reg(instruction, index + BASE_REG);

				write_base_reg(instruction, (sib & 7) + self.state.extra_base_register_base + BASE_REG);
				InstructionInternal::internal_set_memory_displ_size(instruction, 3);
				instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
			}

			return true;
		}
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return true;
}

bool DecoderCore::read_op_mem_2(Instruction& instruction, DecoderCore& self) noexcept {
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t d;
	if (self.try_read_u32(d)) {
		std::uint64_t displ = sext32_64(d);
		if (self.state.address_size == OpSize::Size64) {
			instruction.set_memory_displacement64(displ);
			InstructionInternal::internal_set_memory_displ_size(instruction, 4);
			write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::RAX));
		}
		else {
			instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
			InstructionInternal::internal_set_memory_displ_size(instruction, 3);
			write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::EAX));
		}

		return false;
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return false;
}

bool DecoderCore::read_op_mem_0_4(Instruction& instruction, DecoderCore& self) noexcept {
	std::size_t sib_;
	if (self.try_read_u8(sib_)) {
		std::uint32_t sib = static_cast<std::uint32_t>(sib_);
		InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>(sib >> 6));
		std::uint32_t index = ((sib >> 3) & 7) + self.state.extra_index_register_base;
		std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
		if (index != 4)
			write_index_reg(instruction, index + base_reg);

		std::uint32_t base = sib & 7;
		if (base == 5) {
			self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
			std::size_t d;
			if (!self.try_read_u32(d)) {
				self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
				return true;
			}
			std::uint64_t displ = sext32_64(d);
			if (self.state.address_size == OpSize::Size64) {
				instruction.set_memory_displacement64(displ);
				InstructionInternal::internal_set_memory_displ_size(instruction, 4);
			}
			else {
				instruction.set_memory_displacement64(static_cast<std::uint32_t>(displ));
				InstructionInternal::internal_set_memory_displ_size(instruction, 3);
			}
		}
		else {
			write_base_reg(instruction, base + self.state.extra_base_register_base + base_reg);
			InstructionInternal::internal_set_memory_displ_size(instruction, 0);
			instruction.set_memory_displacement64(0);
		}

		return true;
	}
	self.state.flags |= StateFlags::IS_INVALID | StateFlags::NO_MORE_BYTES;
	return true;
}

bool DecoderCore::read_op_mem_vsib_1(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	InstructionInternal::internal_set_memory_displ_size(instruction, 1);
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t b = self.read_u8();
	if (self.state.address_size == OpSize::Size64) {
		write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::RAX));
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(self.disp8n(tuple_type)) * sext8_64(b));
	}
	else {
		write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + reg_u32(Register::EAX));
		instruction.set_memory_displacement64(self.disp8n(tuple_type) * sext8_32(b));
	}

	return false;
}

bool DecoderCore::read_op_mem_vsib_1_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	InstructionInternal::internal_set_memory_displ_size(instruction, 1);

	self.displ_index = static_cast<std::uint8_t>(self.data_ptr + 1);
	std::uint32_t sib = static_cast<std::uint32_t>(self.read_u16());
	std::uint32_t index = ((sib >> 3) & 7) + self.state.extra_index_register_base;
	if (!is_vsib) {
		if (index != 4)
			write_index_reg(instruction, index + reg_u32(index_reg));
	}
	else
		write_index_reg(instruction, index + self.state.extra_index_register_base_vsib + reg_u32(index_reg));

	InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>((sib >> 6) & 3));
	std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
	write_base_reg(instruction, (sib & 7) + self.state.extra_base_register_base + base_reg);

	std::uint32_t b = sext8_32(sib >> 8);
	std::uint32_t displ = self.disp8n(tuple_type) * b;
	if (self.state.address_size == OpSize::Size64)
		instruction.set_memory_displacement64(static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(displ))));
	else
		instruction.set_memory_displacement64(displ);

	return true;
}

bool DecoderCore::read_op_mem_vsib_0(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
	write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + base_reg);

	return false;
}

bool DecoderCore::read_op_mem_vsib_0_5(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t d = self.read_u32();
	if (self.state.address_size == OpSize::Size64) {
		ICED_DEBUG_ASSERT(self.is64b_mode);
		self.state.flags |= StateFlags::IP_REL64;
		instruction.set_memory_displacement64(sext32_64(d));
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		instruction.set_memory_base(Register::RIP);
	}
	else if (self.is64b_mode) {
		self.state.flags |= StateFlags::IP_REL32;
		instruction.set_memory_displacement64(d);
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_base(Register::EIP);
	}
	else {
		instruction.set_memory_displacement64(d);
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
	}

	return false;
}

bool DecoderCore::read_op_mem_vsib_2_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	std::uint32_t sib = static_cast<std::uint32_t>(self.read_u8());
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);

	std::uint32_t index = ((sib >> 3) & 7) + self.state.extra_index_register_base;
	if (!is_vsib) {
		if (index != 4)
			write_index_reg(instruction, index + reg_u32(index_reg));
	}
	else
		write_index_reg(instruction, index + self.state.extra_index_register_base_vsib + reg_u32(index_reg));

	InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>(sib >> 6));

	std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
	write_base_reg(instruction, (sib & 7) + self.state.extra_base_register_base + base_reg);
	std::uint32_t displ = static_cast<std::uint32_t>(self.read_u32());
	if (self.state.address_size == OpSize::Size64) {
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		instruction.set_memory_displacement64(sext32_64(displ));
	}
	else {
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		instruction.set_memory_displacement64(displ);
	}

	return true;
}

bool DecoderCore::read_op_mem_vsib_2(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
	write_base_reg(instruction, self.state.extra_base_register_base + self.state.rm + base_reg);
	self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
	std::size_t d = self.read_u32();
	if (self.state.address_size == OpSize::Size64) {
		instruction.set_memory_displacement64(sext32_64(d));
		InstructionInternal::internal_set_memory_displ_size(instruction, 4);
	}
	else {
		instruction.set_memory_displacement64(d);
		InstructionInternal::internal_set_memory_displ_size(instruction, 3);
	}

	return false;
}

bool DecoderCore::read_op_mem_vsib_0_4(DecoderCore& self, Instruction& instruction, Register index_reg, TupleType tuple_type, bool is_vsib) noexcept {
	std::uint32_t sib = static_cast<std::uint32_t>(self.read_u8());
	InstructionInternal::internal_set_memory_index_scale(instruction, static_cast<InstrScale>(sib >> 6));
	std::uint32_t index = ((sib >> 3) & 7) + self.state.extra_index_register_base;
	if (!is_vsib) {
		if (index != 4)
			write_index_reg(instruction, index + reg_u32(index_reg));
	}
	else
		write_index_reg(instruction, index + self.state.extra_index_register_base_vsib + reg_u32(index_reg));

	std::uint32_t base = sib & 7;
	if (base == 5) {
		self.displ_index = static_cast<std::uint8_t>(self.data_ptr);
		std::size_t d = self.read_u32();
		if (self.state.address_size == OpSize::Size64) {
			instruction.set_memory_displacement64(sext32_64(d));
			InstructionInternal::internal_set_memory_displ_size(instruction, 4);
		}
		else {
			instruction.set_memory_displacement64(d);
			InstructionInternal::internal_set_memory_displ_size(instruction, 3);
		}
	}
	else {
		std::uint32_t base_reg = self.state.address_size == OpSize::Size64 ? reg_u32(Register::RAX) : reg_u32(Register::EAX);
		write_base_reg(instruction, base + self.state.extra_base_register_base + base_reg);
		InstructionInternal::internal_set_memory_displ_size(instruction, 0);
		instruction.set_memory_displacement64(0);
	}

	return true;
}

ConstantOffsets DecoderCore::get_constant_offsets_impl(const Instruction& instruction) const noexcept {
	struct {
		std::uint8_t displacement_offset = 0;
		std::uint8_t displacement_size = 0;
		std::uint8_t immediate_offset = 0;
		std::uint8_t immediate_size = 0;
		std::uint8_t immediate_offset2 = 0;
		std::uint8_t immediate_size2 = 0;
	} constant_offsets;

	std::uint32_t displ_size = instruction.memory_displ_size();
	if (displ_size != 0) {
		constant_offsets.displacement_offset = static_cast<std::uint8_t>(displ_index - static_cast<std::uint8_t>(instr_start_data_ptr));
		if (displ_size == 8 && (state.flags & StateFlags::ADDR64) == 0)
			constant_offsets.displacement_size = 4;
		else
			constant_offsets.displacement_size = static_cast<std::uint8_t>(displ_size);
	}

	if ((state.flags & StateFlags::NO_IMM) == 0) {
		std::uint32_t extra_imm_sub = 0;
		std::uint32_t len = instruction.len();
		for (std::uint32_t i = instruction.op_count(); i-- > 0;) {
			switch (instruction.op_kind(i)) {
			case OpKind::Immediate8:
			case OpKind::Immediate8to16:
			case OpKind::Immediate8to32:
			case OpKind::Immediate8to64:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - extra_imm_sub - 1);
				constant_offsets.immediate_size = 1;
				goto break_loop;

			case OpKind::Immediate16:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - extra_imm_sub - 2);
				constant_offsets.immediate_size = 2;
				goto break_loop;

			case OpKind::Immediate32:
			case OpKind::Immediate32to64:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - extra_imm_sub - 4);
				constant_offsets.immediate_size = 4;
				goto break_loop;

			case OpKind::Immediate64:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - extra_imm_sub - 8);
				constant_offsets.immediate_size = 8;
				goto break_loop;

			case OpKind::Immediate8_2nd:
				constant_offsets.immediate_offset2 = static_cast<std::uint8_t>(len - 1);
				constant_offsets.immediate_size2 = 1;
				extra_imm_sub = 1;
				break;

			case OpKind::NearBranch16:
				if ((state.flags & StateFlags::BRANCH_IMM8) != 0) {
					constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 1);
					constant_offsets.immediate_size = 1;
				}
				else if ((state.flags & StateFlags::XBEGIN) == 0) {
					constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 2);
					constant_offsets.immediate_size = 2;
				}
				else {
					ICED_DEBUG_ASSERT((state.flags & StateFlags::XBEGIN) != 0);
					if (state.operand_size != OpSize::Size16) {
						constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 4);
						constant_offsets.immediate_size = 4;
					}
					else {
						constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 2);
						constant_offsets.immediate_size = 2;
					}
				}
				break;

			case OpKind::NearBranch32:
			case OpKind::NearBranch64:
				if ((state.flags & StateFlags::BRANCH_IMM8) != 0) {
					constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 1);
					constant_offsets.immediate_size = 1;
				}
				else if ((state.flags & StateFlags::XBEGIN) == 0) {
					constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 4);
					constant_offsets.immediate_size = 4;
				}
				else {
					ICED_DEBUG_ASSERT((state.flags & StateFlags::XBEGIN) != 0);
					if (state.operand_size != OpSize::Size16) {
						constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 4);
						constant_offsets.immediate_size = 4;
					}
					else {
						constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - 2);
						constant_offsets.immediate_size = 2;
					}
				}
				break;

			case OpKind::FarBranch16:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - (2 + 2));
				constant_offsets.immediate_size = 2;
				constant_offsets.immediate_offset2 = static_cast<std::uint8_t>(len - 2);
				constant_offsets.immediate_size2 = 2;
				break;

			case OpKind::FarBranch32:
				constant_offsets.immediate_offset = static_cast<std::uint8_t>(len - (4 + 2));
				constant_offsets.immediate_size = 4;
				constant_offsets.immediate_offset2 = static_cast<std::uint8_t>(len - 2);
				constant_offsets.immediate_size2 = 2;
				break;

			default:
				break;
			}
		}
	break_loop:;
	}

	return ConstantOffsets(constant_offsets.displacement_offset, constant_offsets.displacement_size, constant_offsets.immediate_offset,
						   constant_offsets.immediate_size, constant_offsets.immediate_offset2, constant_offsets.immediate_size2);
}

} // namespace internal

// ---------------------------------------------------------------------------------------------------------------------
// Decoder

Decoder::Decoder(std::uint32_t bitness_, const std::uint8_t* data_, std::size_t size, std::uint32_t options_) noexcept {
	const char* error = init(*this, bitness_, data_, size, 0, options_);
	ICED_ASSERT(error == nullptr);
}

Decoder Decoder::with_ip(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint64_t ip, std::uint32_t options) noexcept {
	Decoder decoder;
	const char* error = init(decoder, bitness, data, size, ip, options);
	ICED_ASSERT(error == nullptr);
	return decoder;
}

Result<Decoder> Decoder::try_new(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint32_t options) noexcept {
	return try_with_ip(bitness, data, size, 0, options);
}

Result<Decoder> Decoder::try_with_ip(std::uint32_t bitness, const std::uint8_t* data, std::size_t size, std::uint64_t ip,
									 std::uint32_t options) noexcept {
	Decoder decoder;
	const char* error = init(decoder, bitness, data, size, ip, options);
	if (error != nullptr)
		return IcedError(error);
	return decoder;
}

DecoderError Decoder::last_error() const noexcept {
	// NoMoreBytes error has highest priority
	if ((state.flags & internal::StateFlags::NO_MORE_BYTES) != 0)
		return DecoderError::NoMoreBytes;
	else if ((state.flags & internal::StateFlags::IS_INVALID) != 0)
		return DecoderError::InvalidInstruction;
	else
		return DecoderError::None;
}

} // namespace iced_x86
