// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/gas/info.rs + formatter/gas/fmt_tbl.rs

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/rounding_control.hpp"
#include "internal/formatter/fmt_common.hpp"
#include "internal/formatter/fmt_utils.hpp"
#include "internal/formatter/gas/fmt_data.hpp"
#include "internal/formatter/gas/info.hpp"
#include "internal/formatter/gas/instr_op_info_flags.hpp"
#include "internal/formatter/gas/mem_size_tbl.hpp"
#include "internal/formatter/pseudo_ops.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal::gas {

static_assert(IcedConstants::MAX_OP_COUNT == 5, "");

InstrOpInfo::InstrOpInfo(FormatterString mnemonic_, const Instruction& instruction, std::uint32_t flags_) noexcept
	: InstrOpInfo(mnemonic_) {
	flags = static_cast<std::uint16_t>(flags_);
	const std::uint32_t instr_op_count = instruction.op_count();
	op_count = static_cast<std::uint8_t>(instr_op_count);
	if ((flags_ & InstrOpInfoFlags::KEEP_OPERAND_ORDER) != 0) {
		op_kinds[0] = to_instr_op_kind(instruction.op0_kind());
		op_kinds[1] = to_instr_op_kind(instruction.op1_kind());
		op_kinds[2] = to_instr_op_kind(instruction.op2_kind());
		op_kinds[3] = to_instr_op_kind(instruction.op3_kind());
		op_kinds[4] = to_instr_op_kind(instruction.op4_kind());
		op_registers[0] = instruction.op0_register();
		op_registers[1] = instruction.op1_register();
		op_registers[2] = instruction.op2_register();
		op_registers[3] = instruction.op3_register();
		op_registers[4] = instruction.op4_register();
	} else {
		switch (instr_op_count) {
		case 0:
			break;

		case 1:
			op_kinds[0] = to_instr_op_kind(instruction.op0_kind());
			op_registers[0] = instruction.op0_register();
			break;

		case 2:
			op_kinds[0] = to_instr_op_kind(instruction.op1_kind());
			op_kinds[1] = to_instr_op_kind(instruction.op0_kind());
			op_registers[0] = instruction.op1_register();
			op_registers[1] = instruction.op0_register();
			break;

		case 3:
			op_kinds[0] = to_instr_op_kind(instruction.op2_kind());
			op_kinds[1] = to_instr_op_kind(instruction.op1_kind());
			op_kinds[2] = to_instr_op_kind(instruction.op0_kind());
			op_registers[0] = instruction.op2_register();
			op_registers[1] = instruction.op1_register();
			op_registers[2] = instruction.op0_register();
			break;

		case 4:
			op_kinds[0] = to_instr_op_kind(instruction.op3_kind());
			op_kinds[1] = to_instr_op_kind(instruction.op2_kind());
			op_kinds[2] = to_instr_op_kind(instruction.op1_kind());
			op_kinds[3] = to_instr_op_kind(instruction.op0_kind());
			op_registers[0] = instruction.op3_register();
			op_registers[1] = instruction.op2_register();
			op_registers[2] = instruction.op1_register();
			op_registers[3] = instruction.op0_register();
			break;

		case 5:
			op_kinds[0] = to_instr_op_kind(instruction.op4_kind());
			op_kinds[1] = to_instr_op_kind(instruction.op3_kind());
			op_kinds[2] = to_instr_op_kind(instruction.op2_kind());
			op_kinds[3] = to_instr_op_kind(instruction.op1_kind());
			op_kinds[4] = to_instr_op_kind(instruction.op0_kind());
			op_registers[0] = instruction.op4_register();
			op_registers[1] = instruction.op3_register();
			op_registers[2] = instruction.op2_register();
			op_registers[3] = instruction.op1_register();
			op_registers[4] = instruction.op0_register();
			break;

		default:
			ICED_UNREACHABLE();
		}
	}
	switch (op_count) {
	case 0:
		op_indexes[0] = OP_ACCESS_INVALID;
		op_indexes[1] = OP_ACCESS_INVALID;
		op_indexes[2] = OP_ACCESS_INVALID;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 1:
		op_indexes[1] = OP_ACCESS_INVALID;
		op_indexes[2] = OP_ACCESS_INVALID;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 2:
		op_indexes[0] = 1;
		op_indexes[2] = OP_ACCESS_INVALID;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 3:
		op_indexes[0] = 2;
		op_indexes[1] = 1;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 4:
		op_indexes[0] = 3;
		op_indexes[1] = 2;
		op_indexes[2] = 1;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 5:
		op_indexes[0] = 4;
		op_indexes[1] = 3;
		op_indexes[2] = 2;
		op_indexes[3] = 1;
		break;

	default:
		ICED_UNREACHABLE();
	}
}

