// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code.hpp"
#include "iced_x86/code_ext.hpp"
#include "iced_x86/encoding_kind.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/iced_error.hpp"
#include "iced_x86/mandatory_prefix.hpp"
#include "iced_x86/memory_size.hpp"
#include "iced_x86/mnemonic.hpp"
#include "iced_x86/mvex_conv_fn.hpp"
#include "iced_x86/mvex_eh_bit.hpp"
#include "iced_x86/mvex_tuple_type_lut_kind.hpp"
#include "iced_x86/op_code_operand_kind.hpp"
#include "iced_x86/op_code_table_kind.hpp"
#include "iced_x86/slice.hpp"
#include "iced_x86/tuple_type.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iced_x86 {

namespace internal {
struct OpCodeInfoInternal;
} // namespace internal

/// Opcode info, returned by `code_ext::op_code()` and `Instruction::op_code()`
class OpCodeInfo {
public:
	/// Gets the code
	///
	/// # Examples
	///
	/// ```cpp
	/// const OpCodeInfo& op_code = code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// assert(op_code.code() == Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// ```
	Code code() const noexcept { return code_; }

	/// Gets the mnemonic
	///
	/// # Examples
	///
	/// ```cpp
	/// const OpCodeInfo& op_code = code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// assert(op_code.mnemonic() == Mnemonic::Vmovapd);
	/// ```
	Mnemonic mnemonic() const noexcept;

	/// Gets the encoding
	///
	/// # Examples
	///
	/// ```cpp
	/// const OpCodeInfo& op_code = code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// assert(op_code.encoding() == EncodingKind::EVEX);
	/// ```
	EncodingKind encoding() const noexcept { return encoding_; }

	/// `true` if it's an instruction, `false` if it's eg. `Code::INVALID`, `db`, `dw`, `dd`, `dq`, `zero_bytes`
	///
	/// # Examples
	///
	/// ```cpp
	/// assert(code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256).is_instruction());
	/// assert(!code_ext::op_code(Code::INVALID).is_instruction());
	/// assert(!code_ext::op_code(Code::DeclareByte).is_instruction());
	/// ```
	bool is_instruction() const noexcept { return !(code_ <= Code::DeclareQword || code_ == Code::Zero_bytes); }

	/// `true` if it's an instruction available in 16-bit mode
	bool mode16() const noexcept;

	/// `true` if it's an instruction available in 32-bit mode
	bool mode32() const noexcept;

	/// `true` if it's an instruction available in 64-bit mode
	bool mode64() const noexcept;

	/// `true` if an `FWAIT` (`9B`) instruction is added before the instruction
	bool fwait() const noexcept;

	/// (Legacy encoding) Gets the required operand size (16,32,64) or 0
	std::uint32_t operand_size() const noexcept { return static_cast<std::uint32_t>(operand_size_); }

	/// (Legacy encoding) Gets the required address size (16,32,64) or 0
	std::uint32_t address_size() const noexcept { return static_cast<std::uint32_t>(address_size_); }

	/// (VEX/XOP/EVEX) `L` / `L'L` value or default value if `is_lig()` is `true`
	std::uint32_t l() const noexcept { return static_cast<std::uint32_t>(l_); }

	/// (VEX/XOP/EVEX/MVEX) `W` value or default value if `is_wig()` or `is_wig32()` is `true`
	std::uint32_t w() const noexcept { return (flags_ & Flags::W) != 0 ? 1 : 0; }

	/// (VEX/XOP/EVEX) `true` if the `L` / `L'L` fields are ignored.
	///
	/// EVEX: if reg-only ops and `{er}` (`EVEX.b` is set), `L'L` is the rounding control and not ignored.
	bool is_lig() const noexcept { return (flags_ & Flags::LIG) != 0; }

	/// (VEX/XOP/EVEX/MVEX) `true` if the `W` field is ignored in 16/32/64-bit modes
	bool is_wig() const noexcept { return (flags_ & Flags::WIG) != 0; }

	/// (VEX/XOP/EVEX/MVEX) `true` if the `W` field is ignored in 16/32-bit modes (but not 64-bit mode)
	bool is_wig32() const noexcept { return (flags_ & Flags::WIG32) != 0; }

	/// (EVEX/MVEX) Gets the tuple type
	TupleType tuple_type() const noexcept { return tuple_type_; }

