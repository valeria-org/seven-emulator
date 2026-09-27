// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/instruction.hpp"
#include "iced_x86/register_ext.hpp"
#include "internal/code_internal.hpp"
#include "internal/iced_assert.hpp"
#include "internal/instruction_internal.hpp"

#include <limits>

namespace iced_x86 {

using internal::InstructionInternal;
using internal::register_from_u8;

namespace {
constexpr const char* INVALID_OPERAND = "Invalid operand";
constexpr const char* INVALID_INDEX = "Invalid index";
constexpr const char* NOT_AN_IMMEDIATE_OPERAND = "Not an immediate operand";
} // namespace

Result<void> Instruction::try_set_op4_kind(OpKind new_value) noexcept {
	if (new_value != OpKind::Immediate8)
		return IcedError("Invalid opkind");
	return {};
}

Result<OpKind> Instruction::try_op_kind(std::uint32_t operand) const noexcept {
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
	if (operand < 4)
		return op_kinds_[operand];
	if (operand == 4)
		return op4_kind();
	return IcedError(INVALID_OPERAND);
}

Result<void> Instruction::try_set_op_kind(std::uint32_t operand, OpKind op_kind) noexcept {
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
	if (operand < 4) {
		op_kinds_[operand] = op_kind;
		return {};
	}
	if (operand == 4)
		return try_set_op4_kind(op_kind);
	return IcedError(INVALID_OPERAND);
}

Result<std::uint64_t> Instruction::try_immediate(std::uint32_t operand) const noexcept {
	if (operand > 4)
		return IcedError(INVALID_OPERAND);
	std::uint64_t value;
	if (get_immediate_core(operand, value))
		return value;
	return IcedError(NOT_AN_IMMEDIATE_OPERAND);
}

Result<void> Instruction::try_set_immediate_u64(std::uint32_t operand, std::uint64_t new_value) noexcept {
	if (operand > 4)
		return IcedError(INVALID_OPERAND);
	if (set_immediate_core(operand, new_value))
		return {};
	return IcedError(NOT_AN_IMMEDIATE_OPERAND);
}

Result<void> Instruction::try_set_op4_register(Register new_value) noexcept {
	if (new_value != Register::None)
		return IcedError("Invalid register");
	return {};
}

Result<Register> Instruction::try_op_register(std::uint32_t operand) const noexcept {
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
	if (operand < 4)
		return regs_[operand];
	if (operand == 4)
		return op4_register();
	return IcedError(INVALID_OPERAND);
}

Result<void> Instruction::try_set_op_register(std::uint32_t operand, Register new_value) noexcept {
	static_assert(IcedConstants::MAX_OP_COUNT == 5, "");
	if (operand < 4) {
		regs_[operand] = new_value;
		return {};
	}
	if (operand == 4)
		return try_set_op4_register(new_value);
	return IcedError(INVALID_OPERAND);
}

void Instruction::set_declare_byte_value(std::size_t index, std::uint8_t new_value) noexcept {
	const auto result = try_set_declare_byte_value(index, new_value);
	(void)result;
	ICED_DEBUG_ASSERT(result.is_ok());
}

Result<void> Instruction::try_set_declare_byte_value(std::size_t index, std::uint8_t new_value) noexcept {
	switch (index) {
	case 0:
		regs_[0] = register_from_u8(new_value);
		break;
	case 1:
		regs_[1] = register_from_u8(new_value);
		break;
	case 2:
		regs_[2] = register_from_u8(new_value);
		break;
	case 3:
		regs_[3] = register_from_u8(new_value);
		break;
	case 4:
		immediate_ = (immediate_ & 0xFFFF'FF00U) | new_value;
		break;
	case 5:
		immediate_ = (immediate_ & 0xFFFF'00FFU) | (static_cast<std::uint32_t>(new_value) << 8);
		break;
	case 6:
		immediate_ = (immediate_ & 0xFF00'FFFFU) | (static_cast<std::uint32_t>(new_value) << 16);
		break;
	case 7:
		immediate_ = (immediate_ & 0x00FF'FFFFU) | (static_cast<std::uint32_t>(new_value) << 24);
		break;
	case 8:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'FFFF'FF00ULL) | new_value;
		break;
	case 9:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'FFFF'00FFULL) | (static_cast<std::uint64_t>(new_value) << 8);
		break;
	case 10:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'FF00'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 16);
		break;
	case 11:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'00FF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 24);
		break;
	case 12:
		mem_displ_ = (mem_displ_ & 0xFFFF'FF00'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 32);
		break;
	case 13:
		mem_displ_ = (mem_displ_ & 0xFFFF'00FF'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 40);
		break;
	case 14:
		mem_displ_ = (mem_displ_ & 0xFF00'FFFF'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 48);
		break;
	case 15:
		mem_displ_ = (mem_displ_ & 0x00FF'FFFF'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 56);
		break;
	default:
		return IcedError(INVALID_INDEX);
	}
	return {};
}

std::uint8_t Instruction::get_declare_byte_value(std::size_t index) const noexcept {
	const auto result = try_get_declare_byte_value(index);
	if (result.is_ok())
		return result.value();
	ICED_DEBUG_ASSERT(false);
	return 0;
}

Result<std::uint8_t> Instruction::try_get_declare_byte_value(std::size_t index) const noexcept {
	switch (index) {
	case 0:
		return static_cast<std::uint8_t>(regs_[0]);
	case 1:
		return static_cast<std::uint8_t>(regs_[1]);
	case 2:
		return static_cast<std::uint8_t>(regs_[2]);
	case 3:
		return static_cast<std::uint8_t>(regs_[3]);
	case 4:
		return static_cast<std::uint8_t>(immediate_);
	case 5:
		return static_cast<std::uint8_t>(immediate_ >> 8);
	case 6:
		return static_cast<std::uint8_t>(immediate_ >> 16);
	case 7:
		return static_cast<std::uint8_t>(immediate_ >> 24);
	case 8:
		return static_cast<std::uint8_t>(mem_displ_);
	case 9:
		return static_cast<std::uint8_t>(static_cast<std::uint32_t>(mem_displ_) >> 8);
	case 10:
		return static_cast<std::uint8_t>(static_cast<std::uint32_t>(mem_displ_) >> 16);
	case 11:
		return static_cast<std::uint8_t>(static_cast<std::uint32_t>(mem_displ_) >> 24);
	case 12:
		return static_cast<std::uint8_t>(mem_displ_ >> 32);
	case 13:
		return static_cast<std::uint8_t>(mem_displ_ >> 40);
	case 14:
		return static_cast<std::uint8_t>(mem_displ_ >> 48);
	case 15:
		return static_cast<std::uint8_t>(mem_displ_ >> 56);
	default:
		return IcedError(INVALID_INDEX);
	}
}

void Instruction::set_declare_word_value(std::size_t index, std::uint16_t new_value) noexcept {
	const auto result = try_set_declare_word_value(index, new_value);
	(void)result;
	ICED_DEBUG_ASSERT(result.is_ok());
}

Result<void> Instruction::try_set_declare_word_value(std::size_t index, std::uint16_t new_value) noexcept {
	switch (index) {
	case 0:
		regs_[0] = register_from_u8(static_cast<std::uint8_t>(new_value));
		regs_[1] = register_from_u8(static_cast<std::uint8_t>(new_value >> 8));
		break;
	case 1:
		regs_[2] = register_from_u8(static_cast<std::uint8_t>(new_value));
		regs_[3] = register_from_u8(static_cast<std::uint8_t>(new_value >> 8));
		break;
	case 2:
		immediate_ = (immediate_ & 0xFFFF'0000U) | new_value;
		break;
	case 3:
		immediate_ = static_cast<std::uint32_t>(static_cast<std::uint16_t>(immediate_)) | (static_cast<std::uint32_t>(new_value) << 16);
		break;
	case 4:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'FFFF'0000ULL) | new_value;
		break;
	case 5:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'0000'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 16);
		break;
	case 6:
		mem_displ_ = (mem_displ_ & 0xFFFF'0000'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 32);
		break;
	case 7:
		mem_displ_ = (mem_displ_ & 0x0000'FFFF'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 48);
		break;
	default:
		return IcedError(INVALID_INDEX);
	}
	return {};
}

std::uint16_t Instruction::get_declare_word_value(std::size_t index) const noexcept {
	const auto result = try_get_declare_word_value(index);
	if (result.is_ok())
		return result.value();
	ICED_DEBUG_ASSERT(false);
	return 0;
}

Result<std::uint16_t> Instruction::try_get_declare_word_value(std::size_t index) const noexcept {
	switch (index) {
	case 0:
		return static_cast<std::uint16_t>(static_cast<std::uint32_t>(regs_[0]) | (static_cast<std::uint32_t>(regs_[1]) << 8));
	case 1:
		return static_cast<std::uint16_t>(static_cast<std::uint32_t>(regs_[2]) | (static_cast<std::uint32_t>(regs_[3]) << 8));
	case 2:
		return static_cast<std::uint16_t>(immediate_);
	case 3:
		return static_cast<std::uint16_t>(immediate_ >> 16);
	case 4:
		return static_cast<std::uint16_t>(mem_displ_);
	case 5:
		return static_cast<std::uint16_t>(static_cast<std::uint32_t>(mem_displ_) >> 16);
	case 6:
		return static_cast<std::uint16_t>(mem_displ_ >> 32);
	case 7:
		return static_cast<std::uint16_t>(mem_displ_ >> 48);
	default:
		return IcedError(INVALID_INDEX);
	}
}

void Instruction::set_declare_dword_value(std::size_t index, std::uint32_t new_value) noexcept {
	const auto result = try_set_declare_dword_value(index, new_value);
	(void)result;
	ICED_DEBUG_ASSERT(result.is_ok());
}

Result<void> Instruction::try_set_declare_dword_value(std::size_t index, std::uint32_t new_value) noexcept {
	switch (index) {
	case 0:
		regs_[0] = register_from_u8(static_cast<std::uint8_t>(new_value));
		regs_[1] = register_from_u8(static_cast<std::uint8_t>(new_value >> 8));
		regs_[2] = register_from_u8(static_cast<std::uint8_t>(new_value >> 16));
		regs_[3] = register_from_u8(static_cast<std::uint8_t>(new_value >> 24));
		break;
	case 1:
		immediate_ = new_value;
		break;
	case 2:
		mem_displ_ = (mem_displ_ & 0xFFFF'FFFF'0000'0000ULL) | new_value;
		break;
	case 3:
		mem_displ_ = (mem_displ_ & 0x0000'0000'FFFF'FFFFULL) | (static_cast<std::uint64_t>(new_value) << 32);
		break;
	default:
		return IcedError(INVALID_INDEX);
	}
	return {};
}

std::uint32_t Instruction::get_declare_dword_value(std::size_t index) const noexcept {
	const auto result = try_get_declare_dword_value(index);
	if (result.is_ok())
		return result.value();
	ICED_DEBUG_ASSERT(false);
	return 0;
}

Result<std::uint32_t> Instruction::try_get_declare_dword_value(std::size_t index) const noexcept {
	switch (index) {
	case 0:
		return static_cast<std::uint32_t>(regs_[0]) | (static_cast<std::uint32_t>(regs_[1]) << 8) | (static_cast<std::uint32_t>(regs_[2]) << 16) |
			(static_cast<std::uint32_t>(regs_[3]) << 24);
	case 1:
		return immediate_;
	case 2:
		return static_cast<std::uint32_t>(mem_displ_);
	case 3:
		return static_cast<std::uint32_t>(mem_displ_ >> 32);
	default:
		return IcedError(INVALID_INDEX);
	}
}

void Instruction::set_declare_qword_value(std::size_t index, std::uint64_t new_value) noexcept {
	const auto result = try_set_declare_qword_value(index, new_value);
	(void)result;
	ICED_DEBUG_ASSERT(result.is_ok());
}

Result<void> Instruction::try_set_declare_qword_value(std::size_t index, std::uint64_t new_value) noexcept {
	switch (index) {
	case 0:
		regs_[0] = register_from_u8(static_cast<std::uint8_t>(new_value));
		regs_[1] = register_from_u8(static_cast<std::uint8_t>(new_value >> 8));
		regs_[2] = register_from_u8(static_cast<std::uint8_t>(new_value >> 16));
		regs_[3] = register_from_u8(static_cast<std::uint8_t>(new_value >> 24));
		immediate_ = static_cast<std::uint32_t>(new_value >> 32);
		break;
	case 1:
		mem_displ_ = new_value;
		break;
	default:
		return IcedError(INVALID_INDEX);
	}
	return {};
}

std::uint64_t Instruction::get_declare_qword_value(std::size_t index) const noexcept {
	const auto result = try_get_declare_qword_value(index);
	if (result.is_ok())
		return result.value();
	ICED_DEBUG_ASSERT(false);
	return 0;
}

Result<std::uint64_t> Instruction::try_get_declare_qword_value(std::size_t index) const noexcept {
	switch (index) {
	case 0:
		return static_cast<std::uint64_t>(regs_[0]) | (static_cast<std::uint64_t>(regs_[1]) << 8) | (static_cast<std::uint64_t>(regs_[2]) << 16) |
			(static_cast<std::uint64_t>(regs_[3]) << 24) | (static_cast<std::uint64_t>(immediate_) << 32);
	case 1:
		return mem_displ_;
	default:
		return IcedError(INVALID_INDEX);
	}
}

std::optional<bool> Instruction::vsib() const noexcept {
	switch (code_) {
	// GENERATOR-BEGIN: Vsib32
	// ⚠️This was generated by GENERATOR!🦹‍♂️
	case Code::VEX_Vpgatherdd_xmm_vm32x_xmm:
	case Code::VEX_Vpgatherdd_ymm_vm32y_ymm:
	case Code::VEX_Vpgatherdq_xmm_vm32x_xmm:
	case Code::VEX_Vpgatherdq_ymm_vm32x_ymm:
	case Code::EVEX_Vpgatherdd_xmm_k1_vm32x:
	case Code::EVEX_Vpgatherdd_ymm_k1_vm32y:
	case Code::EVEX_Vpgatherdd_zmm_k1_vm32z:
	case Code::EVEX_Vpgatherdq_xmm_k1_vm32x:
	case Code::EVEX_Vpgatherdq_ymm_k1_vm32x:
	case Code::EVEX_Vpgatherdq_zmm_k1_vm32y:
	case Code::VEX_Vgatherdps_xmm_vm32x_xmm:
	case Code::VEX_Vgatherdps_ymm_vm32y_ymm:
	case Code::VEX_Vgatherdpd_xmm_vm32x_xmm:
	case Code::VEX_Vgatherdpd_ymm_vm32x_ymm:
	case Code::EVEX_Vgatherdps_xmm_k1_vm32x:
	case Code::EVEX_Vgatherdps_ymm_k1_vm32y:
	case Code::EVEX_Vgatherdps_zmm_k1_vm32z:
	case Code::EVEX_Vgatherdpd_xmm_k1_vm32x:
	case Code::EVEX_Vgatherdpd_ymm_k1_vm32x:
	case Code::EVEX_Vgatherdpd_zmm_k1_vm32y:
	case Code::EVEX_Vpscatterdd_vm32x_k1_xmm:
	case Code::EVEX_Vpscatterdd_vm32y_k1_ymm:
	case Code::EVEX_Vpscatterdd_vm32z_k1_zmm:
	case Code::EVEX_Vpscatterdq_vm32x_k1_xmm:
	case Code::EVEX_Vpscatterdq_vm32x_k1_ymm:
	case Code::EVEX_Vpscatterdq_vm32y_k1_zmm:
	case Code::EVEX_Vscatterdps_vm32x_k1_xmm:
	case Code::EVEX_Vscatterdps_vm32y_k1_ymm:
	case Code::EVEX_Vscatterdps_vm32z_k1_zmm:
	case Code::EVEX_Vscatterdpd_vm32x_k1_xmm:
	case Code::EVEX_Vscatterdpd_vm32x_k1_ymm:
	case Code::EVEX_Vscatterdpd_vm32y_k1_zmm:
	case Code::EVEX_Vgatherpf0dps_vm32z_k1:
	case Code::EVEX_Vgatherpf0dpd_vm32y_k1:
	case Code::EVEX_Vgatherpf1dps_vm32z_k1:
	case Code::EVEX_Vgatherpf1dpd_vm32y_k1:
	case Code::EVEX_Vscatterpf0dps_vm32z_k1:
	case Code::EVEX_Vscatterpf0dpd_vm32y_k1:
	case Code::EVEX_Vscatterpf1dps_vm32z_k1:
	case Code::EVEX_Vscatterpf1dpd_vm32y_k1:
	case Code::MVEX_Vpgatherdd_zmm_k1_mvt:
	case Code::MVEX_Vpgatherdq_zmm_k1_mvt:
	case Code::MVEX_Vgatherdps_zmm_k1_mvt:
	case Code::MVEX_Vgatherdpd_zmm_k1_mvt:
	case Code::MVEX_Vpscatterdd_mvt_k1_zmm:
	case Code::MVEX_Vpscatterdq_mvt_k1_zmm:
	case Code::MVEX_Vscatterdps_mvt_k1_zmm:
	case Code::MVEX_Vscatterdpd_mvt_k1_zmm:
	case Code::MVEX_Undoc_zmm_k1_mvt_512_66_0F38_W0_B0:
	case Code::MVEX_Undoc_zmm_k1_mvt_512_66_0F38_W0_B2:
	case Code::MVEX_Undoc_zmm_k1_mvt_512_66_0F38_W0_C0:
	case Code::MVEX_Vgatherpf0hintdps_mvt_k1:
	case Code::MVEX_Vgatherpf0hintdpd_mvt_k1:
	case Code::MVEX_Vgatherpf0dps_mvt_k1:
	case Code::MVEX_Vgatherpf1dps_mvt_k1:
	case Code::MVEX_Vscatterpf0hintdps_mvt_k1:
	case Code::MVEX_Vscatterpf0hintdpd_mvt_k1:
	case Code::MVEX_Vscatterpf0dps_mvt_k1:
	case Code::MVEX_Vscatterpf1dps_mvt_k1:
	// GENERATOR-END: Vsib32
		return false;

	// GENERATOR-BEGIN: Vsib64
	// ⚠️This was generated by GENERATOR!🦹‍♂️
	case Code::VEX_Vpgatherqd_xmm_vm64x_xmm:
	case Code::VEX_Vpgatherqd_xmm_vm64y_xmm:
	case Code::VEX_Vpgatherqq_xmm_vm64x_xmm:
	case Code::VEX_Vpgatherqq_ymm_vm64y_ymm:
	case Code::EVEX_Vpgatherqd_xmm_k1_vm64x:
	case Code::EVEX_Vpgatherqd_xmm_k1_vm64y:
	case Code::EVEX_Vpgatherqd_ymm_k1_vm64z:
	case Code::EVEX_Vpgatherqq_xmm_k1_vm64x:
	case Code::EVEX_Vpgatherqq_ymm_k1_vm64y:
	case Code::EVEX_Vpgatherqq_zmm_k1_vm64z:
	case Code::VEX_Vgatherqps_xmm_vm64x_xmm:
	case Code::VEX_Vgatherqps_xmm_vm64y_xmm:
	case Code::VEX_Vgatherqpd_xmm_vm64x_xmm:
	case Code::VEX_Vgatherqpd_ymm_vm64y_ymm:
	case Code::EVEX_Vgatherqps_xmm_k1_vm64x:
	case Code::EVEX_Vgatherqps_xmm_k1_vm64y:
	case Code::EVEX_Vgatherqps_ymm_k1_vm64z:
	case Code::EVEX_Vgatherqpd_xmm_k1_vm64x:
	case Code::EVEX_Vgatherqpd_ymm_k1_vm64y:
	case Code::EVEX_Vgatherqpd_zmm_k1_vm64z:
	case Code::EVEX_Vpscatterqd_vm64x_k1_xmm:
	case Code::EVEX_Vpscatterqd_vm64y_k1_xmm:
	case Code::EVEX_Vpscatterqd_vm64z_k1_ymm:
	case Code::EVEX_Vpscatterqq_vm64x_k1_xmm:
	case Code::EVEX_Vpscatterqq_vm64y_k1_ymm:
	case Code::EVEX_Vpscatterqq_vm64z_k1_zmm:
	case Code::EVEX_Vscatterqps_vm64x_k1_xmm:
	case Code::EVEX_Vscatterqps_vm64y_k1_xmm:
	case Code::EVEX_Vscatterqps_vm64z_k1_ymm:
	case Code::EVEX_Vscatterqpd_vm64x_k1_xmm:
	case Code::EVEX_Vscatterqpd_vm64y_k1_ymm:
	case Code::EVEX_Vscatterqpd_vm64z_k1_zmm:
	case Code::EVEX_Vgatherpf0qps_vm64z_k1:
	case Code::EVEX_Vgatherpf0qpd_vm64z_k1:
	case Code::EVEX_Vgatherpf1qps_vm64z_k1:
	case Code::EVEX_Vgatherpf1qpd_vm64z_k1:
	case Code::EVEX_Vscatterpf0qps_vm64z_k1:
	case Code::EVEX_Vscatterpf0qpd_vm64z_k1:
	case Code::EVEX_Vscatterpf1qps_vm64z_k1:
	case Code::EVEX_Vscatterpf1qpd_vm64z_k1:
	// GENERATOR-END: Vsib64
		return true;

	default:
		return std::nullopt;
	}
}

namespace {
// Not inlined in debug builds so the caller (virtual_address()) doesn't need stack space for each call's temporaries
#ifdef NDEBUG
inline
#else
ICED_NOINLINE
#endif
bool get_register_value_helper(Instruction::GetRegisterValueFn get_register_value, void* context, Register register_,
	std::size_t element_index, std::size_t element_size, std::uint64_t& value) {
	const std::optional<std::uint64_t> result = get_register_value(context, register_, element_index, element_size);
	if (!result.has_value())
		return false;
	value = *result;
	return true;
}
} // namespace

std::optional<std::uint64_t> Instruction::virtual_address(std::uint32_t operand, std::size_t element_index, GetRegisterValueFn get_register_value,
	void* context) const {
	// Gets a register value or returns std::nullopt from this function
	std::uint64_t seg;
	std::uint64_t reg;
#define ICED_GET_REG_VALUE(var, register_, elem_index, elem_size) \
	if (!get_register_value_helper(get_register_value, context, (register_), (elem_index), (elem_size), var)) \
		return std::nullopt

	switch (op_kind(operand)) {
	case OpKind::Register:
	case OpKind::NearBranch16:
	case OpKind::NearBranch32:
	case OpKind::NearBranch64:
	case OpKind::FarBranch16:
	case OpKind::FarBranch32:
	case OpKind::Immediate8:
	case OpKind::Immediate8_2nd:
	case OpKind::Immediate16:
	case OpKind::Immediate32:
	case OpKind::Immediate64:
	case OpKind::Immediate8to16:
	case OpKind::Immediate8to32:
	case OpKind::Immediate8to64:
	case OpKind::Immediate32to64:
		return 0;

	case OpKind::MemorySegSI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::SI, 0, 0);
		return seg + static_cast<std::uint16_t>(reg);
	}
	case OpKind::MemorySegESI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::ESI, 0, 0);
		return seg + static_cast<std::uint32_t>(reg);
	}
	case OpKind::MemorySegRSI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::RSI, 0, 0);
		return seg + reg;
	}
	case OpKind::MemorySegDI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::DI, 0, 0);
		return seg + static_cast<std::uint16_t>(reg);
	}
	case OpKind::MemorySegEDI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::EDI, 0, 0);
		return seg + static_cast<std::uint32_t>(reg);
	}
	case OpKind::MemorySegRDI: {
		ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
		ICED_GET_REG_VALUE(reg, Register::RDI, 0, 0);
		return seg + reg;
	}
	case OpKind::MemoryESDI: {
		ICED_GET_REG_VALUE(seg, Register::ES, 0, 0);
		ICED_GET_REG_VALUE(reg, Register::DI, 0, 0);
		return seg + static_cast<std::uint16_t>(reg);
	}
	case OpKind::MemoryESEDI: {
		ICED_GET_REG_VALUE(seg, Register::ES, 0, 0);
		ICED_GET_REG_VALUE(reg, Register::EDI, 0, 0);
		return seg + static_cast<std::uint32_t>(reg);
	}
	case OpKind::MemoryESRDI: {
		ICED_GET_REG_VALUE(seg, Register::ES, 0, 0);
		ICED_GET_REG_VALUE(reg, Register::RDI, 0, 0);
		return seg + reg;
	}
	case OpKind::Memory: {
		const Register base_reg = memory_base();
		const Register index_reg = memory_index();
		const std::uint32_t addr_size = InstructionInternal::get_address_size_in_bytes(base_reg, index_reg, memory_displ_size(), code_size());
		std::uint64_t offset = memory_displacement64();
		std::uint64_t offset_mask;
		switch (addr_size) {
		case 8:
			offset_mask = std::numeric_limits<std::uint64_t>::max();
			break;
		case 4:
			offset_mask = std::numeric_limits<std::uint32_t>::max();
			break;
		default:
			ICED_DEBUG_ASSERT(addr_size == 2);
			offset_mask = std::numeric_limits<std::uint16_t>::max();
			break;
		}
		switch (base_reg) {
		case Register::None:
		case Register::EIP:
		case Register::RIP:
			break;
		default: {
			ICED_GET_REG_VALUE(reg, base_reg, 0, 0);
			offset += reg;
			break;
		}
		}
		const Code code = code_;
		if (index_reg != Register::None && !internal::code_ignores_index(code) && !internal::code_is_tile_stride_index(code)) {
			const std::uint32_t scale = InstructionInternal::internal_get_memory_index_scale(*this);
			const std::optional<bool> is_vsib64 = vsib();
			if (is_vsib64.has_value()) {
				if (*is_vsib64) {
					ICED_GET_REG_VALUE(reg, index_reg, element_index, 8);
					offset += reg << scale;
				}
				else {
					ICED_GET_REG_VALUE(reg, index_reg, element_index, 4);
					offset += static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(reg))) << scale;
				}
			}
			else {
				ICED_GET_REG_VALUE(reg, index_reg, 0, 0);
				offset += reg << scale;
			}
		}
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 1 == static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhq_zmm_k1_mt), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 2 == static_cast<std::uint32_t>(Code::MVEX_Vpackstorehd_mt_k1_zmm), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 3 == static_cast<std::uint32_t>(Code::MVEX_Vpackstorehq_mt_k1_zmm), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 4 == static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhps_zmm_k1_mt), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 5 == static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhpd_zmm_k1_mt), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 6 == static_cast<std::uint32_t>(Code::MVEX_Vpackstorehps_mt_k1_zmm), "");
		static_assert(static_cast<std::uint32_t>(Code::MVEX_Vloadunpackhd_zmm_k1_mt) + 7 == static_cast<std::uint32_t>(Code::MVEX_Vpackstorehpd_mt_k1_zmm), "");
		if (code >= Code::MVEX_Vloadunpackhd_zmm_k1_mt && code <= Code::MVEX_Vpackstorehpd_mt_k1_zmm)
			offset -= 0x40;
		offset &= offset_mask;
		if (!internal::code_ignores_segment(code)) {
			ICED_GET_REG_VALUE(seg, memory_segment(), 0, 0);
			return seg + offset;
		}
		return offset;
	}
	default:
		// Invalid op kind (it's an enum class so it can't happen unless it's an invalid value)
		ICED_UNREACHABLE();
	}
#undef ICED_GET_REG_VALUE
}

} // namespace iced_x86