namespace {

constexpr FormatterStringData<sizeof("xchg")> STR_XCHG("xchg");
constexpr FormatterStringData<sizeof("xchgw")> STR_XCHGW("xchgw");
constexpr FormatterStringData<sizeof("xchgl")> STR_XCHGL("xchgl");
constexpr FormatterStringData<sizeof("xchgq")> STR_XCHGQ("xchgq");

std::uint32_t get_bitness(CodeSize code_size) noexcept {
	static constexpr std::uint32_t CODESIZE_TO_BITNESS[4] = {0, 16, 32, 64};
	static_assert(static_cast<std::uint32_t>(CodeSize::Unknown) == 0, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code16) == 1, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code32) == 2, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code64) == 3, "");
	return CODESIZE_TO_BITNESS[static_cast<std::size_t>(code_size) & 3];
}

FormatterString str(std::uint32_t offset) noexcept { return FormatterString(STRINGS + offset); }

FormatterString get_mnemonic(const FormatterOptions& options, const Instruction& instruction, FormatterString mnemonic,
							 FormatterString mnemonic_suffix, std::uint32_t flags) noexcept {
	if (options.gas_show_mnemonic_size_suffix())
		return mnemonic_suffix;
	if ((flags & InstrOpInfoFlags::MNEMONIC_SUFFIX_IF_MEM) != 0 &&
		get_mem_size_tbl()[static_cast<std::size_t>(instruction.memory_size())]->is_default()) {
		if (instruction.op0_kind() == OpKind::Memory || instruction.op1_kind() == OpKind::Memory || instruction.op2_kind() == OpKind::Memory)
			return mnemonic_suffix;
	}
	return mnemonic;
}

class SimpleInstrInfo final {
public:
	explicit SimpleInstrInfo(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, flags_), instruction, flags_);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_cc final {
public:
	explicit SimpleInstrInfo_cc(const InstrInfo& e) noexcept : mnemonics_(ARGS + e.arg1), cc_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		const std::size_t count = get_cc_mnemonics_count(cc_index_);
		const std::size_t index = get_mnemonic_cc_index(options, cc_index_, count);
		const FormatterString mnemonic = str(mnemonics_[index]);
		const FormatterString mnemonic_suffix = str(mnemonics_[count + index]);
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic, mnemonic_suffix, FLAGS), instruction, FLAGS);
	}

private:
	// The mnemonics followed by the mnemonics with a suffix (offsets in STRINGS)
	const std::uint16_t* mnemonics_;
	std::uint32_t cc_index_;
};