	/// (MVEX) Gets the `EH` bit that's required to encode this instruction
	MvexEHBit mvex_eh_bit() const noexcept;

	/// (MVEX) `true` if the instruction supports eviction hint (if it has a memory operand)
	bool mvex_can_use_eviction_hint() const noexcept;

	/// (MVEX) `true` if the instruction's rounding control bits are stored in `imm8[1:0]`
	bool mvex_can_use_imm_rounding_control() const noexcept;

	/// (MVEX) `true` if the instruction ignores op mask registers (eg. `{k1}`)
	bool mvex_ignores_op_mask_register() const noexcept;

	/// (MVEX) `true` if the instruction must have `MVEX.SSS=000` if `MVEX.EH=1`
	bool mvex_no_sae_rc() const noexcept;

	/// (MVEX) Gets the tuple type / conv lut kind
	MvexTupleTypeLutKind mvex_tuple_type_lut_kind() const noexcept;

	/// (MVEX) Gets the conversion function, eg. `Sf32`
	MvexConvFn mvex_conversion_func() const noexcept;

	/// (MVEX) Gets flags indicating which conversion functions are valid (bit 0 == func 0)
	std::uint8_t mvex_valid_conversion_funcs_mask() const noexcept;

	/// (MVEX) Gets flags indicating which swizzle functions are valid (bit 0 == func 0)
	std::uint8_t mvex_valid_swizzle_funcs_mask() const noexcept;

	/// If it has a memory operand, gets the `MemorySize` (non-broadcast memory type)
	MemorySize memory_size() const noexcept;

	/// If it has a memory operand, gets the `MemorySize` (broadcast memory type)
	MemorySize broadcast_memory_size() const noexcept;

	/// (EVEX) `true` if the instruction supports broadcasting (`EVEX.b` bit) (if it has a memory operand)
	bool can_broadcast() const noexcept;

	/// (EVEX/MVEX) `true` if the instruction supports rounding control
	bool can_use_rounding_control() const noexcept;

	/// (EVEX/MVEX) `true` if the instruction supports suppress all exceptions
	bool can_suppress_all_exceptions() const noexcept;

	/// (EVEX/MVEX) `true` if an opmask register can be used
	bool can_use_op_mask_register() const noexcept;

	/// (EVEX/MVEX) `true` if a non-zero opmask register must be used
	bool require_op_mask_register() const noexcept;

	/// (EVEX) `true` if the instruction supports zeroing masking (if one of the opmask registers `K1`-`K7` is used and destination operand is not a memory operand)
	bool can_use_zeroing_masking() const noexcept;

	/// `true` if the `LOCK` (`F0`) prefix can be used
	bool can_use_lock_prefix() const noexcept;

	/// `true` if the `XACQUIRE` (`F2`) prefix can be used
	bool can_use_xacquire_prefix() const noexcept;

	/// `true` if the `XRELEASE` (`F3`) prefix can be used
	bool can_use_xrelease_prefix() const noexcept;

	/// `true` if the `REP` / `REPE` (`F3`) prefixes can be used
	bool can_use_rep_prefix() const noexcept;

	/// `true` if the `REPNE` (`F2`) prefix can be used
	bool can_use_repne_prefix() const noexcept;

	/// `true` if the `BND` (`F2`) prefix can be used
	bool can_use_bnd_prefix() const noexcept;

	/// `true` if the `HINT-TAKEN` (`3E`) and `HINT-NOT-TAKEN` (`2E`) prefixes can be used
	bool can_use_hint_taken_prefix() const noexcept;

	/// `true` if the `NOTRACK` (`3E`) prefix can be used
	bool can_use_notrack_prefix() const noexcept;

	/// `true` if rounding control is ignored (#UD is not generated)
	bool ignores_rounding_control() const noexcept { return (flags_ & Flags::IGNORES_ROUNDING_CONTROL) != 0; }

	/// `true` if the `LOCK` prefix can be used as an extra register bit (bit 3) to access registers 8-15 without a `REX` prefix (eg. in 32-bit mode)
	bool amd_lock_reg_bit() const noexcept { return (flags_ & Flags::AMD_LOCK_REG_BIT) != 0; }

	/// `true` if the default operand size is 64 in 64-bit mode. A `66` prefix can switch to 16-bit operand size.
	bool default_op_size64() const noexcept;

