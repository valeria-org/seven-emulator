// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/masm/info.rs, fmt_tbl.rs

#include "internal/formatter/masm/info.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "internal/formatter/fmt_common.hpp"
#include "internal/formatter/fmt_utils.hpp"
#include "internal/formatter/masm/fmt_data.hpp"
#include "internal/formatter/masm/instr_op_info_flags.hpp"
#include "internal/formatter/pseudo_ops.hpp"
#include "internal/formatter/pseudo_ops_kind.hpp"

namespace iced_x86::internal::masm {

namespace {

constexpr FormatterStringData<sizeof("xchg")> STR_XCHG("xchg");

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
	explicit SimpleInstrInfo_cc(const InstrInfo& e) noexcept : mnemonics_(ARGS + e.arg1), cc_index_(e.arg3), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const FormatterString mnemonic = str(mnemonics_[get_mnemonic_cc_index(options, cc_index_, get_cc_mnemonics_count(cc_index_))]);
		return InstrOpInfo::with_instruction(mnemonic, instruction, flags_);
	}

private:
	// Offsets in STRINGS
	const std::uint16_t* mnemonics_;
	std::uint32_t cc_index_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_memsize final {
public:
	explicit SimpleInstrInfo_memsize(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), bitness_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const std::uint32_t instr_bitness = get_bitness(instruction.code_size());
		const std::uint32_t flags =
			instr_bitness == 0 || (instr_bitness & bitness_) != 0
				? InstrOpInfoFlags::MEM_SIZE_NOTHING
				: InstrOpInfoFlags::MEM_SIZE_NORMAL | InstrOpInfoFlags::SHOW_NO_MEM_SIZE_FORCE_SIZE | InstrOpInfoFlags::SHOW_MIN_MEM_SIZE_FORCE_SIZE;
		return InstrOpInfo::with_instruction(mnemonic_, instruction, flags);
	}

private:
	FormatterString mnemonic_;
	std::uint32_t bitness_;
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

class SimpleInstrInfo_Int3 final {
public:
	explicit SimpleInstrInfo_Int3(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		static_cast<void>(instruction);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 1;
		info.op_kinds[0] = InstrOpKind::ExtraImmediate8_Value3;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	FormatterString mnemonic_;
};

constexpr std::uint32_t FLAGS_STRING_SHORT_FORM = InstrOpInfoFlags::SHOW_NO_MEM_SIZE_FORCE_SIZE | InstrOpInfoFlags::SHOW_MIN_MEM_SIZE_FORCE_SIZE;

// Base class of the string instruction infos that have a short form without operands (eg. `movsb`)
class StringInstrInfo {
protected:
	explicit StringInstrInfo(const InstrInfo& e) noexcept : mnemonic_args_(str(e.mnemonic)), mnemonic_no_args_(str(e.arg1)) {}

	FormatterString mnemonic_args_;
	FormatterString mnemonic_no_args_;
};

class SimpleInstrInfo_YD final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_YD(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op0_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemoryESDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemoryESEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemoryESRDI;
			break;
		}
		const bool short_form = instruction.op0_kind() == short_form_op_kind;
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_args_, instruction, FLAGS_STRING_SHORT_FORM);
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_DX final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_DX(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op1_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemorySegSI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemorySegESI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemorySegRSI;
			break;
		}
		const bool short_form = instruction.op1_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_args_, instruction, FLAGS_STRING_SHORT_FORM);
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_YX final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_YX(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op0_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemoryESDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemoryESEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemoryESRDI;
			break;
		}
		const bool short_form = instruction.op0_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_args_, instruction, FLAGS_STRING_SHORT_FORM);
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_XY final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_XY(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op1_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemoryESDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemoryESEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemoryESRDI;
			break;
		}
		const bool short_form = instruction.op1_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_args_, instruction, FLAGS_STRING_SHORT_FORM);
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_YA final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_YA(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op0_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemoryESDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemoryESEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemoryESRDI;
			break;
		}
		const bool short_form = instruction.op0_kind() == short_form_op_kind;
		if (!short_form) {
			auto info = InstrOpInfo::with_default(mnemonic_args_);
			info.flags = FLAGS_STRING_SHORT_FORM;
			info.op_count = 1;
			info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op0_kind());
			return info;
		}
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_AX final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_AX(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op1_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemorySegSI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemorySegESI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemorySegRSI;
			break;
		}
		const bool short_form = instruction.op1_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form) {
			auto info = InstrOpInfo::with_default(mnemonic_args_);
			info.flags = FLAGS_STRING_SHORT_FORM;
			info.op_count = 1;
			info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
			info.op_indexes[0] = 1;
			return info;
		}
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_AY final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_AY(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op1_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemoryESDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemoryESEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemoryESRDI;
			break;
		}
		const bool short_form = instruction.op1_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form) {
			auto info = InstrOpInfo::with_default(mnemonic_args_);
			info.flags = FLAGS_STRING_SHORT_FORM;
			info.op_count = 1;
			info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
			info.op_indexes[0] = 1;
			return info;
		}
		auto info = InstrOpInfo::with_default(mnemonic_no_args_);
		info.flags = FLAGS_STRING_SHORT_FORM;
		return info;
	}
};