class SimpleInstrInfo_AamAad final {
public:
	explicit SimpleInstrInfo_AamAad(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		if (instruction.immediate8() == 10)
			return InstrOpInfo(mnemonic_);
		return InstrOpInfo(mnemonic_, instruction, InstrOpInfoFlags::NONE);
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_nop final {
public:
	explicit SimpleInstrInfo_nop(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), register_(static_cast<Register>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness == 0 || (instr_bitness & bitness_) != 0)
			return InstrOpInfo(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		FormatterString mnemonic;
		if (!options.gas_show_mnemonic_size_suffix())
			mnemonic = FormatterString(STR_XCHG);
		else if (register_ == Register::AX)
			mnemonic = FormatterString(STR_XCHGW);
		else if (register_ == Register::EAX)
			mnemonic = FormatterString(STR_XCHGL);
		else if (register_ == Register::RAX)
			mnemonic = FormatterString(STR_XCHGQ);
		else
			ICED_UNREACHABLE();
		InstrOpInfo info(mnemonic);
		info.op_count = 2;
		static_assert(static_cast<std::uint32_t>(InstrOpKind::Register) == 0, "");
		// info.op_kinds[0] = InstrOpKind::Register;
		// info.op_kinds[1] = InstrOpKind::Register;
		info.op_registers[0] = register_;
		info.op_registers[1] = register_;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_NONE;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_NONE;
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	Register register_;
};

class SimpleInstrInfo_STIG1 final {
public:
	explicit SimpleInstrInfo_STIG1(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(mnemonic_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		ICED_DEBUG_ASSERT(instruction.op0_kind() == OpKind::Register && instruction.op0_register() == Register::ST0);
		if (!pseudo_op_ || !(options.use_pseudo_ops() && instruction.op1_register() == Register::ST1)) {
			info.op_count = 1;
			static_assert(static_cast<std::uint32_t>(InstrOpKind::Register) == 0, "");
			// info.op_kinds[0] = InstrOpKind::Register;
			info.op_registers[0] = instruction.op1_register();
			info.op_indexes[0] = 1;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	bool pseudo_op_;
};

class SimpleInstrInfo_STi_ST final {
public:
	explicit SimpleInstrInfo_STi_ST(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		if (pseudo_op_ && options.use_pseudo_ops() && (instruction.op0_register() == Register::ST1 || instruction.op1_register() == Register::ST1))
			return InstrOpInfo(mnemonic_);
		InstrOpInfo info(mnemonic_, instruction, FLAGS);
		ICED_DEBUG_ASSERT(info.op_registers[0] == Register::ST0);
		info.op_registers[0] = REGISTER_ST;
		return info;
	}

private:
	FormatterString mnemonic_;
	bool pseudo_op_;
};

class SimpleInstrInfo_ST_STi final {
public:
	explicit SimpleInstrInfo_ST_STi(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		ICED_DEBUG_ASSERT(info.op_registers[1] == Register::ST0);
		info.op_registers[1] = REGISTER_ST;
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_as final {
public:
	explicit SimpleInstrInfo_as(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = 0;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else
				flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_maskmovq final {
public:
	explicit SimpleInstrInfo_maskmovq(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		ICED_DEBUG_ASSERT(instruction.op_count() == 3);

		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());

		std::uint32_t bitness;
		switch (instruction.op0_kind()) {
		case OpKind::MemorySegDI:
			bitness = 16;
			break;
		case OpKind::MemorySegEDI:
			bitness = 32;
			break;
		case OpKind::MemorySegRDI:
			bitness = 64;
			break;
		default:
			bitness = instr_bitness;
			break;
		}

		InstrOpInfo info(mnemonic_);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op2_kind());
		info.op_registers[0] = instruction.op2_register();
		info.op_indexes[0] = 2;
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_registers[1] = instruction.op1_register();
		info.op_indexes[1] = 1;
		if (instr_bitness != 0 && instr_bitness != bitness) {
			if (bitness == 16)
				info.flags |= static_cast<std::uint16_t>(InstrOpInfoFlags::ADDR_SIZE16);
			else if (bitness == 32)
				info.flags |= static_cast<std::uint16_t>(InstrOpInfoFlags::ADDR_SIZE32);
			else
				info.flags |= static_cast<std::uint16_t>(InstrOpInfoFlags::ADDR_SIZE64);
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_pblendvb final {
public:
	explicit SimpleInstrInfo_pblendvb(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		info.op_count = 3;
		static_assert(static_cast<std::uint32_t>(InstrOpKind::Register) == 0, "");
		// info.op_kinds[0] = InstrOpKind::Register;
		info.op_registers[0] = Register::XMM0;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[1] = 1;
		info.op_registers[1] = instruction.op1_register();
		info.op_kinds[2] = InstrOpInfo::to_instr_op_kind(instruction.op0_kind());
		info.op_registers[2] = instruction.op0_register();
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_OpSize final {
public:
	explicit SimpleInstrInfo_OpSize(const InstrInfo& e) noexcept
		: mnemonics_{str(e.mnemonic), str(ARGS[e.arg1]), str(ARGS[e.arg1 + 1U]), str(ARGS[e.arg1 + 2U])}
		, code_size_(static_cast<CodeSize>(e.arg3)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const FormatterString* mnemonic;
		if (instruction.code_size() == code_size_ && !options.gas_show_mnemonic_size_suffix())
			mnemonic = &mnemonics_[static_cast<std::size_t>(CodeSize::Unknown)];
		else
			mnemonic = &mnemonics_[static_cast<std::size_t>(code_size_)];
		return InstrOpInfo(*mnemonic, instruction, InstrOpInfoFlags::NONE);
	}

private:
	std::array<FormatterString, 4> mnemonics_;
	CodeSize code_size_;
};

class SimpleInstrInfo_OpSize2_bnd final {
public:
	explicit SimpleInstrInfo_OpSize2_bnd(const InstrInfo& e) noexcept
		: mnemonics_{str(e.mnemonic), str(ARGS[e.arg1]), str(ARGS[e.arg1 + 1U]), str(ARGS[e.arg1 + 2U])} {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString* mnemonic;
		if (options.gas_show_mnemonic_size_suffix())
			mnemonic = &mnemonics_[static_cast<std::size_t>(CodeSize::Code64)];
		else
			mnemonic = &mnemonics_[static_cast<std::size_t>(instruction.code_size())];
		return InstrOpInfo(*mnemonic, instruction, flags);
	}

private:
	std::array<FormatterString, 4> mnemonics_;
};

class SimpleInstrInfo_OpSize3 final {
public:
	explicit SimpleInstrInfo_OpSize3(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const FormatterString& mnemonic =
			!options.gas_show_mnemonic_size_suffix() && (instr_bitness == 0 || (instr_bitness & bitness_) != 0) ? mnemonic_ : mnemonic_suffix_;
		return InstrOpInfo(mnemonic, instruction, InstrOpInfoFlags::NONE);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_os2 final {
public:
	explicit SimpleInstrInfo_os2(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, mnemonic_suffix_(str(ARGS[e.arg1]))
		, bitness_(e.arg3)
		, flags_(ARGS[e.arg1 + 2U])
		, can_use_bnd_(ARGS[e.arg1 + 1U] != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = flags_;
		if (can_use_bnd_ && instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const FormatterString& mnemonic = instr_bitness != 0 && instr_bitness != bitness_
											  ? mnemonic_suffix_
											  : get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, flags);
		return InstrOpInfo(mnemonic, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t bitness_;
	std::uint32_t flags_;
	bool can_use_bnd_;
};

class SimpleInstrInfo_os final {
public:
	explicit SimpleInstrInfo_os(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), flags_(e.arg2), can_use_bnd_(e.arg1 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = flags_;
		if (can_use_bnd_ && instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic_, mnemonic_, flags), instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	std::uint32_t flags_;
	bool can_use_bnd_;
};

class SimpleInstrInfo_os_mem2 final {
public:
	explicit SimpleInstrInfo_os_mem2(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const FormatterString& mnemonic = instr_bitness != 0 && (instr_bitness & bitness_) == 0
											  ? mnemonic_suffix_
											  : get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, FLAGS);
		return InstrOpInfo(mnemonic, instruction, InstrOpInfoFlags::NONE);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_Reg16 final {
public:
	explicit SimpleInstrInfo_Reg16(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		InstrOpInfo info(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, FLAGS), instruction, FLAGS);
		info.op_registers[0] = r_to_r16(info.op_registers[0]);
		info.op_registers[1] = r_to_r16(info.op_registers[1]);
		info.op_registers[2] = r_to_r16(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
};

class SimpleInstrInfo_mem16 final {
public:
	explicit SimpleInstrInfo_mem16(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic_reg_suffix_(str(e.arg1)), mnemonic_mem_suffix_(str(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		const FormatterString& mnemonic_suffix =
			instruction.op0_kind() == OpKind::Memory || instruction.op1_kind() == OpKind::Memory ? mnemonic_mem_suffix_ : mnemonic_reg_suffix_;
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix, FLAGS), instruction, FLAGS);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_reg_suffix_;
	FormatterString mnemonic_mem_suffix_;
};

constexpr std::uint32_t NO_CC_INDEX = 0xFFFF'FFFF;
// `InstrInfo::arg3` value if there's no cc index
constexpr std::uint8_t NO_CC_INDEX_U8 = 0xFF;

class SimpleInstrInfo_os_loop final {
public:
	explicit SimpleInstrInfo_os_loop(const InstrInfo& e) noexcept
		: args_(ARGS + e.arg1), bitness_(e.arg2), cc_index_(e.arg3 == NO_CC_INDEX_U8 ? NO_CC_INDEX : e.arg3), reg_size_(args_[0]) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const std::size_t count = cc_index_ == NO_CC_INDEX ? 1 : get_cc_mnemonics_count(cc_index_);
		const std::uint16_t* mnemonics =
			(instr_bitness != 0 && instr_bitness != reg_size_) || options.gas_show_mnemonic_size_suffix() ? args_ + 1 + count : args_ + 1;
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16 | InstrOpInfoFlags::OP_SIZE_IS_BYTE_DIRECTIVE;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32 | InstrOpInfoFlags::OP_SIZE_IS_BYTE_DIRECTIVE;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		const std::size_t index = cc_index_ == NO_CC_INDEX ? 0 : get_mnemonic_cc_index(options, cc_index_, count);
		const FormatterString mnemonic = str(mnemonics[index]);
		return InstrOpInfo(mnemonic, instruction, flags);
	}

private:
	// reg_size, the mnemonics, the mnemonics with a suffix (offsets in STRINGS)
	const std::uint16_t* args_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
	std::uint32_t reg_size_;
};

class SimpleInstrInfo_os_jcc final {
public:
	explicit SimpleInstrInfo_os_jcc(const InstrInfo& e) noexcept : mnemonics_(ARGS + e.arg1), bitness_(e.arg2), cc_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		const Register prefix_seg = instruction.segment_prefix();
		if (prefix_seg == Register::CS)
			flags |= InstrOpInfoFlags::JCC_NOT_TAKEN;
		else if (prefix_seg == Register::DS)
			flags |= InstrOpInfoFlags::JCC_TAKEN;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString mnemonic = str(mnemonics_[get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_))]);
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic, mnemonic, flags), instruction, flags);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
};

class SimpleInstrInfo_movabs final {
public:
	explicit SimpleInstrInfo_movabs(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, mnemonic_suffix_(str(ARGS[e.arg1]))
		, mnemonic64_(str(ARGS[e.arg1 + 1U]))
		, mnemonic_suffix64_(str(ARGS[e.arg1 + 2U])) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		std::uint32_t mem_size;
		const FormatterString* mnemonic;
		const FormatterString* mnemonic_suffix;
		switch (instruction.memory_displ_size()) {
		case 2:
			mem_size = 16;
			mnemonic = &mnemonic_;
			mnemonic_suffix = &mnemonic_suffix_;
			break;
		case 4:
			mem_size = 32;
			mnemonic = &mnemonic_;
			mnemonic_suffix = &mnemonic_suffix_;
			break;
		default:
			mem_size = 64;
			mnemonic = &mnemonic64_;
			mnemonic_suffix = &mnemonic_suffix64_;
			break;
		}
		if (instr_bitness == 0)
			instr_bitness = mem_size;
		if (instr_bitness == 64) {
			if (mem_size == 32)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
		} else if (instr_bitness != mem_size) {
			ICED_DEBUG_ASSERT(mem_size == 16 || mem_size == 32);
			if (mem_size == 16)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
		}
		return InstrOpInfo(get_mnemonic(options, instruction, *mnemonic, *mnemonic_suffix, flags), instruction, flags);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	FormatterString mnemonic64_;
	FormatterString mnemonic_suffix64_;
};

void move_operands(InstrOpInfo& info, std::uint32_t index, InstrOpKind new_op_kind) noexcept {
	ICED_DEBUG_ASSERT(info.op_count <= 4);

	switch (index) {
	case 0:
		info.op_kinds[4] = info.op_kinds[3];
		info.op_registers[4] = info.op_registers[3];
		info.op_kinds[3] = info.op_kinds[2];
		info.op_registers[3] = info.op_registers[2];
		info.op_kinds[2] = info.op_kinds[1];
		info.op_registers[2] = info.op_registers[1];
		info.op_kinds[1] = info.op_kinds[0];
		info.op_registers[1] = info.op_registers[0];
		info.op_kinds[0] = new_op_kind;
		info.op_indexes[4] = info.op_indexes[3];
		info.op_indexes[3] = info.op_indexes[2];
		info.op_indexes[2] = info.op_indexes[1];
		info.op_indexes[1] = info.op_indexes[0];
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_NONE;
		info.op_count++;
		break;

	case 1:
		info.op_kinds[4] = info.op_kinds[3];
		info.op_registers[4] = info.op_registers[3];
		info.op_kinds[3] = info.op_kinds[2];
		info.op_registers[3] = info.op_registers[2];
		info.op_kinds[2] = info.op_kinds[1];
		info.op_registers[2] = info.op_registers[1];
		info.op_kinds[1] = new_op_kind;
		info.op_indexes[4] = info.op_indexes[3];
		info.op_indexes[3] = info.op_indexes[2];
		info.op_indexes[2] = info.op_indexes[1];
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_NONE;
		info.op_count++;
		break;

	default:
		ICED_UNREACHABLE();
	}
}

class SimpleInstrInfo_er final {
public:
	explicit SimpleInstrInfo_er(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), er_index_(e.arg3), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, flags_), instruction, flags_);
		if (IcedConstants::is_mvex(instruction.code())) {
			const RoundingControl rc = instruction.rounding_control();
			if (rc != RoundingControl::None) {
				InstrOpKind rc_op_kind;
				if (instruction.suppress_all_exceptions()) {
					switch (rc) {
					case RoundingControl::RoundToNearest:
						rc_op_kind = InstrOpKind::RnSae;
						break;
					case RoundingControl::RoundDown:
						rc_op_kind = InstrOpKind::RdSae;
						break;
					case RoundingControl::RoundUp:
						rc_op_kind = InstrOpKind::RuSae;
						break;
					case RoundingControl::RoundTowardZero:
						rc_op_kind = InstrOpKind::RzSae;
						break;
					default:
						return info;
					}
				} else {
					switch (rc) {
					case RoundingControl::RoundToNearest:
						rc_op_kind = InstrOpKind::Rn;
						break;
					case RoundingControl::RoundDown:
						rc_op_kind = InstrOpKind::Rd;
						break;
					case RoundingControl::RoundUp:
						rc_op_kind = InstrOpKind::Ru;
						break;
					case RoundingControl::RoundTowardZero:
						rc_op_kind = InstrOpKind::Rz;
						break;
					default:
						return info;
					}
				}
				move_operands(info, er_index_, rc_op_kind);
			} else if (instruction.suppress_all_exceptions())
				move_operands(info, er_index_, InstrOpKind::Sae);
		} else {
			const RoundingControl rc = instruction.rounding_control();
			if (rc != RoundingControl::None && can_show_rounding_control(instruction, options)) {
				InstrOpKind rc_op_kind;
				switch (rc) {
				case RoundingControl::RoundToNearest:
					rc_op_kind = InstrOpKind::RnSae;
					break;
				case RoundingControl::RoundDown:
					rc_op_kind = InstrOpKind::RdSae;
					break;
				case RoundingControl::RoundUp:
					rc_op_kind = InstrOpKind::RuSae;
					break;
				case RoundingControl::RoundTowardZero:
					rc_op_kind = InstrOpKind::RzSae;
					break;
				default:
					return info;
				}
				move_operands(info, er_index_, rc_op_kind);
			}
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t er_index_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_sae final {
public:
	explicit SimpleInstrInfo_sae(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), sae_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (instruction.suppress_all_exceptions())
			move_operands(info, sae_index_, InstrOpKind::Sae);
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t sae_index_;
};

class SimpleInstrInfo_far final {
public:
	explicit SimpleInstrInfo_far(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::INDIRECT_OPERAND;
		std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness == 0)
			instr_bitness = bitness_;
		const FormatterString* mnemonic;
		if (bitness_ == 64) {
			flags |= InstrOpInfoFlags::OP_SIZE64;
			ICED_DEBUG_ASSERT(mnemonic_.lower() == mnemonic_suffix_.lower());
			mnemonic = &mnemonic_;
		} else {
			if (bitness_ != instr_bitness || options.gas_show_mnemonic_size_suffix())
				mnemonic = &mnemonic_suffix_;
			else
				mnemonic = &mnemonic_;
		}
		return InstrOpInfo(*mnemonic, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_bnd final {
public:
	explicit SimpleInstrInfo_bnd(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = flags_;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		return InstrOpInfo(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, flags), instruction, flags);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
	std::uint32_t flags_;
};

void remove_first_imm8_operand(InstrOpInfo& info) noexcept {
	ICED_DEBUG_ASSERT(info.op_kinds[0] == InstrOpKind::Immediate8);
	info.op_count--;
	switch (info.op_count) {
	case 0:
		info.op_indexes[0] = OP_ACCESS_INVALID;
		break;

	case 1:
		info.op_kinds[0] = info.op_kinds[1];
		info.op_registers[0] = info.op_registers[1];
		info.op_indexes[0] = info.op_indexes[1];
		info.op_indexes[1] = OP_ACCESS_INVALID;
		break;

	case 2:
		info.op_kinds[0] = info.op_kinds[1];
		info.op_registers[0] = info.op_registers[1];
		info.op_kinds[1] = info.op_kinds[2];
		info.op_registers[1] = info.op_registers[2];
		info.op_indexes[0] = info.op_indexes[1];
		info.op_indexes[1] = info.op_indexes[2];
		info.op_indexes[2] = OP_ACCESS_INVALID;
		break;

	case 3:
		info.op_kinds[0] = info.op_kinds[1];
		info.op_registers[0] = info.op_registers[1];
		info.op_kinds[1] = info.op_kinds[2];
		info.op_registers[1] = info.op_registers[2];
		info.op_kinds[2] = info.op_kinds[3];
		info.op_registers[2] = info.op_registers[3];
		info.op_indexes[0] = info.op_indexes[1];
		info.op_indexes[1] = info.op_indexes[2];
		info.op_indexes[2] = info.op_indexes[3];
		info.op_indexes[3] = OP_ACCESS_INVALID;
		break;

	case 4:
		info.op_kinds[0] = info.op_kinds[1];
		info.op_registers[0] = info.op_registers[1];
		info.op_kinds[1] = info.op_kinds[2];
		info.op_registers[1] = info.op_registers[2];
		info.op_kinds[2] = info.op_kinds[3];
		info.op_registers[2] = info.op_registers[3];
		info.op_kinds[3] = info.op_kinds[4];
		info.op_registers[3] = info.op_registers[4];
		info.op_indexes[0] = info.op_indexes[1];
		info.op_indexes[1] = info.op_indexes[2];
		info.op_indexes[2] = info.op_indexes[3];
		info.op_indexes[3] = info.op_indexes[4];
		info.op_indexes[4] = OP_ACCESS_INVALID;
		break;

	default:
		ICED_UNREACHABLE();
	}
}

class SimpleInstrInfo_pops final {
public:
	explicit SimpleInstrInfo_pops(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), pseudo_ops_(get_pseudo_ops(static_cast<PseudoOpsKind>(e.arg3))), can_use_sae_(e.arg1 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (can_use_sae_ && instruction.suppress_all_exceptions())
			move_operands(info, 1, InstrOpKind::Sae);
		const std::size_t imm = instruction.immediate8();
		if (options.use_pseudo_ops() && imm < pseudo_ops_.size()) {
			remove_first_imm8_operand(info);
			info.mnemonic = pseudo_ops_[imm];
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	PseudoOps pseudo_ops_;
	bool can_use_sae_;
};

class SimpleInstrInfo_pclmulqdq final {
public:
	explicit SimpleInstrInfo_pclmulqdq(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), pseudo_ops_(get_pseudo_ops(static_cast<PseudoOpsKind>(e.arg3))) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (options.use_pseudo_ops()) {
			std::int32_t index;
			switch (instruction.immediate8()) {
			case 0:
				index = 0;
				break;
			case 1:
				index = 1;
				break;
			case 0x10:
				index = 2;
				break;
			case 0x11:
				index = 3;
				break;
			default:
				index = -1;
				break;
			}
			if (index >= 0) {
				remove_first_imm8_operand(info);
				info.mnemonic = pseudo_ops_[static_cast<std::size_t>(index)];
			}
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	PseudoOps pseudo_ops_;
};

class SimpleInstrInfo_imul final {
public:
	explicit SimpleInstrInfo_imul(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), mnemonic_suffix_(str(e.arg1)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		InstrOpInfo info(get_mnemonic(options, instruction, mnemonic_, mnemonic_suffix_, FLAGS), instruction, FLAGS);
		ICED_DEBUG_ASSERT(info.op_count == 3);
		if (options.use_pseudo_ops() && info.op_kinds[1] == InstrOpKind::Register && info.op_kinds[2] == InstrOpKind::Register &&
			info.op_registers[1] == info.op_registers[2]) {
			info.op_count--;
			info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ_WRITE;
			info.op_indexes[2] = OP_ACCESS_INVALID;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic_suffix_;
};

class SimpleInstrInfo_Reg32 final {
public:
	explicit SimpleInstrInfo_Reg32(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		InstrOpInfo info(mnemonic_, instruction, FLAGS);
		info.op_registers[0] = r64_to_r32(info.op_registers[0]);
		info.op_registers[1] = r64_to_r32(info.op_registers[1]);
		info.op_registers[2] = r64_to_r32(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_DeclareData final {
public:
	explicit SimpleInstrInfo_DeclareData(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const InstrOpKind op_kind_ = get_op_kind(instruction.code());
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::KEEP_OPERAND_ORDER | InstrOpInfoFlags::MNEMONIC_IS_DIRECTIVE);
		info.op_count = static_cast<std::uint8_t>(instruction.declare_data_len());
		info.op_kinds[0] = op_kind_;
		info.op_kinds[1] = op_kind_;
		info.op_kinds[2] = op_kind_;
		info.op_kinds[3] = op_kind_;
		info.op_kinds[4] = op_kind_;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[3] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[4] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	static InstrOpKind get_op_kind(Code code) noexcept {
		switch (code) {
		case Code::DeclareByte:
			return InstrOpKind::DeclareByte;
		case Code::DeclareWord:
			return InstrOpKind::DeclareWord;
		case Code::DeclareDword:
			return InstrOpKind::DeclareDword;
		case Code::DeclareQword:
			return InstrOpKind::DeclareQword;
		default:
			ICED_UNREACHABLE();
		}
	}

	FormatterString mnemonic_;
};

} // namespace

InstrOpInfo get_op_info(const FormatterOptions& options, const Instruction& instruction) noexcept {
	const InstrInfo& info = INSTR_INFOS[static_cast<std::size_t>(instruction.code())];
	switch (info.kind) {
	case InstrInfoKind::Simple:
		return SimpleInstrInfo(info).op_info(options, instruction);
	case InstrInfoKind::cc:
		return SimpleInstrInfo_cc(info).op_info(options, instruction);
	case InstrInfoKind::AamAad:
		return SimpleInstrInfo_AamAad(info).op_info(options, instruction);
	case InstrInfoKind::nop:
		return SimpleInstrInfo_nop(info).op_info(options, instruction);
	case InstrInfoKind::STIG1:
		return SimpleInstrInfo_STIG1(info).op_info(options, instruction);
	case InstrInfoKind::STi_ST:
		return SimpleInstrInfo_STi_ST(info).op_info(options, instruction);
	case InstrInfoKind::ST_STi:
		return SimpleInstrInfo_ST_STi(info).op_info(options, instruction);
	case InstrInfoKind::as:
		return SimpleInstrInfo_as(info).op_info(options, instruction);
	case InstrInfoKind::maskmovq:
		return SimpleInstrInfo_maskmovq(info).op_info(options, instruction);
	case InstrInfoKind::pblendvb:
		return SimpleInstrInfo_pblendvb(info).op_info(options, instruction);
	case InstrInfoKind::OpSize:
		return SimpleInstrInfo_OpSize(info).op_info(options, instruction);
	case InstrInfoKind::OpSize2_bnd:
		return SimpleInstrInfo_OpSize2_bnd(info).op_info(options, instruction);
	case InstrInfoKind::OpSize3:
		return SimpleInstrInfo_OpSize3(info).op_info(options, instruction);
	case InstrInfoKind::os2:
		return SimpleInstrInfo_os2(info).op_info(options, instruction);
	case InstrInfoKind::os:
		return SimpleInstrInfo_os(info).op_info(options, instruction);
	case InstrInfoKind::os_mem2:
		return SimpleInstrInfo_os_mem2(info).op_info(options, instruction);
	case InstrInfoKind::Reg16:
		return SimpleInstrInfo_Reg16(info).op_info(options, instruction);
	case InstrInfoKind::mem16:
		return SimpleInstrInfo_mem16(info).op_info(options, instruction);
	case InstrInfoKind::os_loop:
		return SimpleInstrInfo_os_loop(info).op_info(options, instruction);
	case InstrInfoKind::os_jcc:
		return SimpleInstrInfo_os_jcc(info).op_info(options, instruction);
	case InstrInfoKind::movabs:
		return SimpleInstrInfo_movabs(info).op_info(options, instruction);
	case InstrInfoKind::er:
		return SimpleInstrInfo_er(info).op_info(options, instruction);
	case InstrInfoKind::sae:
		return SimpleInstrInfo_sae(info).op_info(options, instruction);
	case InstrInfoKind::far:
		return SimpleInstrInfo_far(info).op_info(options, instruction);
	case InstrInfoKind::bnd:
		return SimpleInstrInfo_bnd(info).op_info(options, instruction);
	case InstrInfoKind::pops:
		return SimpleInstrInfo_pops(info).op_info(options, instruction);
	case InstrInfoKind::pclmulqdq:
		return SimpleInstrInfo_pclmulqdq(info).op_info(options, instruction);
	case InstrInfoKind::imul:
		return SimpleInstrInfo_imul(info).op_info(options, instruction);
	case InstrInfoKind::Reg32:
		return SimpleInstrInfo_Reg32(info).op_info(options, instruction);
	case InstrInfoKind::DeclareData:
		return SimpleInstrInfo_DeclareData(info).op_info(options, instruction);
	}
	ICED_UNREACHABLE();
}

} // namespace iced_x86::internal::gas