	/// `true` if the operand size is always 64 in 64-bit mode. A `66` prefix is ignored.
	bool force_op_size64() const noexcept;

	/// `true` if the Intel decoder forces 64-bit operand size. A `66` prefix is ignored.
	bool intel_force_op_size64() const noexcept;

	/// `true` if it can only be executed when CPL=0
	bool must_be_cpl0() const noexcept { return (flags_ & (Flags::CPL0 | Flags::CPL1 | Flags::CPL2 | Flags::CPL3)) == Flags::CPL0; }

	/// `true` if it can be executed when CPL=0
	bool cpl0() const noexcept { return (flags_ & Flags::CPL0) != 0; }

	/// `true` if it can be executed when CPL=1
	bool cpl1() const noexcept { return (flags_ & Flags::CPL1) != 0; }

	/// `true` if it can be executed when CPL=2
	bool cpl2() const noexcept { return (flags_ & Flags::CPL2) != 0; }

	/// `true` if it can be executed when CPL=3
	bool cpl3() const noexcept { return (flags_ & Flags::CPL3) != 0; }

	/// `true` if the instruction accesses the I/O address space (eg. `IN`, `OUT`, `INS`, `OUTS`)
	bool is_input_output() const noexcept;

	/// `true` if it's one of the many nop instructions (does not include FPU nop instructions, eg. `FNOP`)
	bool is_nop() const noexcept;

	/// `true` if it's one of the many reserved nop instructions (eg. `0F0D`, `0F18-0F1F`)
	bool is_reserved_nop() const noexcept;

	/// `true` if it's a serializing instruction (Intel CPUs)
	bool is_serializing_intel() const noexcept;

	/// `true` if it's a serializing instruction (AMD CPUs)
	bool is_serializing_amd() const noexcept;

	/// `true` if the instruction requires either CPL=0 or CPL<=3 depending on some CPU option (eg. `CR4.TSD`, `CR4.PCE`, `CR4.UMIP`)
	bool may_require_cpl0() const noexcept;

	/// `true` if it's a tracked `JMP`/`CALL` indirect instruction (CET)
	bool is_cet_tracked() const noexcept;

	/// `true` if it's a non-temporal hint memory access (eg. `MOVNTDQ`)
	bool is_non_temporal() const noexcept;

	/// `true` if it's a no-wait FPU instruction, eg. `FNINIT`
	bool is_fpu_no_wait() const noexcept;

	/// `true` if the mod bits are ignored and it's assumed `modrm[7:6] == 11b`
	bool ignores_mod_bits() const noexcept;

	/// `true` if the `66` prefix is not allowed (it will #UD)
	bool no66() const noexcept;

	/// `true` if the `F2`/`F3` prefixes aren't allowed
	bool nfx() const noexcept;

	/// `true` if the index reg's reg-num (vsib op) (if any) and register ops' reg-nums must be unique,
	/// eg. `MNEMONIC XMM1,YMM1,[RAX+ZMM1*2]` is invalid. Registers = `XMM`/`YMM`/`ZMM`/`TMM`.
	bool requires_unique_reg_nums() const noexcept;

	/// `true` if the destination register's reg-num must not be present in any other operand, eg. `MNEMONIC XMM1,YMM1,[RAX+ZMM1*2]`
	/// is invalid. Registers = `XMM`/`YMM`/`ZMM`/`TMM`.
	bool requires_unique_dest_reg_num() const noexcept;

	/// `true` if it's a privileged instruction (all CPL=0 instructions (except `VMCALL`) and IOPL instructions `IN`, `INS`, `OUT`, `OUTS`, `CLI`, `STI`)
	bool is_privileged() const noexcept;

	/// `true` if it reads/writes too many registers
	bool is_save_restore() const noexcept;

	/// `true` if it's an instruction that implicitly uses the stack register, eg. `CALL`, `POP`, etc
	bool is_stack_instruction() const noexcept;

	/// `true` if the instruction doesn't read the segment register if it uses a memory operand
	bool ignores_segment() const noexcept;

	/// `true` if the opmask register is read and written (instead of just read). This also implies that it can't be `K0`.
	bool is_op_mask_read_write() const noexcept;

	/// `true` if it can be executed in real mode
	bool real_mode() const noexcept;