class SimpleInstrInfo_XLAT final : public StringInstrInfo {
public:
	explicit SimpleInstrInfo_XLAT(const InstrInfo& e) noexcept : StringInstrInfo(e) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
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
		const bool short_form = instruction.memory_base() == base_reg &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_args_, instruction,
												 InstrOpInfoFlags::SHOW_NO_MEM_SIZE_FORCE_SIZE | InstrOpInfoFlags::IGNORE_INDEX_REG);
		return InstrOpInfo::with_default(mnemonic_no_args_);
	}
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

class SimpleInstrInfo_STi_ST final {
public:
	explicit SimpleInstrInfo_STi_ST(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), pseudo_op_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		constexpr std::uint32_t FLAGS = 0;
		if (pseudo_op_ && options.use_pseudo_ops() && (instruction.op0_register() == Register::ST1 || instruction.op1_register() == Register::ST1))
			return InstrOpInfo::with_default(mnemonic_);
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, FLAGS);
		ICED_DEBUG_ASSERT(info.op_registers[1] == Register::ST0);
		info.op_registers[1] = REGISTER_ST;
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
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
		ICED_DEBUG_ASSERT(info.op_registers[0] == Register::ST0);
		info.op_registers[0] = REGISTER_ST;
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_monitor final {
public:
	explicit SimpleInstrInfo_monitor(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic))
		, register1_(static_cast<Register>(e.arg1))
		, register2_(static_cast<Register>(e.arg2))
		, register3_(static_cast<Register>(e.arg3)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 3;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_registers[0] = register1_;
		info.op_kinds[1] = InstrOpKind::Register;
		info.op_registers[1] = register2_;
		info.op_kinds[2] = InstrOpKind::Register;
		info.op_registers[2] = register3_;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_READ;
		if ((instruction.code_size() == CodeSize::Code64 || instruction.code_size() == CodeSize::Unknown) &&
			(Register::EAX <= register2_ && register2_ <= Register::R15D)) {
			info.op_registers[1] = static_cast<Register>(static_cast<std::uint32_t>(info.op_registers[1]) + 0x10);
			info.op_registers[2] = static_cast<Register>(static_cast<std::uint32_t>(info.op_registers[2]) + 0x10);
		}
		return info;
	}

private:
	FormatterString mnemonic_;
	Register register1_;
	Register register2_;
	Register register3_;
};

