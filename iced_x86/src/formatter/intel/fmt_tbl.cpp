// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/intel/info.rs + formatter/intel/fmt_tbl.rs

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/iced_constants.hpp"
#include "internal/formatter/fmt_common.hpp"
#include "internal/formatter/fmt_utils.hpp"
#include "internal/formatter/intel/fmt_data.hpp"
#include "internal/formatter/intel/info.hpp"
#include "internal/formatter/intel/instr_op_info_flags.hpp"
#include "internal/formatter/intel/mem_size_tbl.hpp"
#include "internal/formatter/pseudo_ops.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal::intel {

static_assert(IcedConstants::MAX_OP_COUNT == 5, "");

InstrOpInfo::InstrOpInfo(FormatterString mnemonic_, const Instruction& instruction, std::uint32_t flags_) noexcept
	: InstrOpInfo(mnemonic_) {
	flags = static_cast<std::uint16_t>(flags_);
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
	const std::uint32_t instr_op_count = instruction.op_count();
	op_count = static_cast<std::uint8_t>(instr_op_count);
	switch (instr_op_count) {
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
		op_indexes[1] = 1;
		op_indexes[2] = OP_ACCESS_INVALID;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 3:
		op_indexes[1] = 1;
		op_indexes[2] = 2;
		op_indexes[3] = OP_ACCESS_INVALID;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 4:
		op_indexes[1] = 1;
		op_indexes[2] = 2;
		op_indexes[3] = 3;
		op_indexes[4] = OP_ACCESS_INVALID;
		break;

	case 5:
		op_indexes[1] = 1;
		op_indexes[2] = 2;
		op_indexes[3] = 3;
		op_indexes[4] = 4;
		break;

	default:
		ICED_UNREACHABLE();
	}
}

namespace {

constexpr FormatterStringData<sizeof("xchg")> STR_XCHG("xchg");

FormatterString str(std::uint32_t offset) noexcept { return FormatterString(STRINGS + offset); }

std::uint32_t get_bitness(CodeSize code_size) noexcept {
	static constexpr std::uint32_t CODESIZE_TO_BITNESS[4] = {0, 16, 32, 64};
	static_assert(static_cast<std::uint32_t>(CodeSize::Unknown) == 0, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code16) == 1, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code32) == 2, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code64) == 3, "");
	return CODESIZE_TO_BITNESS[static_cast<std::size_t>(code_size) & 3];
}

class SimpleInstrInfo final {
public:
	explicit SimpleInstrInfo(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		return InstrOpInfo(mnemonic_, instruction, flags_);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_cc final {
public:
	explicit SimpleInstrInfo_cc(const InstrInfo& e) noexcept : mnemonics_(ARGS + e.arg1), cc_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		const FormatterString mnemonic = str(mnemonics_[get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_))]);
		return InstrOpInfo(mnemonic, instruction, FLAGS);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t cc_index_;
};

