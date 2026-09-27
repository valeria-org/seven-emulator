// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/instruction_info.hpp"
#include "internal/iced_assert.hpp"
#include <cstdio>
#include <string>

namespace iced_x86 {

OpAccess InstructionInfo::op_access(std::uint32_t operand) const noexcept {
	if (operand < op_accesses_.size())
		return op_accesses_[operand];
	ICED_DEBUG_ASSERT(false);
	return OpAccess::None;
}

Result<OpAccess> InstructionInfo::try_op_access(std::uint32_t operand) const {
	if (operand < op_accesses_.size())
		return op_accesses_[operand];
	return IcedError("Invalid operand");
}

std::string to_string(const UsedRegister& value) {
	std::string s;
	s.append(to_string(value.register_()));
	s.push_back(':');
	s.append(to_string(value.access()));
	return s;
}

std::string to_string(const UsedMemory& value) {
	std::string s;
	s.push_back('[');
	s.append(to_string(value.segment()));
	s.push_back(':');
	bool need_plus = false;
	if (value.base() != Register::None) {
		s.append(to_string(value.base()));
		need_plus = true;
	}
	if (value.index() != Register::None) {
		if (need_plus)
			s.push_back('+');
		need_plus = true;
		s.append(to_string(value.index()));
		if (value.scale() != 1) {
			s.push_back('*');
			s.append(std::to_string(value.scale()));
		}
	}
	if (value.displacement() != 0 || !need_plus) {
		if (need_plus)
			s.push_back('+');
		if (value.displacement() <= 9)
			s.append(std::to_string(value.displacement()));
		else {
			char buf[2 + 16 + 1];
			std::snprintf(buf, sizeof(buf), "0x%llX", static_cast<unsigned long long>(value.displacement()));
			s.append(buf);
		}
	}
	s.push_back(';');
	s.append(to_string(value.memory_size()));
	s.push_back(';');
	s.append(to_string(value.access()));
	s.push_back(']');
	return s;
}

} // namespace iced_x86
