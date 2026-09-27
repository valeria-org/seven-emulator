// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/code_asm/code_assembler.hpp"
#include "iced_x86/block_encoder.hpp"
#include "iced_x86/block_encoder_options.hpp"
#include "iced_x86/code.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/memory_operand.hpp"
#include "iced_x86/register.hpp"
#include "internal/iced_assert.hpp"

#include <cstring>
#include <utility>

namespace iced_x86::code_asm {

namespace {
constexpr std::size_t MAX_DB_COUNT = 16;
constexpr std::size_t MAX_DW_COUNT = MAX_DB_COUNT / 2;
constexpr std::size_t MAX_DD_COUNT = MAX_DB_COUNT / 4;
constexpr std::size_t MAX_DQ_COUNT = MAX_DB_COUNT / 8;
} // namespace

Result<std::uint64_t> CodeAssemblerResult::label_ip(const CodeLabel& label) const {
	if (label.is_empty())
		return IcedError("Invalid label. Must be created via `CodeAssembler::create_label()`.");
	if (!label.has_instruction_index())
		return IcedError("The label is not associated with an instruction index. It must be emitted via `CodeAssembler::set_label()`.");
	if (label.instruction_index_ >= inner.new_instruction_offsets.size()) {
		return IcedError(
			"Invalid label instruction index or `BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS` option was not enabled when calling `assemble_options()`.");
	}
	const std::uint32_t new_offset = inner.new_instruction_offsets[label.instruction_index_];
	if (new_offset == UINT32_MAX) {
		return IcedError(
			"The instruction was re-written to a longer instruction (eg. JE NEAR -> JE FAR) and there's no instruction offset. Consider using a `zero_bytes()` instruction as a label instead of a normal instruction or disable branch optimizations.");
	}
	return inner.rip + static_cast<std::uint64_t>(new_offset);
}

namespace internal {

CodeAssemblerBase::CodeAssemblerBase(std::uint32_t bitness)
	: bitness_(bitness), instructions_(), current_label_id_(0), current_label_(), current_anon_label_(), next_anon_label_(),
	  defined_anon_label_(false), prefix_flags_(PrefixFlags::NONE),
	  options_(CodeAssemblerOptions::PREFER_VEX | CodeAssemblerOptions::PREFER_SHORT_BRANCH), error_() {
	if (bitness != 16 && bitness != 32 && bitness != 64)
		set_error("Invalid bitness");
}

CodeAssembler& CodeAssemblerBase::self() noexcept { return static_cast<CodeAssembler&>(*this); }

CodeAssembler& CodeAssemblerBase::set_error(const char* message) {
	if (!error_)
		error_.emplace(message);
	return self();
}

CodeAssembler& CodeAssemblerBase::set_error(const IcedError& error) {
	if (!error_)
		error_.emplace(error);
	return self();
}

CodeAssembler& CodeAssemblerBase::add_instr_with_state(const Result<Instruction>& instruction, CodeAsmOpState state) {
	if (error_)
		return self();
	if (instruction.is_err())
		return set_error(instruction.error());
	Instruction instr = instruction.value();
	if (!state.is_default()) {
		if (state.is_broadcast())
			instr.set_is_broadcast(true);
		if (state.zeroing_masking())
			instr.set_zeroing_masking(true);
		if (state.suppress_all_exceptions())
			instr.set_suppress_all_exceptions(true);
		instr.set_op_mask(state.op_mask());
		instr.set_rounding_control(state.rounding_control());
	}
	return add_instr(instr);
}

CodeAssembler& CodeAssemblerBase::add_instr(const Result<Instruction>& instruction) {
	if (error_)
		return self();
	if (instruction.is_err())
		return set_error(instruction.error());
	return add_instr(instruction.value());
}

CodeAssembler& CodeAssemblerBase::add_instr(Instruction instruction) {
	if (error_)
		return self();
	if (!current_label_.is_empty() && defined_anon_label_)
		return set_error("You can't create both an anonymous label and a normal label");
	if (!current_label_.is_empty())
		instruction.set_ip(current_label_.id());
	else if (defined_anon_label_)
		instruction.set_ip(current_anon_label_.id());

	if (prefix_flags_ != 0) {
		if ((prefix_flags_ & PrefixFlags::LOCK) != 0)
			instruction.set_has_lock_prefix(true);
		if ((prefix_flags_ & PrefixFlags::REPE) != 0)
			instruction.set_has_repe_prefix(true);
		else if ((prefix_flags_ & PrefixFlags::REPNE) != 0)
			instruction.set_has_repne_prefix(true);
		if ((prefix_flags_ & PrefixFlags::NOTRACK) != 0)
			instruction.set_segment_prefix(Register::DS);
	}

	instructions_.push_back(instruction);
	current_label_ = CodeLabel();
	defined_anon_label_ = false;
	prefix_flags_ = PrefixFlags::NONE;
	return self();
}

} // namespace internal

CodeAssembler::CodeAssembler(std::uint32_t bitness) : internal::CodeAssemblerFnsAll(bitness) {}

Result<CodeAssembler> CodeAssembler::create(std::uint32_t bitness) {
	switch (bitness) {
	case 16:
	case 32:
	case 64:
		break;
	default:
		return IcedError("Invalid bitness");
	}
	return CodeAssembler(bitness);
}

std::vector<Instruction> CodeAssembler::take_instructions() {
	std::vector<Instruction> instrs = std::move(instructions_);
	instructions_ = std::vector<Instruction>();
	reset();
	return instrs;
}

void CodeAssembler::reset() noexcept {
	instructions_.clear();
	current_label_id_ = 0;
	current_label_ = CodeLabel();
	current_anon_label_ = CodeLabel();
	next_anon_label_ = CodeLabel();
	defined_anon_label_ = false;
	prefix_flags_ = PrefixFlags::NONE;
	error_.reset();
}

CodeAssembler& CodeAssembler::set_label(CodeLabel& label) {
	if (error_)
		return *this;
	if (label.is_empty())
		return set_error("Invalid label. Must be created by create_label()");
	if (label.has_instruction_index())
		return set_error("Labels can't be re-used and can only be set once.");
	if (!current_label_.is_empty())
		return set_error("Only one label per instruction is allowed");
	label.instruction_index_ = instructions_.size();
	current_label_ = label;
	return *this;
}

CodeAssembler& CodeAssembler::anonymous_label() {
	if (error_)
		return *this;
	if (defined_anon_label_)
		return set_error("At most one anonymous label per instruction is allowed");
	current_anon_label_ = next_anon_label_.is_empty() ? create_label() : next_anon_label_;
	next_anon_label_ = CodeLabel();
	defined_anon_label_ = true;
	return *this;
}

Result<CodeLabel> CodeAssembler::bwd() const {
	if (current_anon_label_.is_empty())
		return IcedError("No anonymous label has been created yet");
	return current_anon_label_;
}

Result<CodeLabel> CodeAssembler::fwd() {
	// This method returns a `Result<T>` for consistency with other methods, including `bwd()`,
	// so you don't have to memorize which methods return a Result and which don't.
	if (next_anon_label_.is_empty())
		next_anon_label_ = create_label();
	return next_anon_label_;
}

bool CodeAssembler::decl_data_verify_no_prefixes() {
	if (error_)
		return false;
	if (prefix_flags_ != 0) {
		set_error("db/dw/dd/dq: No prefixes are allowed");
		return false;
	}
	return true;
}

CodeAssembler& CodeAssembler::db(const std::uint8_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	for (std::size_t i = 0; i < size; i += MAX_DB_COUNT) {
		const std::size_t count = size - i < MAX_DB_COUNT ? size - i : MAX_DB_COUNT;
		add_instr(Instruction::with_declare_byte(data + i, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::db_i(const std::int8_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint8_t tmp[MAX_DB_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DB_COUNT) {
		const std::size_t count = size - i < MAX_DB_COUNT ? size - i : MAX_DB_COUNT;
		for (std::size_t j = 0; j < count; j++)
			tmp[j] = static_cast<std::uint8_t>(data[i + j]);
		add_instr(Instruction::with_declare_byte(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dw(const std::uint16_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	for (std::size_t i = 0; i < size; i += MAX_DW_COUNT) {
		const std::size_t count = size - i < MAX_DW_COUNT ? size - i : MAX_DW_COUNT;
		add_instr(Instruction::with_declare_word(data + i, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dw_i(const std::int16_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint16_t tmp[MAX_DW_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DW_COUNT) {
		const std::size_t count = size - i < MAX_DW_COUNT ? size - i : MAX_DW_COUNT;
		for (std::size_t j = 0; j < count; j++)
			tmp[j] = static_cast<std::uint16_t>(data[i + j]);
		add_instr(Instruction::with_declare_word(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dd(const std::uint32_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	for (std::size_t i = 0; i < size; i += MAX_DD_COUNT) {
		const std::size_t count = size - i < MAX_DD_COUNT ? size - i : MAX_DD_COUNT;
		add_instr(Instruction::with_declare_dword(data + i, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dd_i(const std::int32_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint32_t tmp[MAX_DD_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DD_COUNT) {
		const std::size_t count = size - i < MAX_DD_COUNT ? size - i : MAX_DD_COUNT;
		for (std::size_t j = 0; j < count; j++)
			tmp[j] = static_cast<std::uint32_t>(data[i + j]);
		add_instr(Instruction::with_declare_dword(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dd_f32(const float* data, std::size_t size) {
	static_assert(sizeof(float) == sizeof(std::uint32_t), "");
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint32_t tmp[MAX_DD_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DD_COUNT) {
		const std::size_t count = size - i < MAX_DD_COUNT ? size - i : MAX_DD_COUNT;
		for (std::size_t j = 0; j < count; j++)
			std::memcpy(&tmp[j], &data[i + j], sizeof(std::uint32_t));
		add_instr(Instruction::with_declare_dword(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dq(const std::uint64_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	for (std::size_t i = 0; i < size; i += MAX_DQ_COUNT) {
		const std::size_t count = size - i < MAX_DQ_COUNT ? size - i : MAX_DQ_COUNT;
		add_instr(Instruction::with_declare_qword(data + i, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dq_i(const std::int64_t* data, std::size_t size) {
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint64_t tmp[MAX_DQ_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DQ_COUNT) {
		const std::size_t count = size - i < MAX_DQ_COUNT ? size - i : MAX_DQ_COUNT;
		for (std::size_t j = 0; j < count; j++)
			tmp[j] = static_cast<std::uint64_t>(data[i + j]);
		add_instr(Instruction::with_declare_qword(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::dq_f64(const double* data, std::size_t size) {
	static_assert(sizeof(double) == sizeof(std::uint64_t), "");
	if (!decl_data_verify_no_prefixes())
		return *this;
	std::uint64_t tmp[MAX_DQ_COUNT];
	for (std::size_t i = 0; i < size; i += MAX_DQ_COUNT) {
		const std::size_t count = size - i < MAX_DQ_COUNT ? size - i : MAX_DQ_COUNT;
		for (std::size_t j = 0; j < count; j++)
			std::memcpy(&tmp[j], &data[i + j], sizeof(std::uint64_t));
		add_instr(Instruction::with_declare_qword(tmp, count));
	}
	return *this;
}

CodeAssembler& CodeAssembler::nops_with_size(std::size_t size) {
	if (error_)
		return *this;
	if (prefix_flags_ != 0)
		return set_error("No prefixes are allowed");

	constexpr std::size_t MAX_NOP_LEN = 9;
	if (size >= MAX_NOP_LEN) {
		const std::uint8_t* bytes = get_nop_bytes(MAX_NOP_LEN);
		for (std::size_t i = 0; i < size / MAX_NOP_LEN; i++)
			db(bytes, MAX_NOP_LEN);
	}
	const std::size_t remaining = size % MAX_NOP_LEN;
	if (remaining != 0)
		db(get_nop_bytes(remaining), remaining);

	return *this;
}

const std::uint8_t* CodeAssembler::get_nop_bytes(std::size_t size) const noexcept {
	static const std::uint8_t NOP1[] = {0x90}; // NOP
	static const std::uint8_t NOP2[] = {0x66, 0x90}; // 66 NOP
	static const std::uint8_t NOP3[] = {0x0F, 0x1F, 0x00}; // NOP dword ptr [eax] or NOP word ptr [bx+si]
	static const std::uint8_t NOP4[] = {0x0F, 0x1F, 0x40, 0x00}; // NOP dword ptr [eax + 00] or NOP word ptr [bx+si]
	static const std::uint8_t NOP5_32[] = {0x0F, 0x1F, 0x44, 0x00, 0x00}; // NOP dword ptr [eax + eax*1 + 00]
	static const std::uint8_t NOP5_16[] = {0x0F, 0x1F, 0x80, 0x00, 0x00}; // NOP word ptr[bx + si]
	static const std::uint8_t NOP6_32[] = {0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00}; // 66 NOP dword ptr [eax + eax*1 + 00]
	static const std::uint8_t NOP6_16[] = {0x66, 0x0F, 0x1F, 0x80, 0x00, 0x00}; // NOP dword ptr [bx+si]
	static const std::uint8_t NOP7_32[] = {0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00}; // NOP dword ptr [eax + 00000000]
	static const std::uint8_t NOP7_16[] = {0x67, 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00}; // NOP dword ptr [eax+eax]
	static const std::uint8_t NOP8_32[] = {0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}; // NOP dword ptr [eax + eax*1 + 00000000]
	static const std::uint8_t NOP8_16[] = {0x67, 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00}; // NOP word ptr [eax]
	static const std::uint8_t NOP9_32[] = {0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}; // 66 NOP dword ptr [eax + eax*1 + 00000000]
	static const std::uint8_t NOP9_16[] = {0x67, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}; // NOP word ptr [eax+eax]

	switch (size) {
	case 1:
		return NOP1;
	case 2:
		return NOP2;
	case 3:
		return NOP3;
	case 4:
		return NOP4;
	case 5:
		return bitness_ != 16 ? NOP5_32 : NOP5_16;
	case 6:
		return bitness_ != 16 ? NOP6_32 : NOP6_16;
	case 7:
		return bitness_ != 16 ? NOP7_32 : NOP7_16;
	case 8:
		return bitness_ != 16 ? NOP8_32 : NOP8_16;
	case 9:
		return bitness_ != 16 ? NOP9_32 : NOP9_16;
	default:
		ICED_UNREACHABLE();
	}
}

Result<std::vector<std::uint8_t>> CodeAssembler::assemble(std::uint64_t ip) {
	Result<CodeAssemblerResult> result = assemble_options(ip, BlockEncoderOptions::NONE);
	if (result.is_err())
		return result.error();
	return std::move(std::move(result).value().inner.code_buffer);
}

Result<CodeAssemblerResult> CodeAssembler::assemble_options(std::uint64_t ip, std::uint32_t options) {
	if (error_)
		return *error_;
	if (prefix_flags_ != 0)
		return IcedError("Unused prefixes. Did you forget to add an instruction?");
	if (!current_label_.is_empty())
		return IcedError("Unused label. Did you forget to add an instruction?");
	if (defined_anon_label_)
		return IcedError("Unused anonymous label. Did you forget to add an instruction?");
	if (!next_anon_label_.is_empty())
		return IcedError("Unused anonymous fwd() label. Did you forget to call anonymous_label()?");

	Result<BlockEncoderResult> result = BlockEncoder::encode(bitness_, InstructionBlock(instructions_, ip), options);
	if (result.is_err())
		return result.error();
	return CodeAssemblerResult{std::move(std::move(result).value())};
}

CodeAssembler& CodeAssembler::call_far(std::uint16_t selector, std::uint32_t offset) {
	const Code code = bitness_ >= 32 ? Code::Call_ptr1632 : Code::Call_ptr1616;
	return add_instr(Instruction::with_far_branch(code, selector, offset));
}

CodeAssembler& CodeAssembler::jmp_far(std::uint16_t selector, std::uint32_t offset) {
	const Code code = bitness_ >= 32 ? Code::Jmp_ptr1632 : Code::Jmp_ptr1616;
	return add_instr(Instruction::with_far_branch(code, selector, offset));
}

CodeAssembler& CodeAssembler::xlatb() {
	if (error_)
		return *this;
	Register base;
	switch (bitness_) {
	case 64:
		base = Register::RBX;
		break;
	case 32:
		base = Register::EBX;
		break;
	case 16:
		base = Register::BX;
		break;
	default:
		ICED_UNREACHABLE();
	}
	return add_instr(Instruction::with1(Code::Xlat_m8, MemoryOperand::with_base_index(base, Register::AL)));
}

} // namespace iced_x86::code_asm
