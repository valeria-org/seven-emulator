// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/cpuid_feature.hpp"
#include "iced_x86/iced_constants.hpp"
#include "internal/info/instr_info_constants.hpp"
#include <cstdint>

namespace iced_x86::internal {

// One entry per `Code` value (see `InfoFlags1` and `InfoFlags2`)
struct InfoTableEntry {
	std::uint32_t flags1; // InfoFlags1
	std::uint32_t flags2; // InfoFlags2
};

// info_table.cpp (generated)
extern const InfoTableEntry INFO_TABLE[IcedConstants::CODE_ENUM_COUNT];

// rflags_table.cpp (generated). Index = RflagsInfo
extern const std::uint16_t FLAGS_READ[InstrInfoConstants::RFLAGS_INFO_COUNT];
extern const std::uint16_t FLAGS_UNDEFINED[InstrInfoConstants::RFLAGS_INFO_COUNT];
extern const std::uint16_t FLAGS_WRITTEN[InstrInfoConstants::RFLAGS_INFO_COUNT];
extern const std::uint16_t FLAGS_CLEARED[InstrInfoConstants::RFLAGS_INFO_COUNT];
extern const std::uint16_t FLAGS_SET[InstrInfoConstants::RFLAGS_INFO_COUNT];
extern const std::uint16_t FLAGS_MODIFIED[InstrInfoConstants::RFLAGS_INFO_COUNT];

// cpuid_table.cpp (generated). Index = CpuidFeatureInternal. The features are stored in `CPUID_FEATURES[offset..offset+count]`
struct CpuidTableEntry {
	std::uint16_t offset;
	std::uint8_t count;
};
extern const CpuidFeature CPUID_FEATURES[];
extern const CpuidTableEntry CPUID_TABLE[];

} // namespace iced_x86::internal
