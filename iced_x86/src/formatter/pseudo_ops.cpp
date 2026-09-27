// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/formatter/pseudo_ops.hpp"

#include <cstddef>
#include <cstdint>

#include "internal/formatter/pseudo_ops_defs.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

namespace {
using pseudo_ops_defs::PSEUDO_OPS_DEFS;
using pseudo_ops_defs::PSEUDO_OPS_KIND_COUNT;

// Total number of pseudo op strings
constexpr std::size_t get_strings_count() noexcept {
	std::size_t count = 0;
	for (const auto& def : PSEUDO_OPS_DEFS)
		count += def.size;
	return count;
}

// Size of the FormatterString data of all pseudo op strings
constexpr std::size_t get_strings_size() noexcept {
	std::size_t size = 0;
	for (const auto& def : PSEUDO_OPS_DEFS) {
		for (std::size_t i = 0; i < def.size; i++)
			size += 1 + 2 * (def.prefix.size() + def.cc[i].size() + def.suffix.size());
	}
	return size;
}

constexpr std::size_t STRINGS_COUNT = get_strings_count();
constexpr std::size_t STRINGS_SIZE = get_strings_size();
static_assert(STRINGS_SIZE <= 0xFFFF, "");

// All pseudo op strings (FormatterString data), created at compile time (it's small)
struct PseudoOpsData {
	char strings[STRINGS_SIZE];
	std::uint16_t offsets[STRINGS_COUNT];
	// Index of the first string of each `PseudoOpsKind` in `offsets`
	std::uint16_t first[PSEUDO_OPS_KIND_COUNT];
	// Number of strings of each `PseudoOpsKind`
	std::uint8_t sizes[PSEUDO_OPS_KIND_COUNT];

	constexpr PseudoOpsData() noexcept : strings{}, offsets{}, first{}, sizes{} {
		std::size_t offset = 0;
		std::size_t index = 0;
		for (const auto& def : PSEUDO_OPS_DEFS) {
			first[static_cast<std::size_t>(def.kind)] = static_cast<std::uint16_t>(index);
			sizes[static_cast<std::size_t>(def.kind)] = static_cast<std::uint8_t>(def.size);
			for (std::size_t i = 0; i < def.size; i++) {
				const std::string_view parts[3] = {def.prefix, def.cc[i], def.suffix};
				const std::size_t len = parts[0].size() + parts[1].size() + parts[2].size();
				offsets[index++] = static_cast<std::uint16_t>(offset);
				strings[offset] = static_cast<char>(len);
				std::size_t pos = offset + 1;
				for (const auto& part : parts) {
					for (const char c : part) {
						strings[pos] = c;
						strings[pos + len] = formatter_string_to_upper(c);
						pos++;
					}
				}
				offset += 1 + 2 * len;
			}
		}
	}
};

constexpr PseudoOpsData DATA{};
} // namespace

PseudoOps get_pseudo_ops(PseudoOpsKind kind) noexcept {
	const auto index = static_cast<std::size_t>(kind);
	ICED_ASSERT(index < PSEUDO_OPS_KIND_COUNT);
	return PseudoOps(DATA.strings, DATA.offsets + DATA.first[index], DATA.sizes[index]);
}

} // namespace iced_x86::internal
