// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// Rust: formatter/gas.rs

#include "iced_x86/gas_formatter.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "iced_x86/code.hpp"
#include "iced_x86/code_size.hpp"
#include "iced_x86/decorator_kind.hpp"
#include "iced_x86/format_mnemonic_options.hpp"
#include "iced_x86/formatter_text_kind.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/instruction.hpp"
#include "iced_x86/mvex_conv_fn.hpp"
#include "iced_x86/mvex_reg_mem_conv.hpp"
#include "iced_x86/number_kind.hpp"
#include "iced_x86/prefix_kind.hpp"
#include "internal/formatter/buffered_string_output.hpp"
#include "internal/formatter/fmt_common.hpp"
#include "internal/formatter/fmt_consts.hpp"
#include "internal/formatter/fmt_utils.hpp"
#include "internal/formatter/gas/info.hpp"
#include "internal/formatter/gas/instr_op_info_flags.hpp"
#include "internal/formatter/gas/instr_op_kind.hpp"
#include "internal/formatter/gas/mem_size_tbl.hpp"
#include "internal/formatter/gas/regs.hpp"
#include "internal/formatter/gas/size_override.hpp"
#include "internal/formatter/num_fmt.hpp"
#include "internal/formatter/optional_symbol_result.hpp"
#include "internal/formatter/regs_tbl_ls.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instruction_internal.hpp"
#include "internal/mvex/mvex.hpp"

namespace iced_x86 {

namespace internal::gas {

template <typename T>
constexpr NumberKind get_number_kind(bool is_signed) noexcept {
	static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "");
	if (sizeof(T) == 1)
		return is_signed ? NumberKind::Int8 : NumberKind::UInt8;
	if (sizeof(T) == 2)
		return is_signed ? NumberKind::Int16 : NumberKind::UInt16;
	if (sizeof(T) == 4)
		return is_signed ? NumberKind::Int32 : NumberKind::UInt32;
	return is_signed ? NumberKind::Int64 : NumberKind::UInt64;
}

inline std::string_view format_unsigned(NumberFormatter& number_formatter, const FormatterOptions& options,
										const NumberFormattingOptions& number_options, std::uint8_t value) {
	return number_formatter.format_u8(options, number_options, value);
}
inline std::string_view format_unsigned(NumberFormatter& number_formatter, const FormatterOptions& options,
										const NumberFormattingOptions& number_options, std::uint16_t value) {
	return number_formatter.format_u16(options, number_options, value);
}
inline std::string_view format_unsigned(NumberFormatter& number_formatter, const FormatterOptions& options,
										const NumberFormattingOptions& number_options, std::uint32_t value) {
	return number_formatter.format_u32(options, number_options, value);
}
inline std::string_view format_unsigned(NumberFormatter& number_formatter, const FormatterOptions& options,
										const NumberFormattingOptions& number_options, std::uint64_t value) {
	return number_formatter.format_u64(options, number_options, value);
}

// Methods that don't depend on the output type
struct GasFormatterCommon {
	static constexpr std::string_view IMMEDIATE_VALUE_PREFIX = "$";

	static const FormatterString* all_registers(const GasFormatter& self) noexcept {
		return self.options_.gas_naked_registers() ? self.all_registers_naked_ : self.all_registers_;
	}

	static InstrOpInfo get_op_info(const GasFormatter& self, const Instruction& instruction) noexcept {
		return internal::gas::get_op_info(self.options_, instruction);
	}

	static bool show_segment_prefix(const GasFormatter& self, const Instruction& instruction, const InstrOpInfo& op_info) noexcept {
		if ((op_info.flags & (InstrOpInfoFlags::JCC_NOT_TAKEN | InstrOpInfoFlags::JCC_TAKEN)) != 0)
			return false;

		switch (instruction.code()) {
		case Code::Monitorw:
		case Code::Monitord:
		case Code::Monitorq:
		case Code::Monitorxw:
		case Code::Monitorxd:
		case Code::Monitorxq:
		case Code::Clzerow:
		case Code::Clzerod:
		case Code::Clzeroq:
		case Code::Umonitor_r16:
		case Code::Umonitor_r32:
		case Code::Umonitor_r64:
		case Code::Maskmovq_rDI_mm_mm:
		case Code::Maskmovdqu_rDI_xmm_xmm:
		case Code::VEX_Vmaskmovdqu_rDI_xmm_xmm:
			return internal::show_segment_prefix(Register::DS, instruction, self.options_);

		default:
			break;
		}

		for (std::uint32_t i = 0; i < op_info.op_count; i++) {
			switch (op_info.op_kind(i)) {
			case InstrOpKind::Register:
			case InstrOpKind::NearBranch16:
			case InstrOpKind::NearBranch32:
			case InstrOpKind::NearBranch64:
			case InstrOpKind::FarBranch16:
			case InstrOpKind::FarBranch32:
			case InstrOpKind::Immediate8:
			case InstrOpKind::Immediate8_2nd:
			case InstrOpKind::Immediate16:
			case InstrOpKind::Immediate32:
			case InstrOpKind::Immediate64:
			case InstrOpKind::Immediate8to16:
			case InstrOpKind::Immediate8to32:
			case InstrOpKind::Immediate8to64:
			case InstrOpKind::Immediate32to64:
			case InstrOpKind::MemoryESDI:
			case InstrOpKind::MemoryESEDI:
			case InstrOpKind::MemoryESRDI:
			case InstrOpKind::Sae:
			case InstrOpKind::RnSae:
			case InstrOpKind::RdSae:
			case InstrOpKind::RuSae:
			case InstrOpKind::RzSae:
			case InstrOpKind::Rn:
			case InstrOpKind::Rd:
			case InstrOpKind::Ru:
			case InstrOpKind::Rz:
			case InstrOpKind::DeclareByte:
			case InstrOpKind::DeclareWord:
			case InstrOpKind::DeclareDword:
			case InstrOpKind::DeclareQword:
				break;

			case InstrOpKind::MemorySegSI:
			case InstrOpKind::MemorySegESI:
			case InstrOpKind::MemorySegRSI:
			case InstrOpKind::MemorySegDI:
			case InstrOpKind::MemorySegEDI:
			case InstrOpKind::MemorySegRDI:
			case InstrOpKind::Memory:
				return false;

			default:
				ICED_UNREACHABLE();
			}
		}
		return self.options_.show_useless_prefixes();
	}

