// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/nasm/info.rs, fmt_tbl.rs

#include "internal/formatter/nasm/info.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/rounding_control.hpp"
#include "internal/formatter/fmt_common.hpp"
#include "internal/formatter/fmt_utils.hpp"
#include "internal/formatter/nasm/branch_size_info.hpp"
#include "internal/formatter/nasm/far_memory_size_info.hpp"
#include "internal/formatter/nasm/fmt_data.hpp"
#include "internal/formatter/nasm/instr_op_info_flags.hpp"
#include "internal/formatter/nasm/mem_size_tbl.hpp"
#include "internal/formatter/nasm/memory_size_info.hpp"
#include "internal/formatter/nasm/sign_extend_info.hpp"
#include "internal/formatter/pseudo_ops.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"

namespace iced_x86::internal::nasm {

static_assert(IcedConstants::MEMORY_SIZE_ENUM_COUNT <= (1U << InstrOpInfoFlags::MEMORY_SIZE_BITS), "");

namespace {

constexpr FormatterStringData<sizeof("xchg")> STR_XCHG("xchg");

// `InstrInfo::arg3` value if there's no cc index
constexpr std::uint8_t NO_CC_INDEX_U8 = 0xFF;

FormatterString str(std::uint32_t offset) noexcept { return FormatterString(STRINGS + offset); }

std::uint32_t get_bitness(CodeSize code_size) noexcept {
	static constexpr std::uint32_t CODESIZE_TO_BITNESS[4] = {0, 16, 32, 64};
	static_assert(static_cast<std::uint32_t>(CodeSize::Unknown) == 0, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code16) == 1, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code32) == 2, "");
	static_assert(static_cast<std::uint32_t>(CodeSize::Code64) == 3, "");
	return CODESIZE_TO_BITNESS[static_cast<std::size_t>(code_size)];
}