	/// `true` if it can be executed in protected mode
	bool protected_mode() const noexcept;

	/// `true` if it can be executed in virtual 8086 mode
	bool virtual8086_mode() const noexcept;

	/// `true` if it can be executed in compatibility mode
	bool compatibility_mode() const noexcept;

	/// `true` if it can be executed in 64-bit mode
	bool long_mode() const noexcept;

	/// `true` if it can be used outside SMM
	bool use_outside_smm() const noexcept;

	/// `true` if it can be used in SMM
	bool use_in_smm() const noexcept;

	/// `true` if it can be used outside an enclave (SGX)
	bool use_outside_enclave_sgx() const noexcept;

	/// `true` if it can be used inside an enclave (SGX1)
	bool use_in_enclave_sgx1() const noexcept;

	/// `true` if it can be used inside an enclave (SGX2)
	bool use_in_enclave_sgx2() const noexcept;

	/// `true` if it can be used outside VMX operation
	bool use_outside_vmx_op() const noexcept;

	/// `true` if it can be used in VMX root operation
	bool use_in_vmx_root_op() const noexcept;

	/// `true` if it can be used in VMX non-root operation
	bool use_in_vmx_non_root_op() const noexcept;

	/// `true` if it can be used outside SEAM
	bool use_outside_seam() const noexcept;

	/// `true` if it can be used in SEAM
	bool use_in_seam() const noexcept;

	/// `true` if #UD is generated in TDX non-root operation
	bool tdx_non_root_gen_ud() const noexcept;

	/// `true` if #VE is generated in TDX non-root operation
	bool tdx_non_root_gen_ve() const noexcept;

	/// `true` if an exception (eg. #GP(0), #VE) may be generated in TDX non-root operation
	bool tdx_non_root_may_gen_ex() const noexcept;

	/// (Intel VMX) `true` if it causes a VM exit in VMX non-root operation
	bool intel_vm_exit() const noexcept;

	/// (Intel VMX) `true` if it may cause a VM exit in VMX non-root operation
	bool intel_may_vm_exit() const noexcept;

	/// (Intel VMX) `true` if it causes an SMM VM exit in VMX root operation (if dual-monitor treatment is activated)
	bool intel_smm_vm_exit() const noexcept;

	/// (AMD SVM) `true` if it causes a #VMEXIT in guest mode
	bool amd_vm_exit() const noexcept;

	/// (AMD SVM) `true` if it may cause a #VMEXIT in guest mode
	bool amd_may_vm_exit() const noexcept;

	/// `true` if it causes a TSX abort inside a TSX transaction
	bool tsx_abort() const noexcept;

	/// `true` if it causes a TSX abort inside a TSX transaction depending on the implementation
	bool tsx_impl_abort() const noexcept;

	/// `true` if it may cause a TSX abort inside a TSX transaction depending on some condition
	bool tsx_may_abort() const noexcept;

	/// `true` if it's decoded by iced's 16-bit Intel decoder
	bool intel_decoder16() const noexcept;

	/// `true` if it's decoded by iced's 32-bit Intel decoder
	bool intel_decoder32() const noexcept;

	/// `true` if it's decoded by iced's 64-bit Intel decoder
	bool intel_decoder64() const noexcept;

	/// `true` if it's decoded by iced's 16-bit AMD decoder
	bool amd_decoder16() const noexcept;

	/// `true` if it's decoded by iced's 32-bit AMD decoder
	bool amd_decoder32() const noexcept;

	/// `true` if it's decoded by iced's 64-bit AMD decoder
	bool amd_decoder64() const noexcept;

	/// Gets the decoder option that's needed to decode the instruction or `DecoderOptions::NONE`.
	/// The return value is a `DecoderOptions` value.
	std::uint32_t decoder_option() const noexcept;

	/// Gets the opcode table
	OpCodeTableKind table() const noexcept { return table_; }

	/// Gets the mandatory prefix
	MandatoryPrefix mandatory_prefix() const noexcept { return mandatory_prefix_; }