	// Calls the symbol resolver (if any)
	static void get_symbol(GasFormatter& self, OptionalSymbolResult& symbol, const Instruction& instruction, std::uint32_t operand,
						   std::optional<std::uint32_t> instruction_operand, std::uint64_t address, std::uint32_t size) {
		if (self.symbol_resolver_)
			get_symbol_core(self, symbol, instruction, operand, instruction_operand, address, size);
	}

	ICED_NOINLINE static void get_symbol_core(GasFormatter& self, OptionalSymbolResult& symbol, const Instruction& instruction, std::uint32_t operand,
											   std::optional<std::uint32_t> instruction_operand, std::uint64_t address, std::uint32_t size) {
		symbol.set(self.symbol_resolver_->symbol(instruction, operand, instruction_operand, address, size));
	}

	// The resolver will be called again before the symbol is used so the symbol result is an owned copy (the text is stored in `vec`)
	ICED_NOINLINE static void get_owned_symbol(GasFormatter& self, OptionalSymbolResult& symbol, const Instruction& instruction, std::uint32_t operand,
												std::optional<std::uint32_t> instruction_operand, std::uint64_t address, std::uint32_t size,
												std::vector<SymResTextPart>& vec) {
		if (self.symbol_resolver_)
			symbol.set(to_owned(self.symbol_resolver_->symbol(instruction, operand, instruction_operand, address, size), vec));
	}

	static std::string_view get_reg_str(const GasFormatter& self, Register reg) noexcept {
		if (self.options_.prefer_st0() && reg == REGISTER_ST)
			reg = Register::ST0;
		const FormatterString& reg_str = all_registers(self)[static_cast<std::size_t>(reg)];
		return reg_str.get(self.options_.uppercase_registers() || self.options_.uppercase_all());
	}
};

// `TOutput` is `FormatterOutput` or `BufferedStringOutput`. The latter is used by `format(const Instruction&, std::string&)`:
// all writes are inlined (Rust gets the same result with LTO since only one `FormatterOutput` is used)
template <typename TOutput>
struct GasFormatterImpl : GasFormatterCommon {
	static void format_mnemonic(GasFormatter& self, const Instruction& instruction, TOutput& output, const InstrOpInfo& op_info,
								std::uint32_t& column, std::uint32_t mnemonic_options) {
		const FormatterOptions& options = self.options_;
		const FormatterConstants& str = *self.str_;
		bool need_space = false;
		if ((mnemonic_options & FormatMnemonicOptions::NO_PREFIXES) == 0 && (op_info.flags & InstrOpInfoFlags::MNEMONIC_IS_DIRECTIVE) == 0) {
			const Register prefix_seg = instruction.segment_prefix();

			constexpr std::uint32_t PREFIX_FLAGS = (InstrOpInfoFlags::SIZE_OVERRIDE_MASK << InstrOpInfoFlags::OP_SIZE_SHIFT) |
												   (InstrOpInfoFlags::SIZE_OVERRIDE_MASK << InstrOpInfoFlags::ADDR_SIZE_SHIFT) |
												   InstrOpInfoFlags::BND_PREFIX;
			if ((static_cast<std::uint32_t>(prefix_seg) | InstructionInternal::internal_has_any_of_lock_rep_repne_prefix(instruction) |
				 (static_cast<std::uint32_t>(op_info.flags) & PREFIX_FLAGS)) != 0) {
				const FormatterString* prefix;

				if ((op_info.flags & InstrOpInfoFlags::OP_SIZE_IS_BYTE_DIRECTIVE) != 0) {
					const auto size_override = static_cast<SizeOverride>((static_cast<std::uint32_t>(op_info.flags) >> InstrOpInfoFlags::OP_SIZE_SHIFT) &
																		 InstrOpInfoFlags::SIZE_OVERRIDE_MASK);
					switch (size_override) {
					case SizeOverride::None:
						break;

					case SizeOverride::Size16:
					case SizeOverride::Size32: {
						output.write(str.dot_byte.get(options.uppercase_keywords() || options.uppercase_all()), FormatterTextKind::Directive);
						output.write(" ", FormatterTextKind::Text);
						const auto number_options = NumberFormattingOptions::with_immediate(options);
						const std::string_view s = self.number_formatter_->format_u8(options, number_options, 0x66);
						output.write(s, FormatterTextKind::Number);
						output.write(";", FormatterTextKind::Punctuation);
						output.write(" ", FormatterTextKind::Text);
						column += static_cast<std::uint32_t>(str.dot_byte.len() + 1 + s.size() + 1 + 1);
						break;
					}

					case SizeOverride::Size64:
						format_prefix(options, output, instruction, column, str.rex_w, PrefixKind::OperandSize, need_space);
						break;

					default:
						ICED_UNREACHABLE();
					}
				} else {
					prefix = self.vec_->gas_op_size_strings[(static_cast<std::size_t>(op_info.flags) >> InstrOpInfoFlags::OP_SIZE_SHIFT) &
															InstrOpInfoFlags::SIZE_OVERRIDE_MASK];
					if (!prefix->is_default())
						format_prefix(options, output, instruction, column, *prefix, PrefixKind::OperandSize, need_space);
				}

				prefix = self.vec_->gas_addr_size_strings[(static_cast<std::size_t>(op_info.flags) >> InstrOpInfoFlags::ADDR_SIZE_SHIFT) &
														  InstrOpInfoFlags::SIZE_OVERRIDE_MASK];
				if (!prefix->is_default())
					format_prefix(options, output, instruction, column, *prefix, PrefixKind::AddressSize, need_space);

				const bool has_notrack_prefix = prefix_seg == Register::DS && is_notrack_prefix_branch(instruction.code());
				if (!has_notrack_prefix && prefix_seg != Register::None && show_segment_prefix(self, instruction, op_info)) {
					format_prefix(options, output, instruction, column, self.all_registers_naked_[static_cast<std::size_t>(prefix_seg)],
								  get_segment_register_prefix_kind(prefix_seg), need_space);
				}

				if (instruction.has_xacquire_prefix())
					format_prefix(options, output, instruction, column, str.xacquire, PrefixKind::Xacquire, need_space);
				if (instruction.has_xrelease_prefix())
					format_prefix(options, output, instruction, column, str.xrelease, PrefixKind::Xrelease, need_space);
				if (instruction.has_lock_prefix())
					format_prefix(options, output, instruction, column, str.lock, PrefixKind::Lock, need_space);

				if (has_notrack_prefix)
					format_prefix(options, output, instruction, column, str.notrack, PrefixKind::Notrack, need_space);
				const bool has_bnd = (op_info.flags & InstrOpInfoFlags::BND_PREFIX) != 0;
				if (has_bnd)
					format_prefix(options, output, instruction, column, str.bnd, PrefixKind::Bnd, need_space);

				if (instruction.has_repe_prefix() && show_rep_or_repe_prefix(instruction.code(), options)) {
					if (is_repe_or_repne_instruction(instruction.code()))
						format_prefix(options, output, instruction, column, get_mnemonic_cc(options, 4, str.repe), PrefixKind::Repe, need_space);
					else
						format_prefix(options, output, instruction, column, str.rep, PrefixKind::Rep, need_space);
				}
				if (!has_bnd && instruction.has_repne_prefix() && show_repne_prefix(instruction.code(), options))
					format_prefix(options, output, instruction, column, get_mnemonic_cc(options, 5, str.repne), PrefixKind::Repne, need_space);
			}
		}

		if ((mnemonic_options & FormatMnemonicOptions::NO_MNEMONIC) == 0) {
			if (need_space) {
				output.write(" ", FormatterTextKind::Text);
				column++;
			}
			const FormatterString mnemonic = op_info.mnemonic;
			if ((op_info.flags & InstrOpInfoFlags::MNEMONIC_IS_DIRECTIVE) != 0)
				output.write(mnemonic.get(options.uppercase_keywords() || options.uppercase_all()), FormatterTextKind::Directive);
			else
				output.write_mnemonic(instruction, mnemonic.get(options.uppercase_mnemonics() || options.uppercase_all()));
			column += static_cast<std::uint32_t>(mnemonic.len());
		}
		if ((mnemonic_options & FormatMnemonicOptions::NO_PREFIXES) == 0) {
			if ((op_info.flags & InstrOpInfoFlags::JCC_NOT_TAKEN) != 0)
				format_branch_hint(options, output, column, str.pn);
			else if ((op_info.flags & InstrOpInfoFlags::JCC_TAKEN) != 0)
				format_branch_hint(options, output, column, str.pt);
		}
	}

