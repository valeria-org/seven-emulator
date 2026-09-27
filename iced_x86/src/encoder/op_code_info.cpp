// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/op_code_info.hpp"
#include "iced_x86/code_ext.hpp"
#include "iced_x86/instruction.hpp"
#include "internal/encoder/dec_option_value.hpp"
#include "internal/encoder/enc_flags2.hpp"
#include "internal/encoder/enc_flags3.hpp"
#include "internal/encoder/op_code_info_flags1.hpp"
#include "internal/encoder/op_code_info_flags2.hpp"
#include "internal/encoder/op_code_info_internal.hpp"
#include "internal/encoder/to_decoder_options.hpp"
#include "internal/iced_assert.hpp"
#include "internal/mvex/mvex.hpp"

namespace iced_x86 {

using internal::EncFlags2;
using internal::EncFlags3;
using internal::OpCodeInfoFlags1;
using internal::OpCodeInfoFlags2;

static_assert(IcedConstants::MAX_OP_COUNT == 5, "The OpCodeInfo constructor assumes there are 5 operands");

OpCodeOperandKind OpCodeInfo::invalid_operand_op_kind() noexcept {
	ICED_DEBUG_ASSERT(false); // Invalid operand
	return OpCodeOperandKind::None;
}

std::string_view OpCodeInfo::op_code_string() const noexcept {
	return std::string_view(internal::OpCodeInfoInternal::get_strings(*this), op_code_string_len_);
}

std::string_view OpCodeInfo::instruction_string() const noexcept {
	return std::string_view(internal::OpCodeInfoInternal::get_strings(*this) + op_code_string_len_, instruction_string_len_);
}

Mnemonic OpCodeInfo::mnemonic() const noexcept { return code_ext::mnemonic(code_); }

MvexEHBit OpCodeInfo::mvex_eh_bit() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).eh_bit;
	return MvexEHBit::None;
}

bool OpCodeInfo::mvex_can_use_eviction_hint() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).can_use_eviction_hint();
	return false;
}

bool OpCodeInfo::mvex_can_use_imm_rounding_control() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).can_use_imm_rounding_control();
	return false;
}

bool OpCodeInfo::mvex_ignores_op_mask_register() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).ignores_op_mask_register();
	return false;
}

bool OpCodeInfo::mvex_no_sae_rc() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).no_sae_rc();
	return false;
}

MvexTupleTypeLutKind OpCodeInfo::mvex_tuple_type_lut_kind() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).tuple_type_lut_kind;
	return static_cast<MvexTupleTypeLutKind>(0);
}

MvexConvFn OpCodeInfo::mvex_conversion_func() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return internal::get_mvex_info(code()).conv_fn;
	return MvexConvFn::None;
}

std::uint8_t OpCodeInfo::mvex_valid_conversion_funcs_mask() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return static_cast<std::uint8_t>(~internal::get_mvex_info(code()).invalid_conv_fns);
	return 0;
}

std::uint8_t OpCodeInfo::mvex_valid_swizzle_funcs_mask() const noexcept {
	if (encoding() == EncodingKind::MVEX)
		return static_cast<std::uint8_t>(~internal::get_mvex_info(code()).invalid_swizzle_fns);
	return 0;
}

MemorySize OpCodeInfo::memory_size() const noexcept { return internal::SIZES_NORMAL[static_cast<std::size_t>(code())]; }

MemorySize OpCodeInfo::broadcast_memory_size() const noexcept { return internal::SIZES_BCST[static_cast<std::size_t>(code())]; }

std::uint32_t OpCodeInfo::decoder_option() const noexcept {
	const std::uint32_t dec_opt_value = (opc_flags1_ >> OpCodeInfoFlags1::DEC_OPTION_VALUE_SHIFT) & OpCodeInfoFlags1::DEC_OPTION_VALUE_MASK;
	return internal::TO_DECODER_OPTIONS[dec_opt_value];
}

std::uint32_t OpCodeInfo::op_code_len() const noexcept { return (enc_flags2_ & EncFlags2::OP_CODE_IS2_BYTES) != 0 ? 2 : 1; }

std::uint32_t OpCodeInfo::op_count() const noexcept { return internal::OP_COUNT[static_cast<std::size_t>(code())]; }

Result<OpCodeOperandKind> OpCodeInfo::try_op_kind(std::uint32_t operand) const {
	if (operand < MAX_OP_COUNT)
		return op_kinds_[operand];
	return IcedError("Invalid operand");
}