	/// Gets the opcode byte(s). The low byte(s) of this value is the opcode. The length is in `op_code_len()`.
	/// It doesn't include the table value, see `table()`.
	///
	/// # Examples
	///
	/// ```cpp
	/// assert(code_ext::op_code(Code::Ffreep_sti).op_code() == 0xDFC0);
	/// assert(code_ext::op_code(Code::Vmrunw).op_code() == 0x01D8);
	/// assert(code_ext::op_code(Code::Sub_r8_rm8).op_code() == 0x2A);
	/// assert(code_ext::op_code(Code::Cvtpi2ps_xmm_mmm64).op_code() == 0x2A);
	/// ```
	std::uint32_t op_code() const noexcept { return static_cast<std::uint32_t>(op_code_); }

	/// Gets the length of the opcode bytes (`op_code()`). The low bytes is the opcode value.
	///
	/// # Examples
	///
	/// ```cpp
	/// assert(code_ext::op_code(Code::Ffreep_sti).op_code_len() == 2);
	/// assert(code_ext::op_code(Code::Vmrunw).op_code_len() == 2);
	/// assert(code_ext::op_code(Code::Sub_r8_rm8).op_code_len() == 1);
	/// assert(code_ext::op_code(Code::Cvtpi2ps_xmm_mmm64).op_code_len() == 1);
	/// ```
	std::uint32_t op_code_len() const noexcept;

	/// `true` if it's part of a group
	bool is_group() const noexcept { return group_index_ >= 0; }

	/// Group index (0-7) or -1. If it's 0-7, it's stored in the `reg` field of the `modrm` byte.
	std::int32_t group_index() const noexcept { return static_cast<std::int32_t>(group_index_); }

	/// `true` if it's part of a modrm.rm group
	bool is_rm_group() const noexcept { return rm_group_index_ >= 0; }

	/// Group index (0-7) or -1. If it's 0-7, it's stored in the `rm` field of the `modrm` byte.
	std::int32_t rm_group_index() const noexcept { return static_cast<std::int32_t>(rm_group_index_); }

	/// Gets the number of operands
	std::uint32_t op_count() const noexcept;

	/// Gets operand #0's opkind
	OpCodeOperandKind op0_kind() const noexcept { return op_kinds_[0]; }

	/// Gets operand #1's opkind
	OpCodeOperandKind op1_kind() const noexcept { return op_kinds_[1]; }

	/// Gets operand #2's opkind
	OpCodeOperandKind op2_kind() const noexcept { return op_kinds_[2]; }

	/// Gets operand #3's opkind
	OpCodeOperandKind op3_kind() const noexcept { return op_kinds_[3]; }

	/// Gets operand #4's opkind
	OpCodeOperandKind op4_kind() const noexcept { return op_kinds_[4]; }

	/// Gets an operand's opkind
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	OpCodeOperandKind op_kind(std::uint32_t operand) const noexcept {
		if (operand < MAX_OP_COUNT)
			return op_kinds_[operand];
		// Rust: debug_assert!(false)
		return invalid_operand_op_kind();
	}

	/// Gets an operand's opkind
	///
	/// # Errors
	///
	/// Fails if `operand` is invalid
	///
	/// # Arguments
	///
	/// * `operand`: Operand number, 0-4
	Result<OpCodeOperandKind> try_op_kind(std::uint32_t operand) const;

	/// Gets all operand kinds
	Slice<OpCodeOperandKind> op_kinds() const noexcept { return Slice<OpCodeOperandKind>(op_kinds_, op_count()); }

	/// Checks if the instruction is available in 16-bit mode, 32-bit mode or 64-bit mode
	///
	/// # Arguments
	///
	/// * `bitness`: 16, 32 or 64
	bool is_available_in_mode(std::uint32_t bitness) const noexcept;

	/// Gets the opcode string, eg. `VEX.128.66.0F38.W0 78 /r`, see also `instruction_string()`
	///
	///
	/// # Examples
	///
	/// ```cpp
	/// const OpCodeInfo& op_code = code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// assert(op_code.op_code_string() == "EVEX.256.66.0F.W1 28 /r");
	/// ```
	std::string_view op_code_string() const noexcept;

	/// Gets the instruction string, eg. `VPBROADCASTB xmm1, xmm2/m8`, see also `op_code_string()`
	///
	///
	/// # Examples
	///
	/// ```cpp
	/// const OpCodeInfo& op_code = code_ext::op_code(Code::EVEX_Vmovapd_ymm_k1z_ymmm256);
	/// assert(op_code.instruction_string() == "VMOVAPD ymm1 {k1}{z}, ymm2/m256");
	/// ```
	std::string_view instruction_string() const noexcept;

private:
	friend struct internal::OpCodeInfoInternal;