	static void format_branch_hint(const FormatterOptions& options, TOutput& output, std::uint32_t& column, const FormatterString& br_hint) {
		output.write(",", FormatterTextKind::Text);
		output.write(br_hint.get(options.uppercase_prefixes() || options.uppercase_all()), FormatterTextKind::Keyword);
		column += 1 + static_cast<std::uint32_t>(br_hint.len());
	}

	static void format_prefix(const FormatterOptions& options, TOutput& output, const Instruction& instruction, std::uint32_t& column,
							  const FormatterString& prefix, PrefixKind prefix_kind, bool& need_space) {
		if (need_space) {
			column++;
			output.write(" ", FormatterTextKind::Text);
		}
		output.write_prefix(instruction, prefix.get(options.uppercase_prefixes() || options.uppercase_all()), prefix_kind);
		column += static_cast<std::uint32_t>(prefix.len());
		need_space = true;
	}

	static void format_operands(GasFormatter& self, const Instruction& instruction, TOutput& output, const InstrOpInfo& op_info) {
		for (std::uint32_t i = 0; i < op_info.op_count; i++) {
			if (i > 0) {
				output.write(",", FormatterTextKind::Punctuation);
				if (self.options_.space_after_operand_separator())
					output.write(" ", FormatterTextKind::Text);
			}
			format_operand(self, instruction, output, op_info, i);
		}
	}

