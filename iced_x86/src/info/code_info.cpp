// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// code_ext functions that need the instruction info tables (Rust: #[cfg(feature = "instr_info")] parts of code.rs)

#include "iced_x86/code_ext.hpp"
#include "iced_x86/cpuid_feature.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/flow_control.hpp"
#include "iced_x86/slice.hpp"
#include "internal/info/info_flags2.hpp"
#include "internal/info/info_tables.hpp"
#include <cstddef>
#include <cstdint>

namespace iced_x86::code_ext {

using internal::InfoFlags2;

EncodingKind encoding(Code code) noexcept {
	return static_cast<EncodingKind>((internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 >> InfoFlags2::ENCODING_SHIFT) &
									 InfoFlags2::ENCODING_MASK);
}

Slice<CpuidFeature> cpuid_features(Code code) noexcept {
	const std::uint32_t index =
		(internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 >> InfoFlags2::CPUID_FEATURE_INTERNAL_SHIFT) & InfoFlags2::CPUID_FEATURE_INTERNAL_MASK;
	const internal::CpuidTableEntry& entry = internal::CPUID_TABLE[index];
	return Slice<CpuidFeature>(&internal::CPUID_FEATURES[entry.offset], entry.count);
}

FlowControl flow_control(Code code) noexcept {
	return static_cast<FlowControl>((internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 >> InfoFlags2::FLOW_CONTROL_SHIFT) &
									InfoFlags2::FLOW_CONTROL_MASK);
}

bool is_privileged(Code code) noexcept { return (internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 & InfoFlags2::PRIVILEGED) != 0; }

bool is_stack_instruction(Code code) noexcept {
	return (internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 & InfoFlags2::STACK_INSTRUCTION) != 0;
}

bool is_save_restore_instruction(Code code) noexcept {
	return (internal::INFO_TABLE[static_cast<std::size_t>(code)].flags2 & InfoFlags2::SAVE_RESTORE) != 0;
}

} // namespace iced_x86::code_ext