class SimpleInstrInfo final {
public:
	explicit SimpleInstrInfo(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
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
		return InstrOpInfo::with_instruction(mnemonic, instruction, FLAGS);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t cc_index_;
};

class SimpleInstrInfo_push_imm8 final {
public:
	explicit SimpleInstrInfo_push_imm8(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), sex_info_(static_cast<SignExtendInfo>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = static_cast<std::uint32_t>(sex_info_) << InstrOpInfoFlags::SIGN_EXTEND_INFO_SHIFT;

		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (bitness_ != 0 && instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}

		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	SignExtendInfo sex_info_;
};

class SimpleInstrInfo_push_imm final {
public:
	explicit SimpleInstrInfo_push_imm(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), sex_info_(static_cast<SignExtendInfo>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;

		bool sign_extend = true;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (bitness_ != 0 && instr_bitness != 0 && instr_bitness != bitness_) {
			if (instr_bitness == 64)
				flags |= InstrOpInfoFlags::OP_SIZE16;
		}
		else if (bitness_ == 16 && instr_bitness == 16)
			sign_extend = false;

		if (sign_extend)
			flags |= static_cast<std::uint32_t>(sex_info_) << InstrOpInfoFlags::SIGN_EXTEND_INFO_SHIFT;

		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	SignExtendInfo sex_info_;
};

class SimpleInstrInfo_SignExt final {
public:
	explicit SimpleInstrInfo_SignExt(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, sex_info_reg_(static_cast<SignExtendInfo>(e.arg1))
		, sex_info_mem_(static_cast<SignExtendInfo>(e.arg3))
		, flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		const SignExtendInfo sex_info =
			instruction.op0_kind() == OpKind::Memory || instruction.op1_kind() == OpKind::Memory ? sex_info_mem_ : sex_info_reg_;
		const std::uint32_t flags = flags_ | (static_cast<std::uint32_t>(sex_info) << InstrOpInfoFlags::SIGN_EXTEND_INFO_SHIFT);
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	SignExtendInfo sex_info_reg_;
	SignExtendInfo sex_info_mem_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_imul final {
public:
	explicit SimpleInstrInfo_imul(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), sex_info_(static_cast<SignExtendInfo>(e.arg3)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const std::uint32_t flags = static_cast<std::uint32_t>(sex_info_) << InstrOpInfoFlags::SIGN_EXTEND_INFO_SHIFT;
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
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
	SignExtendInfo sex_info_;
};

class SimpleInstrInfo_AamAad final {
public:
	explicit SimpleInstrInfo_AamAad(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		if (instruction.immediate8() == 10)
			return InstrOpInfo::with_default(mnemonic_);
		return InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
	}

private:
	FormatterString mnemonic_;
};

constexpr std::uint32_t get_address_size_flags(OpKind op_kind) noexcept {
	switch (op_kind) {
	case OpKind::MemorySegSI:
	case OpKind::MemorySegDI:
	case OpKind::MemoryESDI:
		return InstrOpInfoFlags::ADDR_SIZE16;
	case OpKind::MemorySegESI:
	case OpKind::MemorySegEDI:
	case OpKind::MemoryESEDI:
		return InstrOpInfoFlags::ADDR_SIZE32;
	case OpKind::MemorySegRSI:
	case OpKind::MemorySegRDI:
	case OpKind::MemoryESRDI:
		return InstrOpInfoFlags::ADDR_SIZE64;
	default:
		return 0;
	}
}

class SimpleInstrInfo_String final {
public:
	explicit SimpleInstrInfo_String(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const OpKind op_kind = instruction.op0_kind() != OpKind::Register ? instruction.op0_kind() : instruction.op1_kind();
		const std::uint32_t op_kind_flags = get_address_size_flags(op_kind);
		std::uint32_t instr_flags;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			instr_flags = op_kind_flags;
			break;
		case CodeSize::Code16:
			instr_flags = InstrOpInfoFlags::ADDR_SIZE16;
			break;
		case CodeSize::Code32:
			instr_flags = InstrOpInfoFlags::ADDR_SIZE32;
			break;
		case CodeSize::Code64:
		default:
			instr_flags = InstrOpInfoFlags::ADDR_SIZE64;
			break;
		}
		const std::uint32_t flags = op_kind_flags != instr_flags ? op_kind_flags : 0;
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.flags = flags;
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_XLAT final {
public:
	explicit SimpleInstrInfo_XLAT(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		Register base_reg;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			base_reg = instruction.memory_base();
			break;
		case CodeSize::Code16:
			base_reg = Register::BX;
			break;
		case CodeSize::Code32:
			base_reg = Register::EBX;
			break;
		case CodeSize::Code64:
		default:
			base_reg = Register::RBX;
			break;
		}
		std::uint32_t flags = 0;
		const Register mem_base_reg = instruction.memory_base();
		if (mem_base_reg != base_reg) {
			if (mem_base_reg == Register::BX)
				flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (mem_base_reg == Register::EBX)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else if (mem_base_reg == Register::RBX)
				flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.flags = flags;
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
			return InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		auto info = InstrOpInfo::with_default(FormatterString(STR_XCHG));
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
		auto info = InstrOpInfo::with_default(mnemonic_);
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

class SimpleInstrInfo_STIG2 final {
public:
	explicit SimpleInstrInfo_STIG2(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, flags_(e.arg1 != 0 ? InstrOpInfoFlags::REGISTER_TO : InstrOpInfoFlags::NONE)
		, pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.flags = flags_;
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		ICED_DEBUG_ASSERT(instruction.op1_kind() == OpKind::Register && instruction.op1_register() == Register::ST0);
		if (!pseudo_op_ || !(options.use_pseudo_ops() && instruction.op0_register() == Register::ST1)) {
			info.op_count = 1;
			static_assert(static_cast<std::uint32_t>(InstrOpKind::Register) == 0, "");
			// info.op_kinds[0] = InstrOpKind::Register;
			info.op_registers[0] = instruction.op0_register();
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
	bool pseudo_op_;
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
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
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

		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[0] = 1;
		info.op_registers[0] = instruction.op1_register();
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op2_kind());
		info.op_indexes[1] = 2;
		info.op_registers[1] = instruction.op2_register();
		if (instr_bitness != 0 && instr_bitness != bitness) {
			if (bitness == 16)
				info.flags |= InstrOpInfoFlags::ADDR_SIZE16;
			else if (bitness == 32)
				info.flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else
				info.flags |= InstrOpInfoFlags::ADDR_SIZE64;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_pblendvb final {
public:
	explicit SimpleInstrInfo_pblendvb(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mem_size_(static_cast<MemorySize>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_default(mnemonic_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		info.op_count = 3;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op0_kind());
		info.op_registers[0] = instruction.op0_register();
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[1] = 1;
		info.op_registers[1] = instruction.op1_register();
		static_assert(static_cast<std::uint32_t>(InstrOpKind::Register) == 0, "");
		// info.op_kinds[2] = InstrOpKind::Register;
		info.op_registers[2] = Register::XMM0;
		info.set_memory_size(mem_size_);
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	FormatterString mnemonic_;
	MemorySize mem_size_;
};

class SimpleInstrInfo_reverse final {
public:
	explicit SimpleInstrInfo_reverse(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_default(mnemonic_);
		ICED_DEBUG_ASSERT(instruction.op_count() == 2);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[0] = 1;
		info.op_registers[0] = instruction.op1_register();
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op0_kind());
		info.op_registers[1] = instruction.op0_register();
		info.set_memory_size(instruction.memory_size());
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
		static_cast<void>(options);
		const FormatterString mnemonic = instruction.code_size() == code_size_ ? mnemonics_[static_cast<std::size_t>(CodeSize::Unknown)]
																	 : mnemonics_[static_cast<std::size_t>(code_size_)];
		return InstrOpInfo::with_instruction(mnemonic, instruction, InstrOpInfoFlags::NONE);
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
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString mnemonic = mnemonics_[static_cast<std::size_t>(instruction.code_size())];
		return InstrOpInfo::with_instruction(mnemonic, instruction, flags);
	}

private:
	std::array<FormatterString, 4> mnemonics_;
};

class SimpleInstrInfo_OpSize3 final {
public:
	explicit SimpleInstrInfo_OpSize3(const InstrInfo& e) noexcept
		: mnemonic_default_(str(e.mnemonic)), mnemonic_full_(str(e.arg1)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const FormatterString mnemonic = instr_bitness == 0 || (instr_bitness & bitness_) != 0 ? mnemonic_default_ : mnemonic_full_;
		return InstrOpInfo::with_instruction(mnemonic, instruction, InstrOpInfoFlags::NONE);
	}

private:
	FormatterString mnemonic_default_;
	FormatterString mnemonic_full_;
	std::uint32_t bitness_;
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
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_os_mem final {
public:
	explicit SimpleInstrInfo_os_mem(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const bool has_mem_op = instruction.op0_kind() == OpKind::Memory || instruction.op1_kind() == OpKind::Memory;
		if (has_mem_op &&
			!(instr_bitness == 0 || (instr_bitness != 64 && instr_bitness == bitness_) || (instr_bitness == 64 && bitness_ == 32))) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_os_mem2 final {
public:
	explicit SimpleInstrInfo_os_mem2(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = flags_;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		if (instr_bitness != 0 && (instr_bitness & bitness_) == 0) {
			if (instr_bitness != 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else
				flags |= InstrOpInfoFlags::OP_SIZE32;
		}
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_os_mem_reg16 final {
public:
	explicit SimpleInstrInfo_os_mem_reg16(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		ICED_DEBUG_ASSERT(instruction.op_count() == 1);
		if (instruction.op0_kind() == OpKind::Memory) {
			if (!(instr_bitness == 0 || (instr_bitness != 64 && instr_bitness == bitness_) || (instr_bitness == 64 && bitness_ == 32))) {
				if (bitness_ == 16)
					flags |= InstrOpInfoFlags::OP_SIZE16;
				else if (bitness_ == 32)
					flags |= InstrOpInfoFlags::OP_SIZE32;
				else
					flags |= InstrOpInfoFlags::OP_SIZE64;
			}
		}
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
		if (instruction.op0_kind() == OpKind::Register) {
			Register reg = info.op_registers[0];
			std::uint32_t reg_size;
			if (Register::AX <= reg && reg <= Register::R15W)
				reg_size = 16;
			else if (Register::EAX <= reg && reg <= Register::R15D) {
				reg = r_to_r16(reg);
				reg_size = 32;
			}
			else if (Register::RAX <= reg && reg <= Register::R15) {
				reg = r_to_r16(reg);
				reg_size = 64;
			}
			else
				reg_size = 0;
			ICED_DEBUG_ASSERT(reg_size != 0);
			if (reg_size != 0) {
				info.op_registers[0] = reg;
				if (!((instr_bitness != 64 && instr_bitness == reg_size) || (instr_bitness == 64 && reg_size == 32))) {
					if (bitness_ == 16)
						info.flags |= InstrOpInfoFlags::OP_SIZE16;
					else if (bitness_ == 32)
						info.flags |= InstrOpInfoFlags::OP_SIZE32;
					else
						info.flags |= InstrOpInfoFlags::OP_SIZE64;
				}
			}
		}
		return info;
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
		if (flags != InstrOpInfoFlags::NONE) {
			if (instr_bitness != 0 && instr_bitness != bitness_) {
				if (bitness_ == 16)
					flags |= InstrOpInfoFlags::OP_SIZE16;
				else if (bitness_ == 32)
					flags |= InstrOpInfoFlags::OP_SIZE32;
				else
					flags |= InstrOpInfoFlags::OP_SIZE64;
			}
		}
		else {
			BranchSizeInfo branch_info = BranchSizeInfo::Near;
			if (instr_bitness != 0 && instr_bitness != bitness_) {
				if (bitness_ == 16)
					branch_info = BranchSizeInfo::NearWord;
				else if (bitness_ == 32)
					branch_info = BranchSizeInfo::NearDword;
			}
			flags |= static_cast<std::uint32_t>(branch_info) << InstrOpInfoFlags::BRANCH_SIZE_INFO_SHIFT;
		}
		const Register prefix_seg = instruction.segment_prefix();
		if (prefix_seg == Register::CS)
			flags |= InstrOpInfoFlags::JCC_NOT_TAKEN;
		else if (prefix_seg == Register::DS)
			flags |= InstrOpInfoFlags::JCC_TAKEN;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString mnemonic = str(mnemonics_[get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_))]);
		return InstrOpInfo::with_instruction(mnemonic, instruction, flags);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_os_loop final {
public:
	explicit SimpleInstrInfo_os_loop(const InstrInfo& e) noexcept
		: mnemonics_(ARGS + e.arg1 + 1)
		, bitness_(e.arg2)
		, cc_index_(e.arg3 == NO_CC_INDEX_U8 ? 0xFFFF'FFFF : e.arg3)
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
		const bool add_reg = expected_reg != register_;
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				flags |= InstrOpInfoFlags::OP_SIZE16;
			else if (bitness_ == 32)
				flags |= InstrOpInfoFlags::OP_SIZE32;
			else
				flags |= InstrOpInfoFlags::OP_SIZE64;
		}
		const std::size_t index = cc_index_ == 0xFFFF'FFFF ? 0 : get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_));
		const FormatterString mnemonic = str(mnemonics_[index]);
		auto info = InstrOpInfo::with_instruction(mnemonic, instruction, flags);
		if (add_reg) {
			ICED_DEBUG_ASSERT(info.op_count == 1);
			info.op_count = 2;
			info.op_kinds[1] = InstrOpKind::Register;
			info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ_WRITE;
			info.op_registers[1] = register_;
		}
		return info;
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t bitness_;
	std::uint32_t cc_index_;
	Register register_;
};

class SimpleInstrInfo_os_call final {
public:
	explicit SimpleInstrInfo_os_call(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), bitness_(e.arg3), can_have_bnd_prefix_(e.arg1 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		if (can_have_bnd_prefix_ && instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		BranchSizeInfo branch_info = BranchSizeInfo::None;
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				branch_info = BranchSizeInfo::Word;
			else if (bitness_ == 32)
				branch_info = BranchSizeInfo::Dword;
		}
		flags |= static_cast<std::uint32_t>(branch_info) << InstrOpInfoFlags::BRANCH_SIZE_INFO_SHIFT;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
	bool can_have_bnd_prefix_;
};

class SimpleInstrInfo_far final {
public:
	explicit SimpleInstrInfo_far(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		BranchSizeInfo branch_info = BranchSizeInfo::None;
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				branch_info = BranchSizeInfo::Word;
			else
				branch_info = BranchSizeInfo::Dword;
		}
		flags |= static_cast<std::uint32_t>(branch_info) << InstrOpInfoFlags::BRANCH_SIZE_INFO_SHIFT;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
};

class SimpleInstrInfo_far_mem final {
public:
	explicit SimpleInstrInfo_far_mem(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::SHOW_NO_MEM_SIZE_FORCE_SIZE;
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		FarMemorySizeInfo far_mem_size_info = FarMemorySizeInfo::None;
		if (instr_bitness != 0 && instr_bitness != bitness_) {
			if (bitness_ == 16)
				far_mem_size_info = FarMemorySizeInfo::Word;
			else
				far_mem_size_info = FarMemorySizeInfo::Dword;
		}
		flags |= static_cast<std::uint32_t>(far_mem_size_info) << InstrOpInfoFlags::FAR_MEMORY_SIZE_INFO_SHIFT;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
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
		MemorySizeInfo mem_size_info = MemorySizeInfo::None;
		if (instr_bitness == 64) {
			if (mem_size == 32)
				flags |= InstrOpInfoFlags::ADDR_SIZE32;
			else
				mem_size_info = MemorySizeInfo::Qword;
		}
		else if (instr_bitness != mem_size) {
			ICED_DEBUG_ASSERT(mem_size == 16 || mem_size == 32);
			if (mem_size == 16)
				mem_size_info = MemorySizeInfo::Word;
			else
				mem_size_info = MemorySizeInfo::Dword;
		}
		flags |= static_cast<std::uint32_t>(mem_size_info) << InstrOpInfoFlags::MEMORY_SIZE_INFO_SHIFT;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
};

// Rust: `SimpleInstrInfo_er::move_operands()`
void move_operands(InstrOpInfo& info, std::uint32_t index, InstrOpKind new_op_kind) noexcept {
	ICED_DEBUG_ASSERT(info.op_count <= 4);

	switch (index) {
	case 2:
		ICED_DEBUG_ASSERT(info.op_count < 4 || info.op_kinds[3] != InstrOpKind::Register);
		info.op_kinds[4] = info.op_kinds[3];
		info.op_kinds[3] = info.op_kinds[2];
		info.op_registers[3] = info.op_registers[2];
		info.op_kinds[2] = new_op_kind;
		info.op_indexes[4] = info.op_indexes[3];
		info.op_indexes[3] = info.op_indexes[2];
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_NONE;
		info.op_count++;
		break;

	case 3:
		ICED_DEBUG_ASSERT(info.op_count < 4 || info.op_kinds[3] != InstrOpKind::Register);
		info.op_kinds[4] = info.op_kinds[3];
		info.op_kinds[3] = new_op_kind;
		info.op_indexes[4] = info.op_indexes[3];
		info.op_indexes[3] = InstrInfoConstants::OP_ACCESS_NONE;
		info.op_count++;
		break;

	default:
		ICED_UNREACHABLE();
	}
}

class SimpleInstrInfo_er final {
public:
	explicit SimpleInstrInfo_er(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), er_index_(e.arg3), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
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
					case RoundingControl::None:
					default:
						return info;
					}
				}
				else {
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
					case RoundingControl::None:
					default:
						return info;
					}
				}
				move_operands(info, er_index_, rc_op_kind);
			}
			else if (instruction.suppress_all_exceptions())
				move_operands(info, er_index_, InstrOpKind::Sae);
		}
		else {
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
				case RoundingControl::None:
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
	std::uint32_t er_index_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_sae final {
public:
	explicit SimpleInstrInfo_sae(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), sae_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (instruction.suppress_all_exceptions())
			move_operands(info, sae_index_, InstrOpKind::Sae);
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t sae_index_;
};

class SimpleInstrInfo_bcst final {
public:
	explicit SimpleInstrInfo_bcst(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), flags_no_broadcast_(e.arg2), mem_size_tbl_(get_mem_size_tbl()) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const FormatterString* bcst_to = mem_size_tbl_[static_cast<std::size_t>(instruction.memory_size())].bcst_to;
		const std::uint32_t flags = !bcst_to->is_default() ? InstrOpInfoFlags::NONE : flags_no_broadcast_;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_no_broadcast_;
	const MemSizeInfo* mem_size_tbl_;
};

class SimpleInstrInfo_bnd final {
public:
	explicit SimpleInstrInfo_bnd(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = flags_;
		if (instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

void remove_last_op(InstrOpInfo& info) noexcept {
	switch (info.op_count) {
	case 5:
		info.op_indexes[4] = OP_ACCESS_INVALID;
		break;
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
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (instruction.suppress_all_exceptions())
			move_operands(info, instruction.op_count() - 1, InstrOpKind::Sae);
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
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		if (options.use_pseudo_ops()) {
			std::size_t index;
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
				return info;
			}
			info.mnemonic = pseudo_ops_[index];
			remove_last_op(info);
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	PseudoOps pseudo_ops_;
};

class SimpleInstrInfo_Reg16 final {
public:
	explicit SimpleInstrInfo_Reg16(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		constexpr std::uint32_t FLAGS = InstrOpInfoFlags::NONE;
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, FLAGS);
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
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, FLAGS);
		info.op_registers[0] = r64_to_r32(info.op_registers[0]);
		info.op_registers[1] = r64_to_r32(info.op_registers[1]);
		info.op_registers[2] = r64_to_r32(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_invlpga final {
public:
	explicit SimpleInstrInfo_invlpga(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		static_cast<void>(instruction);
		auto info = InstrOpInfo::with_default(mnemonic_);
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
		const InstrOpKind op_kind_ = to_op_kind(instruction.code());
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::MNEMONIC_IS_DIRECTIVE);
		info.op_count = static_cast<std::uint8_t>(instruction.declare_data_len());
		for (std::size_t i = 0; i < IcedConstants::MAX_OP_COUNT; i++) {
			info.op_kinds[i] = op_kind_;
			info.op_indexes[i] = InstrInfoConstants::OP_ACCESS_READ;
		}
		return info;
	}

private:
	static InstrOpKind to_op_kind(Code code) noexcept {
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
	case InstrInfoKind::push_imm8:
		return SimpleInstrInfo_push_imm8(info).op_info(options, instruction);
	case InstrInfoKind::push_imm:
		return SimpleInstrInfo_push_imm(info).op_info(options, instruction);
	case InstrInfoKind::SignExt:
		return SimpleInstrInfo_SignExt(info).op_info(options, instruction);
	case InstrInfoKind::imul:
		return SimpleInstrInfo_imul(info).op_info(options, instruction);
	case InstrInfoKind::AamAad:
		return SimpleInstrInfo_AamAad(info).op_info(options, instruction);
	case InstrInfoKind::String:
		return SimpleInstrInfo_String(info).op_info(options, instruction);
	case InstrInfoKind::XLAT:
		return SimpleInstrInfo_XLAT(info).op_info(options, instruction);
	case InstrInfoKind::nop:
		return SimpleInstrInfo_nop(info).op_info(options, instruction);
	case InstrInfoKind::STIG1:
		return SimpleInstrInfo_STIG1(info).op_info(options, instruction);
	case InstrInfoKind::STIG2:
		return SimpleInstrInfo_STIG2(info).op_info(options, instruction);
	case InstrInfoKind::as:
		return SimpleInstrInfo_as(info).op_info(options, instruction);
	case InstrInfoKind::maskmovq:
		return SimpleInstrInfo_maskmovq(info).op_info(options, instruction);
	case InstrInfoKind::pblendvb:
		return SimpleInstrInfo_pblendvb(info).op_info(options, instruction);
	case InstrInfoKind::reverse:
		return SimpleInstrInfo_reverse(info).op_info(options, instruction);
	case InstrInfoKind::OpSize:
		return SimpleInstrInfo_OpSize(info).op_info(options, instruction);
	case InstrInfoKind::OpSize2_bnd:
		return SimpleInstrInfo_OpSize2_bnd(info).op_info(options, instruction);
	case InstrInfoKind::OpSize3:
		return SimpleInstrInfo_OpSize3(info).op_info(options, instruction);
	case InstrInfoKind::os:
		return SimpleInstrInfo_os(info).op_info(options, instruction);
	case InstrInfoKind::os_mem:
		return SimpleInstrInfo_os_mem(info).op_info(options, instruction);
	case InstrInfoKind::os_mem2:
		return SimpleInstrInfo_os_mem2(info).op_info(options, instruction);
	case InstrInfoKind::os_mem_reg16:
		return SimpleInstrInfo_os_mem_reg16(info).op_info(options, instruction);
	case InstrInfoKind::os_jcc:
		return SimpleInstrInfo_os_jcc(info).op_info(options, instruction);
	case InstrInfoKind::os_loop:
		return SimpleInstrInfo_os_loop(info).op_info(options, instruction);
	case InstrInfoKind::os_call:
		return SimpleInstrInfo_os_call(info).op_info(options, instruction);
	case InstrInfoKind::far:
		return SimpleInstrInfo_far(info).op_info(options, instruction);
	case InstrInfoKind::far_mem:
		return SimpleInstrInfo_far_mem(info).op_info(options, instruction);
	case InstrInfoKind::movabs:
		return SimpleInstrInfo_movabs(info).op_info(options, instruction);
	case InstrInfoKind::er:
		return SimpleInstrInfo_er(info).op_info(options, instruction);
	case InstrInfoKind::sae:
		return SimpleInstrInfo_sae(info).op_info(options, instruction);
	case InstrInfoKind::bcst:
		return SimpleInstrInfo_bcst(info).op_info(options, instruction);
	case InstrInfoKind::bnd:
		return SimpleInstrInfo_bnd(info).op_info(options, instruction);
	case InstrInfoKind::pops:
		return SimpleInstrInfo_pops(info).op_info(options, instruction);
	case InstrInfoKind::pclmulqdq:
		return SimpleInstrInfo_pclmulqdq(info).op_info(options, instruction);
	case InstrInfoKind::Reg16:
		return SimpleInstrInfo_Reg16(info).op_info(options, instruction);
	case InstrInfoKind::Reg32:
		return SimpleInstrInfo_Reg32(info).op_info(options, instruction);
	case InstrInfoKind::invlpga:
		return SimpleInstrInfo_invlpga(info).op_info(options, instruction);
	case InstrInfoKind::DeclareData:
		return SimpleInstrInfo_DeclareData(info).op_info(options, instruction);
	}
	ICED_UNREACHABLE();
}

} // namespace iced_x86::internal::nasm