	static void format_operand(GasFormatter& self, const Instruction& instruction, TOutput& output, const InstrOpInfo& op_info,
							   std::uint32_t operand) {
		ICED_DEBUG_ASSERT(operand < op_info.op_count);

		std::uint32_t mvex_rm_operand;
		if (IcedConstants::is_mvex(instruction.code())) {
			const std::uint32_t op_count = instruction.op_count();
			ICED_DEBUG_ASSERT(op_count != 0);
			mvex_rm_operand = (instruction.op_kind(op_count - 1) == OpKind::Immediate8 && op_info.op_count == op_count) ? 1 : 0;
		} else
			mvex_rm_operand = std::numeric_limits<std::uint32_t>::max();

		const std::optional<std::uint32_t> instruction_operand = op_info.instruction_index(operand);

		if ((op_info.flags & InstrOpInfoFlags::INDIRECT_OPERAND) != 0)
			output.write("*", FormatterTextKind::Operator);

		const FormatterOptions& options = self.options_;
		const FormatterConstants& str = *self.str_;
		const InstrOpKind op_kind = op_info.op_kind(operand);
		switch (op_kind) {
		case InstrOpKind::Register:
			format_register_internal(self, output, instruction, operand, instruction_operand, op_info.op_register(operand));
			break;

		case InstrOpKind::NearBranch16:
		case InstrOpKind::NearBranch32:
		case InstrOpKind::NearBranch64:
			format_near_branch(self, instruction, output, op_kind, operand, instruction_operand);
			break;

		case InstrOpKind::FarBranch16:
		case InstrOpKind::FarBranch32:
			format_far_branch(self, instruction, output, op_kind, operand, instruction_operand);
			break;

		case InstrOpKind::Immediate8:
		case InstrOpKind::Immediate8_2nd:
		case InstrOpKind::DeclareByte: {
			if (op_kind != InstrOpKind::DeclareByte)
				output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			std::uint8_t imm8;
			if (op_kind == InstrOpKind::Immediate8)
				imm8 = instruction.immediate8();
			else if (op_kind == InstrOpKind::Immediate8_2nd)
				imm8 = instruction.immediate8_2nd();
			else
				imm8 = instruction.get_declare_byte_value(operand);
			format_immediate(self, instruction, output, operand, instruction_operand, imm8);
			break;
		}

		case InstrOpKind::Immediate16:
		case InstrOpKind::Immediate8to16:
		case InstrOpKind::DeclareWord: {
			if (op_kind != InstrOpKind::DeclareWord)
				output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			std::uint16_t imm16;
			if (op_kind == InstrOpKind::Immediate16)
				imm16 = instruction.immediate16();
			else if (op_kind == InstrOpKind::Immediate8to16)
				imm16 = static_cast<std::uint16_t>(instruction.immediate8to16());
			else
				imm16 = instruction.get_declare_word_value(operand);
			format_immediate(self, instruction, output, operand, instruction_operand, imm16);
			break;
		}

		case InstrOpKind::Immediate32:
		case InstrOpKind::Immediate8to32:
		case InstrOpKind::DeclareDword: {
			if (op_kind != InstrOpKind::DeclareDword)
				output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			std::uint32_t imm32;
			if (op_kind == InstrOpKind::Immediate32)
				imm32 = instruction.immediate32();
			else if (op_kind == InstrOpKind::Immediate8to32)
				imm32 = static_cast<std::uint32_t>(instruction.immediate8to32());
			else
				imm32 = instruction.get_declare_dword_value(operand);
			format_immediate(self, instruction, output, operand, instruction_operand, imm32);
			break;
		}

		case InstrOpKind::Immediate64:
		case InstrOpKind::Immediate8to64:
		case InstrOpKind::Immediate32to64:
		case InstrOpKind::DeclareQword: {
			if (op_kind != InstrOpKind::DeclareQword)
				output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			std::uint64_t imm64;
			if (op_kind == InstrOpKind::Immediate32to64)
				imm64 = static_cast<std::uint64_t>(instruction.immediate32to64());
			else if (op_kind == InstrOpKind::Immediate8to64)
				imm64 = static_cast<std::uint64_t>(instruction.immediate8to64());
			else if (op_kind == InstrOpKind::Immediate64)
				imm64 = instruction.immediate64();
			else
				imm64 = instruction.get_declare_qword_value(operand);
			format_immediate(self, instruction, output, operand, instruction_operand, imm64);
			break;
		}

		case InstrOpKind::MemorySegSI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::SI, Register::None, 0, 0, 0, 2);
			break;
		case InstrOpKind::MemorySegESI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::ESI, Register::None, 0, 0, 0, 4);
			break;
		case InstrOpKind::MemorySegRSI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::RSI, Register::None, 0, 0, 0, 8);
			break;
		case InstrOpKind::MemorySegDI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::DI, Register::None, 0, 0, 0, 2);
			break;
		case InstrOpKind::MemorySegEDI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::EDI, Register::None, 0, 0, 0, 4);
			break;
		case InstrOpKind::MemorySegRDI:
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), Register::RDI, Register::None, 0, 0, 0, 8);
			break;
		case InstrOpKind::MemoryESDI:
			format_memory(self, output, instruction, operand, instruction_operand, Register::ES, Register::DI, Register::None, 0, 0, 0, 2);
			break;
		case InstrOpKind::MemoryESEDI:
			format_memory(self, output, instruction, operand, instruction_operand, Register::ES, Register::EDI, Register::None, 0, 0, 0, 4);
			break;
		case InstrOpKind::MemoryESRDI:
			format_memory(self, output, instruction, operand, instruction_operand, Register::ES, Register::RDI, Register::None, 0, 0, 0, 8);
			break;

		case InstrOpKind::Memory: {
			const std::uint32_t displ_size = instruction.memory_displ_size();
			const Register base_reg = instruction.memory_base();
			Register index_reg = instruction.memory_index();
			const std::uint32_t addr_size = InstructionInternal::get_address_size_in_bytes(base_reg, index_reg, displ_size, instruction.code_size());
			const std::int64_t displ = addr_size == 8 ? static_cast<std::int64_t>(instruction.memory_displacement64())
													  : static_cast<std::int64_t>(instruction.memory_displacement32());
			if ((op_info.flags & InstrOpInfoFlags::IGNORE_INDEX_REG) != 0)
				index_reg = Register::None;
			format_memory(self, output, instruction, operand, instruction_operand, instruction.memory_segment(), base_reg, index_reg,
						  InstructionInternal::internal_get_memory_index_scale(instruction), displ_size, displ, addr_size);
			break;
		}

		case InstrOpKind::Sae:
			format_decorator(options, output, instruction, operand, instruction_operand, str.sae, DecoratorKind::SuppressAllExceptions);
			break;
		case InstrOpKind::RnSae:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rn_sae, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::RdSae:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rd_sae, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::RuSae:
			format_decorator(options, output, instruction, operand, instruction_operand, str.ru_sae, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::RzSae:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rz_sae, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::Rn:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rn, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::Rd:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rd, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::Ru:
			format_decorator(options, output, instruction, operand, instruction_operand, str.ru, DecoratorKind::RoundingControl);
			break;
		case InstrOpKind::Rz:
			format_decorator(options, output, instruction, operand, instruction_operand, str.rz, DecoratorKind::RoundingControl);
			break;

		default:
			ICED_UNREACHABLE();
		}

		if (operand + 1 == op_info.op_count && InstructionInternal::internal_has_op_mask_or_zeroing_masking(instruction)) {
			if (instruction.has_op_mask()) {
				output.write("{", FormatterTextKind::Punctuation);
				format_register_internal(self, output, instruction, operand, instruction_operand, instruction.op_mask());
				output.write("}", FormatterTextKind::Punctuation);
			}
			if (instruction.zeroing_masking())
				format_decorator(options, output, instruction, operand, instruction_operand, str.z, DecoratorKind::ZeroingMasking);
		}
		if (mvex_rm_operand == operand) {
			const MvexRegMemConv conv = instruction.mvex_reg_mem_conv();
			if (conv != MvexRegMemConv::None) {
				const MvexInfo& mvex = get_mvex_info(instruction.code());
				if (mvex.conv_fn != MvexConvFn::None) {
					const auto& tbl = mvex.is_conv_fn_32() ? self.vec_->mvex_reg_mem_consts_32 : self.vec_->mvex_reg_mem_consts_64;
					const FormatterString& s = *tbl[static_cast<std::size_t>(conv)];
					if (s.len() != 0)
						format_decorator(options, output, instruction, operand, instruction_operand, s, DecoratorKind::SwizzleMemConv);
				}
			}
		}
	}

	// The following methods are part of format_operand() in Rust. They're not inlined so format_operand()'s stack frame stays small.

	ICED_NOINLINE static void format_near_branch(GasFormatter& self, const Instruction& instruction, TOutput& output, InstrOpKind op_kind,
												  std::uint32_t operand, std::optional<std::uint32_t> instruction_operand) {
		const FormatterOptions& options = self.options_;
		std::uint32_t imm_size;
		std::uint64_t imm64;
		NumberKind number_kind;
		if (op_kind == InstrOpKind::NearBranch64) {
			imm_size = 8;
			imm64 = instruction.near_branch64();
			number_kind = NumberKind::UInt64;
		} else if (op_kind == InstrOpKind::NearBranch32) {
			imm_size = 4;
			imm64 = instruction.near_branch32();
			number_kind = NumberKind::UInt32;
		} else {
			imm_size = 2;
			imm64 = instruction.near_branch16();
			number_kind = NumberKind::UInt16;
		}
		auto number_options = NumberFormattingOptions::with_branch(options);
		FormatterOperandOptions operand_options;
		if (self.options_provider_)
			self.options_provider_->operand_options(instruction, operand, instruction_operand, operand_options, number_options);
		OptionalSymbolResult symbol;
		get_symbol(self, symbol, instruction, operand, instruction_operand, imm64, imm_size);
		if (symbol) {
			FormatterOutputMethods::write1(output, instruction, operand, instruction_operand, options, *self.number_formatter_, number_options, imm64,
										   *symbol, options.show_symbol_address());
		} else {
			const FormatterFlowControl flow_control = get_flow_control(instruction);
			std::string_view s;
			if (op_kind == InstrOpKind::NearBranch32)
				s = self.number_formatter_->format_u32_zeros(options, number_options, instruction.near_branch32(), number_options.leading_zeros);
			else if (op_kind == InstrOpKind::NearBranch64)
				s = self.number_formatter_->format_u64_zeros(options, number_options, instruction.near_branch64(), number_options.leading_zeros);
			else
				s = self.number_formatter_->format_u16_zeros(options, number_options, instruction.near_branch16(), number_options.leading_zeros);
			output.write_number(instruction, operand, instruction_operand, s, imm64, number_kind,
								is_call(flow_control) ? FormatterTextKind::FunctionAddress : FormatterTextKind::LabelAddress);
		}
	}

	// Immediate8/16/32/64 and db/dw/dd/dq (T = std::uint8_t/16/32/64)
	template <typename T>
	ICED_NOINLINE static void format_immediate(GasFormatter& self, const Instruction& instruction, TOutput& output, std::uint32_t operand,
												std::optional<std::uint32_t> instruction_operand, T imm) {
		using S = std::make_signed_t<T>;
		const FormatterOptions& options = self.options_;
		auto number_options = NumberFormattingOptions::with_immediate(options);
		FormatterOperandOptions operand_options;
		if (self.options_provider_)
			self.options_provider_->operand_options(instruction, operand, instruction_operand, operand_options, number_options);
		OptionalSymbolResult symbol;
		get_symbol(self, symbol, instruction, operand, instruction_operand, imm, sizeof(T));
		if (symbol) {
			FormatterOutputMethods::write1(output, instruction, operand, instruction_operand, options, *self.number_formatter_, number_options, imm,
										   *symbol, options.show_symbol_address());
		} else {
			std::uint64_t value64;
			NumberKind number_kind;
			if (number_options.signed_number) {
				value64 = static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<S>(imm)));
				number_kind = get_number_kind<T>(true);
				if (static_cast<S>(imm) < 0) {
					output.write("-", FormatterTextKind::Operator);
					imm = static_cast<T>(static_cast<T>(0) - imm);
				}
			} else {
				value64 = imm;
				number_kind = get_number_kind<T>(false);
			}
			const std::string_view s = format_unsigned(*self.number_formatter_, options, number_options, imm);
			output.write_number(instruction, operand, instruction_operand, s, value64, number_kind, FormatterTextKind::Number);
		}
	}

	ICED_NOINLINE static void format_far_branch_symbol(GasFormatter& self, const Instruction& instruction, TOutput& output,
														std::uint32_t operand, std::optional<std::uint32_t> instruction_operand,
														const NumberFormattingOptions& number_options, std::uint64_t imm64, const SymbolResult& symbol) {
		const FormatterOptions& options = self.options_;
		output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
		ICED_DEBUG_ASSERT(operand + 1 == 1);
		OptionalSymbolResult selector_symbol;
		get_symbol(self, selector_symbol, instruction, operand + 1, instruction_operand, instruction.far_branch_selector(), 2);
		if (selector_symbol) {
			FormatterOutputMethods::write1(output, instruction, operand, instruction_operand, options, *self.number_formatter_, number_options,
										   instruction.far_branch_selector(), *selector_symbol, options.show_symbol_address());
		} else {
			const std::string_view s =
				self.number_formatter_->format_u16_zeros(options, number_options, instruction.far_branch_selector(), number_options.leading_zeros);
			output.write_number(instruction, operand, instruction_operand, s, instruction.far_branch_selector(), NumberKind::UInt16,
								FormatterTextKind::SelectorValue);
		}
		output.write(",", FormatterTextKind::Punctuation);
		if (options.space_after_operand_separator())
			output.write(" ", FormatterTextKind::Text);
		output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
		FormatterOutputMethods::write1(output, instruction, operand, instruction_operand, options, *self.number_formatter_, number_options, imm64,
									   symbol, options.show_symbol_address());
	}

	ICED_NOINLINE static void format_far_branch(GasFormatter& self, const Instruction& instruction, TOutput& output, InstrOpKind op_kind,
												 std::uint32_t operand, std::optional<std::uint32_t> instruction_operand) {
		const FormatterOptions& options = self.options_;
		std::uint32_t imm_size;
		std::uint64_t imm64;
		NumberKind number_kind;
		if (op_kind == InstrOpKind::FarBranch32) {
			imm_size = 4;
			imm64 = instruction.far_branch32();
			number_kind = NumberKind::UInt32;
		} else {
			imm_size = 2;
			imm64 = instruction.far_branch16();
			number_kind = NumberKind::UInt16;
		}
		auto number_options = NumberFormattingOptions::with_branch(options);
		FormatterOperandOptions operand_options;
		if (self.options_provider_)
			self.options_provider_->operand_options(instruction, operand, instruction_operand, operand_options, number_options);
		std::vector<SymResTextPart> vec;
		OptionalSymbolResult symbol;
		get_owned_symbol(self, symbol, instruction, operand, instruction_operand, static_cast<std::uint32_t>(imm64), imm_size, vec);
		if (symbol) {
			format_far_branch_symbol(self, instruction, output, operand, instruction_operand, number_options, imm64, *symbol);
		} else {
			const FormatterFlowControl flow_control = get_flow_control(instruction);
			std::string_view s =
				self.number_formatter_->format_u16_zeros(options, number_options, instruction.far_branch_selector(), number_options.leading_zeros);
			output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			output.write_number(instruction, operand, instruction_operand, s, instruction.far_branch_selector(), NumberKind::UInt16,
								FormatterTextKind::SelectorValue);
			output.write(",", FormatterTextKind::Punctuation);
			if (options.space_after_operand_separator())
				output.write(" ", FormatterTextKind::Text);
			if (op_kind == InstrOpKind::FarBranch32)
				s = self.number_formatter_->format_u32_zeros(options, number_options, instruction.far_branch32(), number_options.leading_zeros);
			else
				s = self.number_formatter_->format_u16_zeros(options, number_options, instruction.far_branch16(), number_options.leading_zeros);
			output.write(IMMEDIATE_VALUE_PREFIX, FormatterTextKind::Operator);
			output.write_number(instruction, operand, instruction_operand, s, imm64, number_kind,
								is_call(flow_control) ? FormatterTextKind::FunctionAddress : FormatterTextKind::LabelAddress);
		}
	}

	static void format_decorator(const FormatterOptions& options, TOutput& output, const Instruction& instruction, std::uint32_t operand,
								 std::optional<std::uint32_t> instruction_operand, const FormatterString& text, DecoratorKind decorator) {
		output.write("{", FormatterTextKind::Punctuation);
		output.write_decorator(instruction, operand, instruction_operand, text.get(options.uppercase_decorators() || options.uppercase_all()), decorator);
		output.write("}", FormatterTextKind::Punctuation);
	}

	static void format_register_internal(const GasFormatter& self, TOutput& output, const Instruction& instruction, std::uint32_t operand,
										 std::optional<std::uint32_t> instruction_operand, Register reg) {
		output.write_register(instruction, operand, instruction_operand, get_reg_str(self, reg), reg);
	}

	// Part of format_memory() in Rust (not inlined to keep the stack frames small): formats the symbol or the displacement
	static void format_memory_displ(GasFormatter& self, TOutput& output, const Instruction& instruction, std::uint32_t operand,
												   std::optional<std::uint32_t> instruction_operand, const NumberFormattingOptions& number_options,
												   const OptionalSymbolResult& symbol, bool has_base_or_index_reg, std::uint32_t displ_size,
												   std::int64_t displ, std::uint32_t addr_size, std::uint64_t abs_addr) {
		const FormatterOptions& options = self.options_;
		if (symbol) {
			FormatterOutputMethods::write1(output, instruction, operand, instruction_operand, options, *self.number_formatter_, number_options, abs_addr,
										   *symbol, options.show_symbol_address());
		} else if (!has_base_or_index_reg || (displ_size != 0 && (options.show_zero_displacements() || displ != 0))) {
			const auto orig_displ = static_cast<std::uint64_t>(displ);
			bool is_signed;
			if (has_base_or_index_reg) {
				is_signed = number_options.signed_number;
				if (addr_size == 8) {
					if (number_options.signed_number && displ < 0) {
						output.write("-", FormatterTextKind::Operator);
						displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
					}
					if (number_options.displacement_leading_zeros)
						displ_size = 4;
				} else if (addr_size == 4) {
					if (number_options.signed_number && static_cast<std::int32_t>(displ) < 0) {
						output.write("-", FormatterTextKind::Operator);
						displ = static_cast<std::int64_t>(static_cast<std::uint32_t>(0U - static_cast<std::uint32_t>(displ)));
					}
					if (number_options.displacement_leading_zeros)
						displ_size = 4;
				} else {
					ICED_DEBUG_ASSERT(addr_size == 2);
					if (number_options.signed_number && static_cast<std::int16_t>(displ) < 0) {
						output.write("-", FormatterTextKind::Operator);
						displ = static_cast<std::int64_t>(static_cast<std::uint16_t>(0U - static_cast<std::uint16_t>(displ)));
					}
					if (number_options.displacement_leading_zeros)
						displ_size = 2;
				}
			} else
				is_signed = false;

			std::string_view s;
			NumberKind displ_kind;
			if (displ_size <= 1 && static_cast<std::uint64_t>(displ) <= std::numeric_limits<std::uint8_t>::max()) {
				s = self.number_formatter_->format_displ_u8(options, number_options, static_cast<std::uint8_t>(displ));
				displ_kind = is_signed ? NumberKind::Int8 : NumberKind::UInt8;
			} else if (displ_size <= 2 && static_cast<std::uint64_t>(displ) <= std::numeric_limits<std::uint16_t>::max()) {
				s = self.number_formatter_->format_displ_u16(options, number_options, static_cast<std::uint16_t>(displ));
				displ_kind = is_signed ? NumberKind::Int16 : NumberKind::UInt16;
			} else if (displ_size <= 4 && static_cast<std::uint64_t>(displ) <= std::numeric_limits<std::uint32_t>::max()) {
				s = self.number_formatter_->format_displ_u32(options, number_options, static_cast<std::uint32_t>(displ));
				displ_kind = is_signed ? NumberKind::Int32 : NumberKind::UInt32;
			} else if (displ_size <= 8) {
				s = self.number_formatter_->format_displ_u64(options, number_options, static_cast<std::uint64_t>(displ));
				displ_kind = is_signed ? NumberKind::Int64 : NumberKind::UInt64;
			} else
				ICED_UNREACHABLE();
			output.write_number(instruction, operand, instruction_operand, s, orig_displ, displ_kind, FormatterTextKind::Number);
		}
	}

	ICED_NOINLINE static void format_memory(GasFormatter& self, TOutput& output, const Instruction& instruction, std::uint32_t operand,
							  std::optional<std::uint32_t> instruction_operand, Register seg_reg, Register base_reg, Register index_reg,
							  std::uint32_t scale, std::uint32_t displ_size, std::int64_t displ, std::uint32_t addr_size) {
		ICED_DEBUG_ASSERT(scale < sizeof(SCALE_NUMBERS) / sizeof(SCALE_NUMBERS[0]));
		ICED_DEBUG_ASSERT(InstructionInternal::get_address_size_in_bytes(base_reg, index_reg, displ_size, instruction.code_size()) == addr_size);

		const FormatterOptions& options = self.options_;
		auto operand_options = FormatterOperandOptions::with_memory_size_options(options.memory_size_options());
		operand_options.set_rip_relative_addresses(options.rip_relative_addresses());
		auto number_options = NumberFormattingOptions::with_displacement(options);
		if (self.options_provider_)
			self.options_provider_->operand_options(instruction, operand, instruction_operand, operand_options, number_options);

		std::uint64_t abs_addr;
		if (base_reg == Register::RIP) {
			abs_addr = static_cast<std::uint64_t>(displ);
			if (options.rip_relative_addresses())
				displ = static_cast<std::int64_t>(static_cast<std::uint64_t>(displ) - instruction.next_ip());
			else {
				ICED_DEBUG_ASSERT(index_reg == Register::None);
				base_reg = Register::None;
			}
			displ_size = 8;
		} else if (base_reg == Register::EIP) {
			abs_addr = static_cast<std::uint32_t>(displ);
			if (options.rip_relative_addresses())
				displ = static_cast<std::int32_t>(static_cast<std::uint32_t>(displ) - instruction.next_ip32());
			else {
				ICED_DEBUG_ASSERT(index_reg == Register::None);
				base_reg = Register::None;
			}
			displ_size = 4;
		} else
			abs_addr = static_cast<std::uint64_t>(displ);

		OptionalSymbolResult symbol;
		get_symbol(self, symbol, instruction, operand, instruction_operand, abs_addr, addr_size);

		const bool use_scale = addr_size == 2 || !show_index_scale(instruction, options) ? false : scale != 0 || options.always_show_scale();

		const bool has_base_or_index_reg = base_reg != Register::None || index_reg != Register::None;

		const CodeSize code_size = instruction.code_size();
		const Register seg_override = instruction.segment_prefix();
		const bool notrack_prefix = seg_override == Register::DS && is_notrack_prefix_branch(instruction.code()) &&
									!((code_size == CodeSize::Code16 || code_size == CodeSize::Code32) &&
									  (base_reg == Register::BP || base_reg == Register::EBP || base_reg == Register::ESP));
		if (options.always_show_segment_register() ||
			(seg_override != Register::None && !notrack_prefix && internal::show_segment_prefix(Register::None, instruction, options))) {
			format_register_internal(self, output, instruction, operand, instruction_operand, seg_reg);
			output.write(":", FormatterTextKind::Punctuation);
		}

		format_memory_displ(self, output, instruction, operand, instruction_operand, number_options, symbol, has_base_or_index_reg, displ_size, displ,
							addr_size, abs_addr);

		if (has_base_or_index_reg) {
			output.write("(", FormatterTextKind::Punctuation);
			if (options.space_after_memory_bracket())
				output.write(" ", FormatterTextKind::Text);

			if (base_reg != Register::None && index_reg == Register::None && !use_scale)
				format_register_internal(self, output, instruction, operand, instruction_operand, base_reg);
			else {
				if (base_reg != Register::None)
					format_register_internal(self, output, instruction, operand, instruction_operand, base_reg);

				output.write(",", FormatterTextKind::Punctuation);
				if (options.gas_space_after_memory_operand_comma())
					output.write(" ", FormatterTextKind::Text);

				if (index_reg != Register::None) {
					format_register_internal(self, output, instruction, operand, instruction_operand, index_reg);

					if (use_scale) {
						output.write(",", FormatterTextKind::Punctuation);
						if (options.gas_space_after_memory_operand_comma())
							output.write(" ", FormatterTextKind::Text);

						output.write_number(instruction, operand, instruction_operand, SCALE_NUMBERS[scale], 1ULL << scale, NumberKind::Int32,
											FormatterTextKind::Number);
					}
				}
			}

			if (options.space_after_memory_bracket())
				output.write(" ", FormatterTextKind::Text);
			output.write(")", FormatterTextKind::Punctuation);
		}

		const MemorySize mem_size = instruction.memory_size();
		ICED_DEBUG_ASSERT(static_cast<std::size_t>(mem_size) < IcedConstants::MEMORY_SIZE_ENUM_COUNT);
		const FormatterString& bcst_to = *self.all_memory_sizes_[static_cast<std::size_t>(mem_size)];
		if (!bcst_to.is_default())
			format_decorator(options, output, instruction, operand, instruction_operand, bcst_to, DecoratorKind::Broadcast);
		if (instruction.is_mvex_eviction_hint())
			format_decorator(options, output, instruction, operand, instruction_operand, self.str_->mvex.eh, DecoratorKind::EvictionHint);
	}

	static void add_tabs(TOutput& output, std::uint32_t column, std::uint32_t first_operand_char_index, std::uint32_t tab_size) {
		// Fast path (default options): same as `internal::add_tabs()` but the write can be inlined
		if (tab_size == 0 && first_operand_char_index <= column)
			output.write(" ", FormatterTextKind::Text);
		else
			internal::add_tabs(output, column, first_operand_char_index, tab_size);
	}
};

} // namespace internal::gas