class SimpleInstrInfo_memsize final {
public:
	explicit SimpleInstrInfo_memsize(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const std::uint32_t flags = instr_bitness == 0 || (instr_bitness & bitness_) != 0
										? InstrOpInfoFlags::MEM_SIZE_NOTHING
										: InstrOpInfoFlags::SHOW_NO_MEM_SIZE_FORCE_SIZE | InstrOpInfoFlags::SHOW_MIN_MEM_SIZE_FORCE_SIZE;
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_StringIg1 final {
public:
	explicit SimpleInstrInfo_StringIg1(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_);
		info.op_count = 1;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op0_kind());
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_StringIg0 final {
public:
	explicit SimpleInstrInfo_StringIg0(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_);
		info.op_count = 1;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[0] = info.op_indexes[1];
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_nop final {
public:
	explicit SimpleInstrInfo_nop(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), register_(static_cast<Register>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness == 0 || (instr_bitness & bitness_) != 0)
			return InstrOpInfo(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		InstrOpInfo info{FormatterString(STR_XCHG)};
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

class SimpleInstrInfo_ST1 final {
public:
	explicit SimpleInstrInfo_ST1(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, flags_(e.arg2)
		, op0_access_(e.arg3 != 0 ? InstrInfoConstants::OP_ACCESS_WRITE : InstrInfoConstants::OP_ACCESS_READ_WRITE) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_, instruction, flags_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 1);
		info.op_count = 2;
		info.op_kinds[1] = info.op_kinds[0];
		info.op_registers[1] = info.op_registers[0];
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_registers[0] = REGISTER_ST;
		info.op_indexes[1] = info.op_indexes[0];
		info.op_indexes[0] = op0_access_;
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
	std::int8_t op0_access_;
};

class SimpleInstrInfo_ST2 final {
public:
	explicit SimpleInstrInfo_ST2(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_, instruction, flags_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 1);
		info.op_count = 2;
		info.op_kinds[1] = InstrOpKind::Register;
		info.op_registers[1] = REGISTER_ST;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_maskmovq final {
public:
	explicit SimpleInstrInfo_maskmovq(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		ICED_DEBUG_ASSERT(instruction.op_count() == 3);

		const OpKind op_kind = instruction.op0_kind();
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemorySegDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemorySegEDI;
			break;
		case CodeSize::Code64:
			short_form_op_kind = OpKind::MemorySegRDI;
			break;
		case CodeSize::Unknown:
		default:
			short_form_op_kind = op_kind;
			break;
		}
		std::uint32_t flags = InstrOpInfoFlags::IGNORE_SEGMENT_PREFIX;
		if (op_kind != short_form_op_kind) {
			if (op_kind == OpKind::MemorySegDI)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (op_kind == OpKind::MemorySegEDI)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else if (op_kind == OpKind::MemorySegRDI)
				flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		InstrOpInfo info(mnemonic_);
		info.flags = static_cast<std::uint16_t>(flags);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[0] = 1;
		info.op_registers[0] = instruction.op1_register();
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op2_kind());
		info.op_indexes[1] = 2;
		info.op_registers[1] = instruction.op2_register();
		const Register seg_reg = instruction.segment_prefix();
		if (seg_reg != Register::None && show_segment_prefix(Register::None, instruction, options)) {
			info.op_count = 3;
			info.op_kinds[2] = InstrOpKind::Register;
			info.op_registers[2] = seg_reg;
			info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_READ;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_os final {
public:
	explicit SimpleInstrInfo_os(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = flags_;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32) {
				if (instr_bitness != 64)
					flags |= InstrOpInfoFlags::OP_SIZE32;
			} else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_os_bnd final {
public:
	explicit SimpleInstrInfo_os_bnd(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		if (instruction.has_repne_prefix())
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
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_as final {
public:
	explicit SimpleInstrInfo_as(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
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

class SimpleInstrInfo_os_jcc final {
public:
	explicit SimpleInstrInfo_os_jcc(const InstrInfo& e) noexcept
		: mnemonics_(ARGS + e.arg1 + 1), bitness_(e.arg2), cc_index_(e.arg3), flags_(ARGS[e.arg1]) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = flags_;
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
			flags |= InstrOpInfoFlags::IGNORE_SEGMENT_PREFIX | InstrOpInfoFlags::JCC_NOT_TAKEN;
		else if (prefix_seg == Register::DS)
			flags |= InstrOpInfoFlags::IGNORE_SEGMENT_PREFIX | InstrOpInfoFlags::JCC_TAKEN;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString mnemonic = str(mnemonics_[get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_))]);
		return InstrOpInfo(mnemonic, instruction, flags);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
	std::uint32_t flags_;
};

constexpr std::uint32_t NO_CC_INDEX = 0xFFFF'FFFF;
// `InstrInfo::arg3` value if there's no cc index
constexpr std::uint8_t NO_CC_INDEX_U8 = 0xFF;

class SimpleInstrInfo_os_loop final {
public:
	explicit SimpleInstrInfo_os_loop(const InstrInfo& e) noexcept
		: mnemonics_(ARGS + e.arg1 + 1)
		, bitness_(e.arg2)
		, cc_index_(e.arg3 == NO_CC_INDEX_U8 ? NO_CC_INDEX : e.arg3)
		, register_(static_cast<Register>(ARGS[e.arg1])) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		Register expected_reg;
		switch (instr_bitness) {
		case 0:
			expected_reg = register_;
			break;
		case 16:
			expected_reg = Register::CX;
			break;
		case 32:
			expected_reg = Register::ECX;
			break;
		case 64:
			expected_reg = Register::RCX;
			break;
		default:
			ICED_UNREACHABLE();
		}
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		if (expected_reg != register_) {
			if (register_ == Register::CX)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (register_ == Register::ECX)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else
				flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		const std::size_t index = cc_index_ == NO_CC_INDEX ? 0 : get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_));
		const FormatterString mnemonic = str(mnemonics_[index]);
		return InstrOpInfo(mnemonic, instruction, flags);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
	Register register_;
};

class SimpleInstrInfo_movabs final {
public:
	explicit SimpleInstrInfo_movabs(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		std::uint32_t mem_size;
		switch (instruction.memory_displ_size()) {
		case 2:
			mem_size = 16;
			break;
		case 4:
			mem_size = 32;
			break;
		default:
			mem_size = 64;
			break;
		}
		if (instr_bitness == 0)
			instr_bitness = mem_size;
		if (instr_bitness != mem_size) {
			if (mem_size == 16)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (mem_size == 32)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else
				flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_opmask_op final {
public:
	explicit SimpleInstrInfo_opmask_op(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		ICED_DEBUG_ASSERT(instruction.op_count() <= 2);
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		const Register kreg = instruction.op_mask();
		if (kreg != Register::None) {
			info.op_count++;
			info.op_kinds[2] = info.op_kinds[1];
			info.op_registers[2] = info.op_registers[1];
			info.op_indexes[2] = 1;
			info.op_kinds[1] = InstrOpKind::Register;
			info.op_registers[1] = kreg;
			info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
			info.flags |= static_cast<std::uint16_t>(InstrOpInfoFlags::IGNORE_OP_MASK);
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_bnd final {
public:
	explicit SimpleInstrInfo_bnd(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = flags_;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_ST_STi final {
public:
	explicit SimpleInstrInfo_ST_STi(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = 0;
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

class SimpleInstrInfo_STi_ST final {
public:
	explicit SimpleInstrInfo_STi_ST(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = 0;
		if (pseudo_op_ && options.use_pseudo_ops() && (instruction.op0_register() == Register::ST1 || instruction.op1_register() == Register::ST1))
			return InstrOpInfo(mnemonic_);
		InstrOpInfo info(mnemonic_, instruction, FLAGS);
		ICED_DEBUG_ASSERT(info.op_registers[1] == Register::ST0);
		info.op_registers[1] = REGISTER_ST;
		return info;
	}

private:
	FormatterString mnemonic_;
	bool pseudo_op_;
};

void remove_last_op(InstrOpInfo& info) noexcept {
	switch (info.op_count) {
	case 4:
		info.op_indexes[3] = OP_ACCESS_INVALID;
		break;
	case 3:
		info.op_indexes[2] = OP_ACCESS_INVALID;
		break;
	default:
		ICED_UNREACHABLE();
	}
	info.op_count--;
}

class SimpleInstrInfo_pops final {
public:
	explicit SimpleInstrInfo_pops(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), pseudo_ops_(get_pseudo_ops(static_cast<PseudoOpsKind>(e.arg3))) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		const std::size_t imm = instruction.immediate8();
		if (options.use_pseudo_ops() && imm < pseudo_ops_.size()) {
			info.mnemonic = pseudo_ops_[imm];
			remove_last_op(info);
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	PseudoOps pseudo_ops_;
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
				info.mnemonic = pseudo_ops_[static_cast<std::size_t>(index)];
				remove_last_op(info);
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
	explicit SimpleInstrInfo_imul(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		ICED_DEBUG_ASSERT(info.op_count == 3);
		if (options.use_pseudo_ops() && info.op_kinds[0] == InstrOpKind::Register && info.op_kinds[1] == InstrOpKind::Register &&
			info.op_registers[0] == info.op_registers[1]) {
			info.op_count--;
			info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ_WRITE;
			info.op_kinds[1] = info.op_kinds[2];
			info.op_indexes[1] = 2;
			info.op_indexes[2] = OP_ACCESS_INVALID;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_Reg16 final {
public:
	explicit SimpleInstrInfo_Reg16(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		InstrOpInfo info(mnemonic_, instruction, FLAGS);
		info.op_registers[0] = r_to_r16(info.op_registers[0]);
		info.op_registers[1] = r_to_r16(info.op_registers[1]);
		info.op_registers[2] = r_to_r16(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
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

class SimpleInstrInfo_reg final {
public:
	explicit SimpleInstrInfo_reg(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), register_(static_cast<Register>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		ICED_DEBUG_ASSERT(instruction.op_count() == 0);
		info.op_count = 1;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_registers[0] = register_;
		if (instruction.code() == Code::Skinit)
			info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ_WRITE;
		else
			info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	FormatterString mnemonic_;
	Register register_;
};

class SimpleInstrInfo_invlpga final {
public:
	explicit SimpleInstrInfo_invlpga(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		static_cast<void>(instruction);
		InstrOpInfo info(mnemonic_);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_kinds[1] = InstrOpKind::Register;
		info.op_registers[1] = Register::ECX;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
		switch (bitness_) {
		case 16:
			info.op_registers[0] = Register::AX;
			break;
		case 32:
			info.op_registers[0] = Register::EAX;
			break;
		case 64:
			info.op_registers[0] = Register::RAX;
			break;
		default:
			ICED_UNREACHABLE();
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_DeclareData final {
public:
	explicit SimpleInstrInfo_DeclareData(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const InstrOpKind op_kind_ = get_op_kind(instruction.code());
		InstrOpInfo info(mnemonic_, instruction, InstrOpInfoFlags::MNEMONIC_IS_DIRECTIVE);
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

class SimpleInstrInfo_bcst final {
public:
	explicit SimpleInstrInfo_bcst(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_no_broadcast_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const FormatterString bcst_to = *get_mem_size_tbl()[static_cast<std::size_t>(instruction.memory_size())].bcst_to;
		const std::uint32_t flags = !bcst_to.is_default() ? InstrOpInfoFlags::NONE : flags_no_broadcast_;
		return InstrOpInfo(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_no_broadcast_;
};

} // namespace

InstrOpInfo get_op_info(const FormatterOptions& options, const Instruction& instruction) noexcept {
	const InstrInfo& info = INSTR_INFOS[static_cast<std::size_t>(instruction.code())];
	switch (info.kind) {
	case InstrInfoKind::Simple:
		return SimpleInstrInfo(info).op_info(options, instruction);
	case InstrInfoKind::cc:
		return SimpleInstrInfo_cc(info).op_info(options, instruction);
	case InstrInfoKind::memsize:
		return SimpleInstrInfo_memsize(info).op_info(options, instruction);
	case InstrInfoKind::StringIg1:
		return SimpleInstrInfo_StringIg1(info).op_info(options, instruction);
	case InstrInfoKind::StringIg0:
		return SimpleInstrInfo_StringIg0(info).op_info(options, instruction);
	case InstrInfoKind::nop:
		return SimpleInstrInfo_nop(info).op_info(options, instruction);
	case InstrInfoKind::ST1:
		return SimpleInstrInfo_ST1(info).op_info(options, instruction);
	case InstrInfoKind::ST2:
		return SimpleInstrInfo_ST2(info).op_info(options, instruction);
	case InstrInfoKind::maskmovq:
		return SimpleInstrInfo_maskmovq(info).op_info(options, instruction);
	case InstrInfoKind::os:
		return SimpleInstrInfo_os(info).op_info(options, instruction);
	case InstrInfoKind::os_bnd:
		return SimpleInstrInfo_os_bnd(info).op_info(options, instruction);
	case InstrInfoKind::as:
		return SimpleInstrInfo_as(info).op_info(options, instruction);
	case InstrInfoKind::os_jcc:
		return SimpleInstrInfo_os_jcc(info).op_info(options, instruction);
	case InstrInfoKind::os_loop:
		return SimpleInstrInfo_os_loop(info).op_info(options, instruction);
	case InstrInfoKind::movabs:
		return SimpleInstrInfo_movabs(info).op_info(options, instruction);
	case InstrInfoKind::opmask_op:
		return SimpleInstrInfo_opmask_op(info).op_info(options, instruction);
	case InstrInfoKind::bnd:
		return SimpleInstrInfo_bnd(info).op_info(options, instruction);
	case InstrInfoKind::ST_STi:
		return SimpleInstrInfo_ST_STi(info).op_info(options, instruction);
	case InstrInfoKind::STi_ST:
		return SimpleInstrInfo_STi_ST(info).op_info(options, instruction);
	case InstrInfoKind::pops:
		return SimpleInstrInfo_pops(info).op_info(options, instruction);
	case InstrInfoKind::pclmulqdq:
		return SimpleInstrInfo_pclmulqdq(info).op_info(options, instruction);
	case InstrInfoKind::imul:
		return SimpleInstrInfo_imul(info).op_info(options, instruction);
	case InstrInfoKind::Reg16:
		return SimpleInstrInfo_Reg16(info).op_info(options, instruction);
	case InstrInfoKind::Reg32:
		return SimpleInstrInfo_Reg32(info).op_info(options, instruction);
	case InstrInfoKind::reg:
		return SimpleInstrInfo_reg(info).op_info(options, instruction);
	case InstrInfoKind::invlpga:
		return SimpleInstrInfo_invlpga(info).op_info(options, instruction);
	case InstrInfoKind::DeclareData:
		return SimpleInstrInfo_DeclareData(info).op_info(options, instruction);
	case InstrInfoKind::bcst:
		return SimpleInstrInfo_bcst(info).op_info(options, instruction);
	}
	ICED_UNREACHABLE();
}

} // namespace iced_x86::internal::intel