class SimpleInstrInfo_mwait final {
public:
	explicit SimpleInstrInfo_mwait(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 2;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_kinds[1] = InstrOpKind::Register;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;

		switch (instruction.code_size()) {
		case CodeSize::Code16:
			info.op_registers[0] = Register::AX;
			info.op_registers[1] = Register::ECX;
			break;
		case CodeSize::Code32:
			info.op_registers[0] = Register::EAX;
			info.op_registers[1] = Register::ECX;
			break;
		case CodeSize::Unknown:
		case CodeSize::Code64:
		default:
			info.op_registers[0] = Register::RAX;
			info.op_registers[1] = Register::RCX;
			break;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_mwaitx final {
public:
	explicit SimpleInstrInfo_mwaitx(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 3;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_kinds[1] = InstrOpKind::Register;
		info.op_kinds[2] = InstrOpKind::Register;
		info.op_indexes[0] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[1] = InstrInfoConstants::OP_ACCESS_READ;
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_COND_READ;

		switch (instruction.code_size()) {
		case CodeSize::Code16:
			info.op_registers[0] = Register::AX;
			info.op_registers[1] = Register::ECX;
			info.op_registers[2] = Register::EBX;
			break;
		case CodeSize::Code32:
			info.op_registers[0] = Register::EAX;
			info.op_registers[1] = Register::ECX;
			info.op_registers[2] = Register::EBX;
			break;
		case CodeSize::Unknown:
		case CodeSize::Code64:
		default:
			info.op_registers[0] = Register::RAX;
			info.op_registers[1] = Register::RCX;
			info.op_registers[2] = Register::RBX;
			break;
		}
		return info;
	}

private:
	FormatterString mnemonic_;
};

class SimpleInstrInfo_maskmovq final {
public:
	explicit SimpleInstrInfo_maskmovq(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		ICED_DEBUG_ASSERT(instruction.op_count() == 3);
		OpKind short_form_op_kind;
		switch (instruction.code_size()) {
		case CodeSize::Unknown:
			short_form_op_kind = instruction.op0_kind();
			break;
		case CodeSize::Code16:
			short_form_op_kind = OpKind::MemorySegDI;
			break;
		case CodeSize::Code32:
			short_form_op_kind = OpKind::MemorySegEDI;
			break;
		case CodeSize::Code64:
		default:
			short_form_op_kind = OpKind::MemorySegRDI;
			break;
		}
		const bool short_form = instruction.op0_kind() == short_form_op_kind &&
								(instruction.segment_prefix() == Register::None || !show_segment_prefix(Register::None, instruction, options));
		if (!short_form)
			return InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.flags = flags_;
		info.op_count = 2;
		info.op_kinds[0] = InstrOpInfo::to_instr_op_kind(instruction.op1_kind());
		info.op_indexes[0] = 1;
		info.op_registers[0] = instruction.op1_register();
		info.op_kinds[1] = InstrOpInfo::to_instr_op_kind(instruction.op2_kind());
		info.op_indexes[1] = 2;
		info.op_registers[1] = instruction.op2_register();
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_pblendvb final {
public:
	explicit SimpleInstrInfo_pblendvb(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

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
		info.op_kinds[2] = InstrOpKind::Register;
		info.op_registers[2] = Register::XMM0;
		info.op_indexes[2] = InstrInfoConstants::OP_ACCESS_READ;
		return info;
	}

private:
	FormatterString mnemonic_;
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

class SimpleInstrInfo_OpSize_cc final {
public:
	explicit SimpleInstrInfo_OpSize_cc(const InstrInfo& e) noexcept
		: mnemonics_(ARGS + e.arg1), cc_index_(e.arg3), code_size_(static_cast<CodeSize>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		const std::size_t count = get_cc_mnemonics_count(cc_index_);
		const std::uint16_t* mnemonics = instruction.code_size() == code_size_ ? mnemonics_ : mnemonics_ + count;
		const FormatterString mnemonic = str(mnemonics[get_mnemonic_cc_index(options, cc_index_, count)]);
		return InstrOpInfo::with_instruction(mnemonic, instruction, InstrOpInfoFlags::NONE);
	}

private:
	// The mnemonics followed by the other mnemonics (offsets in STRINGS)
	const std::uint16_t* mnemonics_;
	std::uint32_t cc_index_;
	CodeSize code_size_;
};

class SimpleInstrInfo_OpSize2 final {
public:
	explicit SimpleInstrInfo_OpSize2(const InstrInfo& e) noexcept
		: mnemonics_{str(e.mnemonic), str(ARGS[e.arg1]), str(ARGS[e.arg1 + 1U]), str(ARGS[e.arg1 + 2U])}, can_use_bnd_(e.arg3 != 0) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		std::uint32_t flags = InstrOpInfoFlags::NONE;
		if (can_use_bnd_ && instruction.has_repne_prefix())
			flags |= InstrOpInfoFlags::BND_PREFIX;
		const FormatterString mnemonic = mnemonics_[static_cast<std::size_t>(instruction.code_size())];
		return InstrOpInfo::with_instruction(mnemonic, instruction, flags);
	}

private:
	std::array<FormatterString, 4> mnemonics_;
	bool can_use_bnd_;
};

class SimpleInstrInfo_fword final {
public:
	explicit SimpleInstrInfo_fword(const InstrInfo& e) noexcept
		: mnemonic_(str(e.mnemonic)), mnemonic2_(str(e.arg1)), code_size_(static_cast<CodeSize>(e.arg3)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		const FormatterString mnemonic =
			instruction.code_size() == code_size_ || instruction.code_size() == CodeSize::Unknown ? mnemonic_ : mnemonic2_;
		return InstrOpInfo::with_instruction(mnemonic, instruction, flags_);
	}

private:
	FormatterString mnemonic_;
	FormatterString mnemonic2_;
	CodeSize code_size_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_jcc final {
public:
	explicit SimpleInstrInfo_jcc(const InstrInfo& e) noexcept : mnemonics_(ARGS + e.arg1), cc_index_(e.arg3) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		std::uint32_t flags = InstrOpInfoFlags::NONE;
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
	std::uint32_t cc_index_;
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
		: mnemonic_(str(e.mnemonic)), pseudo_ops_(get_pseudo_ops(static_cast<PseudoOpsKind>(e.arg3))), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
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
	std::uint32_t flags_;
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

class SimpleInstrInfo_imul final {
public:
	explicit SimpleInstrInfo_imul(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, InstrOpInfoFlags::NONE);
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
	explicit SimpleInstrInfo_Reg16(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
		info.op_registers[0] = r_to_r16(info.op_registers[0]);
		info.op_registers[1] = r_to_r16(info.op_registers[1]);
		info.op_registers[2] = r_to_r16(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_Reg32 final {
public:
	explicit SimpleInstrInfo_Reg32(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), flags_(e.arg2) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		auto info = InstrOpInfo::with_instruction(mnemonic_, instruction, flags_);
		info.op_registers[0] = r64_to_r32(info.op_registers[0]);
		info.op_registers[1] = r64_to_r32(info.op_registers[1]);
		info.op_registers[2] = r64_to_r32(info.op_registers[2]);
		return info;
	}

private:
	FormatterString mnemonic_;
	std::uint32_t flags_;
};

class SimpleInstrInfo_reg final {
public:
	explicit SimpleInstrInfo_reg(const InstrInfo& e) noexcept : mnemonic_(str(e.mnemonic)), register_(static_cast<Register>(e.arg2)) {}

	InstrOpInfo op_info(const FormatterOptions& options, const Instruction& instruction) const noexcept {
		static_cast<void>(options);
		static_cast<void>(instruction);
		auto info = InstrOpInfo::with_default(mnemonic_);
		info.op_count = 1;
		info.op_kinds[0] = InstrOpKind::Register;
		info.op_registers[0] = register_;
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
	case InstrInfoKind::memsize:
		return SimpleInstrInfo_memsize(info).op_info(options, instruction);
	case InstrInfoKind::AamAad:
		return SimpleInstrInfo_AamAad(info).op_info(options, instruction);
	case InstrInfoKind::Int3:
		return SimpleInstrInfo_Int3(info).op_info(options, instruction);
	case InstrInfoKind::YD:
		return SimpleInstrInfo_YD(info).op_info(options, instruction);
	case InstrInfoKind::DX:
		return SimpleInstrInfo_DX(info).op_info(options, instruction);
	case InstrInfoKind::YX:
		return SimpleInstrInfo_YX(info).op_info(options, instruction);
	case InstrInfoKind::XY:
		return SimpleInstrInfo_XY(info).op_info(options, instruction);
	case InstrInfoKind::YA:
		return SimpleInstrInfo_YA(info).op_info(options, instruction);
	case InstrInfoKind::AX:
		return SimpleInstrInfo_AX(info).op_info(options, instruction);
	case InstrInfoKind::AY:
		return SimpleInstrInfo_AY(info).op_info(options, instruction);
	case InstrInfoKind::XLAT:
		return SimpleInstrInfo_XLAT(info).op_info(options, instruction);
	case InstrInfoKind::nop:
		return SimpleInstrInfo_nop(info).op_info(options, instruction);
	case InstrInfoKind::STIG1:
		return SimpleInstrInfo_STIG1(info).op_info(options, instruction);
	case InstrInfoKind::STi_ST:
		return SimpleInstrInfo_STi_ST(info).op_info(options, instruction);
	case InstrInfoKind::ST_STi:
		return SimpleInstrInfo_ST_STi(info).op_info(options, instruction);
	case InstrInfoKind::monitor:
		return SimpleInstrInfo_monitor(info).op_info(options, instruction);
	case InstrInfoKind::mwait:
		return SimpleInstrInfo_mwait(info).op_info(options, instruction);
	case InstrInfoKind::mwaitx:
		return SimpleInstrInfo_mwaitx(info).op_info(options, instruction);
	case InstrInfoKind::maskmovq:
		return SimpleInstrInfo_maskmovq(info).op_info(options, instruction);
	case InstrInfoKind::pblendvb:
		return SimpleInstrInfo_pblendvb(info).op_info(options, instruction);
	case InstrInfoKind::reverse:
		return SimpleInstrInfo_reverse(info).op_info(options, instruction);
	case InstrInfoKind::OpSize:
		return SimpleInstrInfo_OpSize(info).op_info(options, instruction);
	case InstrInfoKind::OpSize_cc:
		return SimpleInstrInfo_OpSize_cc(info).op_info(options, instruction);
	case InstrInfoKind::OpSize2:
		return SimpleInstrInfo_OpSize2(info).op_info(options, instruction);
	case InstrInfoKind::fword:
		return SimpleInstrInfo_fword(info).op_info(options, instruction);
	case InstrInfoKind::jcc:
		return SimpleInstrInfo_jcc(info).op_info(options, instruction);
	case InstrInfoKind::bnd:
		return SimpleInstrInfo_bnd(info).op_info(options, instruction);
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
	}
	ICED_UNREACHABLE();
}

} // namespace iced_x86::internal::masm