using GasFormatterImpl = internal::gas::GasFormatterImpl<FormatterOutput>;
#ifndef ICED_X86_NO_FORMATTER_STRING_SPECIALIZATION
using GasFormatterStringImpl = internal::gas::GasFormatterImpl<internal::BufferedStringOutput>;
#endif
using internal::gas::InstrOpInfo;

GasFormatter::GasFormatter() : GasFormatter(nullptr, nullptr) {}

GasFormatter::GasFormatter(std::unique_ptr<SymbolResolver> symbol_resolver, std::unique_ptr<FormatterOptionsProvider> options_provider)
	: options_(FormatterOptions::with_gas())
	, all_registers_(internal::gas::get_all_registers().data())
	, all_registers_naked_(internal::get_regs_tbl().data())
	, all_memory_sizes_(internal::gas::get_mem_size_tbl().data())
	, str_(&internal::get_formatter_constants())
	, vec_(&internal::get_array_constants())
	, number_formatter_(std::make_unique<internal::NumberFormatter>())
	, symbol_resolver_(std::move(symbol_resolver))
	, options_provider_(std::move(options_provider)) {}

GasFormatter::~GasFormatter() = default;
GasFormatter::GasFormatter(GasFormatter&& other) noexcept = default;
GasFormatter& GasFormatter::operator=(GasFormatter&& other) noexcept = default;

