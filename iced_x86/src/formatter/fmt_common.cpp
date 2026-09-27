// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/formatter/fmt_common.hpp"

#include <cstddef>
#include <limits>

#include "iced_x86/iced_constants.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

void FormatterOutputMethods::write2(FormatterOutput& output, const Instruction& instruction, std::uint32_t operand,
									std::optional<std::uint32_t> instruction_operand, const FormatterOptions& options, NumberFormatter& number_formatter,
									const NumberFormattingOptions& number_options, std::uint64_t address, const SymbolResult& symbol,
									bool show_symbol_address, bool write_minus_if_signed, bool spaces_between_op) {
	auto displ = static_cast<std::int64_t>(address - symbol.address);
	if ((symbol.flags & SymbolFlags::SIGNED) != 0) {
		if (write_minus_if_signed)
			output.write("-", FormatterTextKind::Operator);
		displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
	}
	output.write_symbol(instruction, operand, instruction_operand, address, symbol);
	NumberKind number_kind;
	if (displ != 0) {
		if (spaces_between_op)
			output.write(" ", FormatterTextKind::Text);
		const auto orig_displ = static_cast<std::uint64_t>(displ);
		if (displ < 0) {
			output.write("-", FormatterTextKind::Operator);
			displ = static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(displ));
			if (displ <= static_cast<std::int64_t>(std::numeric_limits<std::int8_t>::max()) + 1)
				number_kind = NumberKind::Int8;
			else if (displ <= static_cast<std::int64_t>(std::numeric_limits<std::int16_t>::max()) + 1)
				number_kind = NumberKind::Int16;
			else if (displ <= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) + 1)
				number_kind = NumberKind::Int32;
			else
				number_kind = NumberKind::Int64;
		}
		else {
			output.write("+", FormatterTextKind::Operator);
			if (displ <= std::numeric_limits<std::int8_t>::max())
				number_kind = NumberKind::Int8;
			else if (displ <= std::numeric_limits<std::int16_t>::max())
				number_kind = NumberKind::Int16;
			else if (displ <= std::numeric_limits<std::int32_t>::max())
				number_kind = NumberKind::Int32;
			else
				number_kind = NumberKind::Int64;
		}
		if (spaces_between_op)
			output.write(" ", FormatterTextKind::Text);
		const auto s = number_formatter.format_u64_zeros(options, number_options, static_cast<std::uint64_t>(displ), false);
		output.write_number(instruction, operand, instruction_operand, s, orig_displ, number_kind, FormatterTextKind::Number);
	}
	if (show_symbol_address) {
		output.write(" ", FormatterTextKind::Text);
		output.write("(", FormatterTextKind::Punctuation);
		std::string_view s;
		if (address <= std::numeric_limits<std::uint16_t>::max()) {
			number_kind = NumberKind::UInt16;
			s = number_formatter.format_u16_zeros(options, number_options, static_cast<std::uint16_t>(address), true);
		}
		else if (address <= std::numeric_limits<std::uint32_t>::max()) {
			number_kind = NumberKind::UInt32;
			s = number_formatter.format_u32_zeros(options, number_options, static_cast<std::uint32_t>(address), true);
		}
		else {
			number_kind = NumberKind::UInt64;
			s = number_formatter.format_u64_zeros(options, number_options, address, true);
		}
		output.write_number(instruction, operand, instruction_operand, s, address, number_kind, FormatterTextKind::Number);
		output.write(")", FormatterTextKind::Punctuation);
	}
}

std::size_t get_mnemonic_cc_index(const FormatterOptions& options, std::uint32_t cc_index, std::size_t mnemonics_size) noexcept {
	std::size_t index;
	switch (cc_index) {
	// o
	case 0:
		ICED_DEBUG_ASSERT(mnemonics_size == 1);
		index = 0;
		break;
	// no
	case 1:
		ICED_DEBUG_ASSERT(mnemonics_size == 1);
		index = 0;
		break;
	// b, c, nae
	case 2:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_B_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_b());
		break;
	// ae, nb, nc
	case 3:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_AE_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_ae());
		break;
	// e, z
	case 4:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_E_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_e());
		break;
	// ne, nz
	case 5:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_NE_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_ne());
		break;
	// be, na
	case 6:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_BE_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_be());
		break;
	// a, nbe
	case 7:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_A_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_a());
		break;
	// s
	case 8:
		ICED_DEBUG_ASSERT(mnemonics_size == 1);
		index = 0;
		break;
	// ns
	case 9:
		ICED_DEBUG_ASSERT(mnemonics_size == 1);
		index = 0;
		break;
	// p, pe
	case 10:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_P_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_p());
		break;
	// np, po
	case 11:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_NP_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_np());
		break;
	// l, nge
	case 12:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_L_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_l());
		break;
	// ge, nl
	case 13:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_GE_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_ge());
		break;
	// le, ng
	case 14:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_LE_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_le());
		break;
	// g, nle
	case 15:
		ICED_DEBUG_ASSERT(mnemonics_size == IcedConstants::CC_G_ENUM_COUNT);
		index = static_cast<std::size_t>(options.cc_g());
		break;
	default:
		ICED_UNREACHABLE();
	}
	ICED_ASSERT(index < mnemonics_size);
	return index;
}

} // namespace iced_x86::internal