	// All values are generated (src/encoder/op_code_info_table.cpp) so the table is stored in read-only memory
	constexpr OpCodeInfo(std::uint16_t code, std::uint32_t enc_flags2, std::uint32_t enc_flags3, std::uint32_t opc_flags1, std::uint32_t opc_flags2,
		std::uint16_t op_code, std::uint16_t flags, std::uint8_t encoding, std::uint8_t operand_size, std::uint8_t address_size, std::uint8_t l,
		std::uint8_t tuple_type, std::uint8_t table, std::uint8_t mandatory_prefix, std::int8_t group_index, std::int8_t rm_group_index,
		std::uint8_t op0_kind, std::uint8_t op1_kind, std::uint8_t op2_kind, std::uint8_t op3_kind, std::uint8_t op4_kind,
		std::uint32_t strings_offset, std::uint8_t op_code_string_len, std::uint8_t instruction_string_len) noexcept
		: enc_flags2_(enc_flags2), enc_flags3_(enc_flags3), opc_flags1_(opc_flags1), opc_flags2_(opc_flags2), strings_offset_(strings_offset),
		  code_(static_cast<Code>(code)), op_code_(op_code), flags_(flags), encoding_(static_cast<EncodingKind>(encoding)),
		  operand_size_(operand_size), address_size_(address_size), l_(l), tuple_type_(static_cast<TupleType>(tuple_type)),
		  table_(static_cast<OpCodeTableKind>(table)), mandatory_prefix_(static_cast<MandatoryPrefix>(mandatory_prefix)), group_index_(group_index),
		  rm_group_index_(rm_group_index),
		  op_kinds_{static_cast<OpCodeOperandKind>(op0_kind), static_cast<OpCodeOperandKind>(op1_kind), static_cast<OpCodeOperandKind>(op2_kind),
			  static_cast<OpCodeOperandKind>(op3_kind), static_cast<OpCodeOperandKind>(op4_kind)},
		  op_code_string_len_(op_code_string_len), instruction_string_len_(instruction_string_len) {}

	static OpCodeOperandKind invalid_operand_op_kind() noexcept;

	static constexpr std::size_t MAX_OP_COUNT = IcedConstants::MAX_OP_COUNT;

	struct Flags {
		static constexpr std::uint16_t NONE = 0;
		static constexpr std::uint16_t IGNORES_ROUNDING_CONTROL = 0x0001;
		static constexpr std::uint16_t AMD_LOCK_REG_BIT = 0x0002;
		static constexpr std::uint16_t LIG = 0x0004;
		static constexpr std::uint16_t W = 0x0008;
		static constexpr std::uint16_t WIG = 0x0010;
		static constexpr std::uint16_t WIG32 = 0x0020;
		static constexpr std::uint16_t CPL0 = 0x0040;
		static constexpr std::uint16_t CPL1 = 0x0080;
		static constexpr std::uint16_t CPL2 = 0x0100;
		static constexpr std::uint16_t CPL3 = 0x0200;
	};

	std::uint32_t enc_flags2_;
	std::uint32_t enc_flags3_;
	std::uint32_t opc_flags1_;
	std::uint32_t opc_flags2_;
	// Location of the op code string (followed by the instruction string) in the generated strings table
	std::uint32_t strings_offset_;
	Code code_;
	std::uint16_t op_code_;
	std::uint16_t flags_;
	EncodingKind encoding_;
	std::uint8_t operand_size_;
	std::uint8_t address_size_;
	std::uint8_t l_;
	TupleType tuple_type_;
	OpCodeTableKind table_;
	MandatoryPrefix mandatory_prefix_;
	std::int8_t group_index_;
	std::int8_t rm_group_index_;
	OpCodeOperandKind op_kinds_[MAX_OP_COUNT];
	std::uint8_t op_code_string_len_;
	std::uint8_t instruction_string_len_;
};

/// Gets the instruction string (same as `OpCodeInfo::instruction_string()`), eg. `VMOVAPD ymm1 {k1}{z}, ymm2/m256`
inline std::string to_string(const OpCodeInfo& op_code) { return std::string(op_code.instruction_string()); }

} // namespace iced_x86