void GasFormatter::format_mnemonic_options(const Instruction& instruction, FormatterOutput& output, std::uint32_t options) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	std::uint32_t column = 0;
	GasFormatterImpl::format_mnemonic(*this, instruction, output, op_info, column, options);
}

std::uint32_t GasFormatter::operand_count(const Instruction& instruction) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	return op_info.op_count;
}

Result<std::optional<OpAccess>> GasFormatter::op_access(const Instruction& instruction, std::uint32_t operand) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	if (operand >= op_info.op_count)
		return IcedError("Invalid operand");
	return op_info.op_access(operand);
}

Result<std::optional<std::uint32_t>> GasFormatter::get_instruction_operand(const Instruction& instruction, std::uint32_t operand) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	if (operand >= op_info.op_count)
		return IcedError("Invalid operand");
	return op_info.instruction_index(operand);
}

Result<std::optional<std::uint32_t>> GasFormatter::get_formatter_operand(const Instruction& instruction, std::uint32_t instruction_operand) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	if (instruction_operand >= instruction.op_count())
		return IcedError("Invalid instruction operand");
	return op_info.operand_index(instruction_operand);
}

Result<void> GasFormatter::format_operand(const Instruction& instruction, FormatterOutput& output, std::uint32_t operand) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	if (operand >= op_info.op_count)
		return IcedError("Invalid operand");
	GasFormatterImpl::format_operand(*this, instruction, output, op_info, operand);
	return {};
}

