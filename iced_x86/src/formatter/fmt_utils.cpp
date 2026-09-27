// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/formatter/fmt_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace iced_x86::internal {

static constexpr std::string_view SPACES_TABLE[12] = {
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
	"           ",
	"            ",
};

static constexpr std::string_view TABS_TABLE[4] = {
	"\t",
	"\t\t",
	"\t\t\t",
	"\t\t\t\t",
};

template <std::size_t N>
static void add_strings(FormatterOutput& output, const std::string_view (&strings)[N], std::uint32_t count) {
	std::size_t count2 = count;
	while (count2 > 0) {
		const std::size_t n = std::min(count2, N);
		output.write(strings[n - 1], FormatterTextKind::Text);
		count2 -= n;
	}
}

void add_tabs(FormatterOutput& output, std::uint32_t column, std::uint32_t first_operand_char_index, std::uint32_t tab_size) {
	constexpr std::uint32_t MAX_FIRST_OPERAND_CHAR_INDEX = 256;
	first_operand_char_index = std::min(first_operand_char_index, MAX_FIRST_OPERAND_CHAR_INDEX);

	if (tab_size == 0) {
		const std::uint32_t chars_left = first_operand_char_index <= column ? 1 : first_operand_char_index - column;
		add_strings(output, SPACES_TABLE, chars_left);
	}
	else {
		const std::uint32_t end_col = first_operand_char_index <= column ? column + 1 : first_operand_char_index;
		const std::uint32_t end_col_rounded_down = end_col / tab_size * tab_size;
		const bool added_tabs = end_col_rounded_down > column;
		if (added_tabs) {
			const std::uint32_t tabs = (end_col_rounded_down - (column / tab_size * tab_size)) / tab_size;
			add_strings(output, TABS_TABLE, tabs);
			column = end_col_rounded_down;
		}
		if (first_operand_char_index > column)
			add_strings(output, SPACES_TABLE, first_operand_char_index - column);
		else if (!added_tabs)
			add_strings(output, SPACES_TABLE, 1);
	}
}

} // namespace iced_x86::internal