bool OpCodeInfo::is_available_in_mode(std::uint32_t bitness) const noexcept {
	switch (bitness) {
	case 16:
		return mode16();
	case 32:
		return mode32();
	case 64:
		return mode64();
	default:
		return false;
	}
}

bool OpCodeInfo::mode16() const noexcept { return (enc_flags3_ & EncFlags3::BIT16OR32) != 0; }
bool OpCodeInfo::mode32() const noexcept { return (enc_flags3_ & EncFlags3::BIT16OR32) != 0; }
bool OpCodeInfo::mode64() const noexcept { return (enc_flags3_ & EncFlags3::BIT64) != 0; }
bool OpCodeInfo::fwait() const noexcept { return (enc_flags3_ & EncFlags3::FWAIT) != 0; }
bool OpCodeInfo::can_broadcast() const noexcept { return (enc_flags3_ & EncFlags3::BROADCAST) != 0; }
bool OpCodeInfo::can_use_rounding_control() const noexcept { return (enc_flags3_ & EncFlags3::ROUNDING_CONTROL) != 0; }
bool OpCodeInfo::can_suppress_all_exceptions() const noexcept { return (enc_flags3_ & EncFlags3::SUPPRESS_ALL_EXCEPTIONS) != 0; }
bool OpCodeInfo::can_use_op_mask_register() const noexcept { return (enc_flags3_ & EncFlags3::OP_MASK_REGISTER) != 0; }
bool OpCodeInfo::require_op_mask_register() const noexcept { return (enc_flags3_ & EncFlags3::REQUIRE_OP_MASK_REGISTER) != 0; }
bool OpCodeInfo::can_use_zeroing_masking() const noexcept { return (enc_flags3_ & EncFlags3::ZEROING_MASKING) != 0; }
bool OpCodeInfo::can_use_lock_prefix() const noexcept { return (enc_flags3_ & EncFlags3::LOCK) != 0; }
bool OpCodeInfo::can_use_xacquire_prefix() const noexcept { return (enc_flags3_ & EncFlags3::XACQUIRE) != 0; }
bool OpCodeInfo::can_use_xrelease_prefix() const noexcept { return (enc_flags3_ & EncFlags3::XRELEASE) != 0; }
bool OpCodeInfo::can_use_rep_prefix() const noexcept { return (enc_flags3_ & EncFlags3::REP) != 0; }
bool OpCodeInfo::can_use_repne_prefix() const noexcept { return (enc_flags3_ & EncFlags3::REPNE) != 0; }
bool OpCodeInfo::can_use_bnd_prefix() const noexcept { return (enc_flags3_ & EncFlags3::BND) != 0; }
bool OpCodeInfo::can_use_hint_taken_prefix() const noexcept { return (enc_flags3_ & EncFlags3::HINT_TAKEN) != 0; }
bool OpCodeInfo::can_use_notrack_prefix() const noexcept { return (enc_flags3_ & EncFlags3::NOTRACK) != 0; }
bool OpCodeInfo::default_op_size64() const noexcept { return (enc_flags3_ & EncFlags3::DEFAULT_OP_SIZE64) != 0; }
bool OpCodeInfo::force_op_size64() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::FORCE_OP_SIZE64) != 0; }
bool OpCodeInfo::intel_force_op_size64() const noexcept { return (enc_flags3_ & EncFlags3::INTEL_FORCE_OP_SIZE64) != 0; }
bool OpCodeInfo::is_input_output() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::INPUT_OUTPUT) != 0; }
bool OpCodeInfo::is_nop() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::NOP) != 0; }
bool OpCodeInfo::is_reserved_nop() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::RESERVED_NOP) != 0; }
bool OpCodeInfo::is_serializing_intel() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::SERIALIZING_INTEL) != 0; }
bool OpCodeInfo::is_serializing_amd() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::SERIALIZING_AMD) != 0; }
bool OpCodeInfo::may_require_cpl0() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::MAY_REQUIRE_CPL0) != 0; }
bool OpCodeInfo::is_cet_tracked() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::CET_TRACKED) != 0; }
bool OpCodeInfo::is_non_temporal() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::NON_TEMPORAL) != 0; }
bool OpCodeInfo::is_fpu_no_wait() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::FPU_NO_WAIT) != 0; }
bool OpCodeInfo::ignores_mod_bits() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::IGNORES_MOD_BITS) != 0; }
bool OpCodeInfo::no66() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::NO66) != 0; }
bool OpCodeInfo::nfx() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::NFX) != 0; }
bool OpCodeInfo::requires_unique_reg_nums() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::REQUIRES_UNIQUE_REG_NUMS) != 0; }
bool OpCodeInfo::requires_unique_dest_reg_num() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::REQUIRES_UNIQUE_DEST_REG_NUM) != 0; }
bool OpCodeInfo::is_privileged() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::PRIVILEGED) != 0; }
bool OpCodeInfo::is_save_restore() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::SAVE_RESTORE) != 0; }
bool OpCodeInfo::is_stack_instruction() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::STACK_INSTRUCTION) != 0; }
bool OpCodeInfo::ignores_segment() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::IGNORES_SEGMENT) != 0; }
bool OpCodeInfo::is_op_mask_read_write() const noexcept { return (opc_flags1_ & OpCodeInfoFlags1::OP_MASK_READ_WRITE) != 0; }
bool OpCodeInfo::real_mode() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::REAL_MODE) != 0; }
bool OpCodeInfo::protected_mode() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::PROTECTED_MODE) != 0; }
bool OpCodeInfo::virtual8086_mode() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::VIRTUAL8086_MODE) != 0; }
bool OpCodeInfo::compatibility_mode() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::COMPATIBILITY_MODE) != 0; }
bool OpCodeInfo::long_mode() const noexcept { return (enc_flags3_ & EncFlags3::BIT64) != 0; }
bool OpCodeInfo::use_outside_smm() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_OUTSIDE_SMM) != 0; }
bool OpCodeInfo::use_in_smm() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_SMM) != 0; }
bool OpCodeInfo::use_outside_enclave_sgx() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_OUTSIDE_ENCLAVE_SGX) != 0; }
bool OpCodeInfo::use_in_enclave_sgx1() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_ENCLAVE_SGX1) != 0; }
bool OpCodeInfo::use_in_enclave_sgx2() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_ENCLAVE_SGX2) != 0; }
bool OpCodeInfo::use_outside_vmx_op() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_OUTSIDE_VMX_OP) != 0; }
bool OpCodeInfo::use_in_vmx_root_op() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_VMX_ROOT_OP) != 0; }
bool OpCodeInfo::use_in_vmx_non_root_op() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_VMX_NON_ROOT_OP) != 0; }
bool OpCodeInfo::use_outside_seam() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_OUTSIDE_SEAM) != 0; }
bool OpCodeInfo::use_in_seam() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::USE_IN_SEAM) != 0; }
bool OpCodeInfo::tdx_non_root_gen_ud() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TDX_NON_ROOT_GEN_UD) != 0; }
bool OpCodeInfo::tdx_non_root_gen_ve() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TDX_NON_ROOT_GEN_VE) != 0; }
bool OpCodeInfo::tdx_non_root_may_gen_ex() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TDX_NON_ROOT_MAY_GEN_EX) != 0; }
bool OpCodeInfo::intel_vm_exit() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_VM_EXIT) != 0; }
bool OpCodeInfo::intel_may_vm_exit() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_MAY_VM_EXIT) != 0; }
bool OpCodeInfo::intel_smm_vm_exit() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_SMM_VM_EXIT) != 0; }
bool OpCodeInfo::amd_vm_exit() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::AMD_VM_EXIT) != 0; }
bool OpCodeInfo::amd_may_vm_exit() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::AMD_MAY_VM_EXIT) != 0; }
bool OpCodeInfo::tsx_abort() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TSX_ABORT) != 0; }
bool OpCodeInfo::tsx_impl_abort() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TSX_IMPL_ABORT) != 0; }
bool OpCodeInfo::tsx_may_abort() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::TSX_MAY_ABORT) != 0; }
bool OpCodeInfo::intel_decoder16() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_DECODER16OR32) != 0; }
bool OpCodeInfo::intel_decoder32() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_DECODER16OR32) != 0; }
bool OpCodeInfo::intel_decoder64() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::INTEL_DECODER64) != 0; }
bool OpCodeInfo::amd_decoder16() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::AMD_DECODER16OR32) != 0; }
bool OpCodeInfo::amd_decoder32() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::AMD_DECODER16OR32) != 0; }
bool OpCodeInfo::amd_decoder64() const noexcept { return (opc_flags2_ & OpCodeInfoFlags2::AMD_DECODER64) != 0; }

namespace code_ext {
const OpCodeInfo& op_code(Code code) noexcept { return internal::OpCodeInfoInternal::TABLE[static_cast<std::size_t>(code)]; }
} // namespace code_ext

} // namespace iced_x86