void GasFormatter::format_operand_separator(const Instruction& instruction, FormatterOutput& output) {
	static_cast<void>(instruction);
	output.write(",", FormatterTextKind::Punctuation);
	if (options_.space_after_operand_separator())
		output.write(" ", FormatterTextKind::Text);
}

void GasFormatter::format_all_operands(const Instruction& instruction, FormatterOutput& output) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);
	GasFormatterImpl::format_operands(*this, instruction, output, op_info);
}

void GasFormatter::format(const Instruction& instruction, FormatterOutput& output) {
	const InstrOpInfo op_info = GasFormatterImpl::get_op_info(*this, instruction);

	std::uint32_t column = 0;
	GasFormatterImpl::format_mnemonic(*this, instruction, output, op_info, column, FormatMnemonicOptions::NONE);

	if (op_info.op_count != 0) {
		GasFormatterImpl::add_tabs(output, column, options_.first_operand_char_index(), options_.tab_size());
		GasFormatterImpl::format_operands(*this, instruction, output, op_info);
	}
}

void GasFormatter::format(const Instruction& instruction, std::string& output_string) {
#ifdef ICED_X86_NO_FORMATTER_STRING_SPECIALIZATION
	StringFormatterOutput output(output_string);
	format(instruction, output);
#else
	if (ICED_UNLIKELY(!string_buffer_))
		string_buffer_ = std::make_unique<internal::FormatterStringBuffer>();
	internal::BufferedStringOutput output(*string_buffer_);
	const InstrOpInfo op_info = GasFormatterStringImpl::get_op_info(*this, instruction);

	std::uint32_t column = 0;
	GasFormatterStringImpl::format_mnemonic(*this, instruction, output, op_info, column, FormatMnemonicOptions::NONE);

	if (op_info.op_count != 0) {
		GasFormatterStringImpl::add_tabs(output, column, options_.first_operand_char_index(), options_.tab_size());
		GasFormatterStringImpl::format_operands(*this, instruction, output, op_info);
	}
	output.flush(output_string);
#endif
}

std::string_view GasFormatter::format_register(Register register_) { return GasFormatterImpl::get_reg_str(*this, register_); }

std::string_view GasFormatter::format_i8(std::int8_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_i8(options_, number_options, value);
}

std::string_view GasFormatter::format_i16(std::int16_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_i16(options_, number_options, value);
}

std::string_view GasFormatter::format_i32(std::int32_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_i32(options_, number_options, value);
}

std::string_view GasFormatter::format_i64(std::int64_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_i64(options_, number_options, value);
}

std::string_view GasFormatter::format_u8(std::uint8_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_u8(options_, number_options, value);
}

std::string_view GasFormatter::format_u16(std::uint16_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_u16(options_, number_options, value);
}

std::string_view GasFormatter::format_u32(std::uint32_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_u32(options_, number_options, value);
}

std::string_view GasFormatter::format_u64(std::uint64_t value) {
	const auto number_options = NumberFormattingOptions::with_immediate(options_);
	return number_formatter_->format_u64(options_, number_options, value);
}

std::string_view GasFormatter::format_i8_options(std::int8_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_i8(options_, number_options, value);
}

std::string_view GasFormatter::format_i16_options(std::int16_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_i16(options_, number_options, value);
}

std::string_view GasFormatter::format_i32_options(std::int32_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_i32(options_, number_options, value);
}

std::string_view GasFormatter::format_i64_options(std::int64_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_i64(options_, number_options, value);
}

std::string_view GasFormatter::format_u8_options(std::uint8_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_u8(options_, number_options, value);
}

std::string_view GasFormatter::format_u16_options(std::uint16_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_u16(options_, number_options, value);
}

std::string_view GasFormatter::format_u32_options(std::uint32_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_u32(options_, number_options, value);
}

std::string_view GasFormatter::format_u64_options(std::uint64_t value, const NumberFormattingOptions& number_options) {
	return number_formatter_->format_u64(options_, number_options, value);
}

} // namespace iced_x86
