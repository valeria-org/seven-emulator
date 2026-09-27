// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/op_kind.hpp"
#include "iced_x86/register.hpp"
#include <cstdint>
#include <optional>

namespace iced_x86 {
class Encoder;
class Instruction;
} // namespace iced_x86

namespace iced_x86::internal {

// Operand encoder. All instances are `constexpr` objects (see the generated src/encoder/op_code_handlers_table.cpp) so they're
// constant initialized (no dynamic initialization, no destructors).
class Op {
public:
	constexpr Op() noexcept = default;
	Op(const Op&) = delete;
	Op& operator=(const Op&) = delete;

	virtual void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const = 0;

	/// If this is an immediate operand, it returns the `OpKind` value, else it returns `std::nullopt`
	virtual std::optional<OpKind> immediate_op_kind() const noexcept { return std::nullopt; }

	/// If this is a near branch operand, it returns the `OpKind` value, else it returns `std::nullopt`
	virtual std::optional<OpKind> near_branch_op_kind() const noexcept { return std::nullopt; }

	/// If this is a far branch operand, it returns the `OpKind` value, else it returns `std::nullopt`
	virtual std::optional<OpKind> far_branch_op_kind() const noexcept { return std::nullopt; }

protected:
	// Not virtual: the objects are never destroyed polymorphically (they're all static constexpr objects)
	~Op() = default;
};

class InvalidOpHandler final : public Op {
public:
	constexpr InvalidOpHandler() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
};

class OpModRM_rm_mem_only final : public Op {
public:
	constexpr explicit OpModRM_rm_mem_only(bool must_use_sib) noexcept : must_use_sib(must_use_sib) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	bool must_use_sib;
};

class OpModRM_rm final : public Op {
public:
	constexpr OpModRM_rm(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpRegEmbed8 final : public Op {
public:
	constexpr OpRegEmbed8(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpModRM_rm_reg_only final : public Op {
public:
	constexpr OpModRM_rm_reg_only(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpModRM_reg final : public Op {
public:
	constexpr OpModRM_reg(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpModRM_reg_mem final : public Op {
public:
	constexpr OpModRM_reg_mem(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpModRM_regF0 final : public Op {
public:
	constexpr OpModRM_regF0(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpReg final : public Op {
public:
	constexpr explicit OpReg(Register register_) noexcept : register_(register_) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register register_;
};

class OpRegSTi final : public Op {
public:
	constexpr OpRegSTi() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
};

class OprDI final : public Op {
public:
	constexpr OprDI() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	static constexpr std::uint32_t get_reg_size(OpKind op_kind) noexcept {
		switch (op_kind) {
		case OpKind::MemorySegRDI:
			return 8;
		case OpKind::MemorySegEDI:
			return 4;
		case OpKind::MemorySegDI:
			return 2;
		default:
			return 0;
		}
	}
};

class OpIb final : public Op {
public:
	constexpr explicit OpIb(OpKind op_kind) noexcept : op_kind(op_kind) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return op_kind; }

	OpKind op_kind;
};

class OpIw final : public Op {
public:
	constexpr OpIw() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return OpKind::Immediate16; }
};

class OpId final : public Op {
public:
	constexpr explicit OpId(OpKind op_kind) noexcept : op_kind(op_kind) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return op_kind; }

	OpKind op_kind;
};

class OpIq final : public Op {
public:
	constexpr OpIq() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return OpKind::Immediate64; }
};

class OpI4 final : public Op {
public:
	constexpr OpI4() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return OpKind::Immediate8; }
};

class OpX final : public Op {
public:
	constexpr OpX() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	static constexpr std::uint32_t get_xreg_size(OpKind op_kind) noexcept {
		switch (op_kind) {
		case OpKind::MemorySegRSI:
			return 8;
		case OpKind::MemorySegESI:
			return 4;
		case OpKind::MemorySegSI:
			return 2;
		default:
			return 0;
		}
	}

	static constexpr std::uint32_t get_yreg_size(OpKind op_kind) noexcept {
		switch (op_kind) {
		case OpKind::MemoryESRDI:
			return 8;
		case OpKind::MemoryESEDI:
			return 4;
		case OpKind::MemoryESDI:
			return 2;
		default:
			return 0;
		}
	}
};

class OpY final : public Op {
public:
	constexpr OpY() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
};

class OpMRBX final : public Op {
public:
	constexpr OpMRBX() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
};

class OpJ final : public Op {
public:
	constexpr OpJ(OpKind op_kind, std::uint32_t imm_size) noexcept : op_kind(op_kind), imm_size(imm_size) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> near_branch_op_kind() const noexcept override { return op_kind; }

	OpKind op_kind;
	std::uint32_t imm_size;
};

class OpJx final : public Op {
public:
	constexpr explicit OpJx(std::uint32_t imm_size) noexcept : imm_size(imm_size) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> near_branch_op_kind() const noexcept override;

	std::uint32_t imm_size;
};

class OpJdisp final : public Op {
public:
	constexpr explicit OpJdisp(std::uint32_t displ_size) noexcept : displ_size(displ_size) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> near_branch_op_kind() const noexcept override {
		return displ_size == 2 ? OpKind::NearBranch16 : OpKind::NearBranch32;
	}

	std::uint32_t displ_size;
};

class OpA final : public Op {
public:
	constexpr explicit OpA(std::uint32_t size) noexcept : size(size) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> far_branch_op_kind() const noexcept override { return size == 2 ? OpKind::FarBranch16 : OpKind::FarBranch32; }

	std::uint32_t size;
};

class OpO final : public Op {
public:
	constexpr OpO() noexcept {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
};

class OpImm final : public Op {
public:
	constexpr explicit OpImm(std::uint8_t value) noexcept : value(value) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;
	std::optional<OpKind> immediate_op_kind() const noexcept override { return OpKind::Immediate8; }

	std::uint8_t value;
};

class OpHx final : public Op {
public:
	constexpr OpHx(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

class OpVsib final : public Op {
public:
	constexpr OpVsib(Register vsib_index_reg_lo, Register vsib_index_reg_hi) noexcept
		: vsib_index_reg_lo(vsib_index_reg_lo), vsib_index_reg_hi(vsib_index_reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register vsib_index_reg_lo;
	Register vsib_index_reg_hi;
};

class OpIsX final : public Op {
public:
	constexpr OpIsX(Register reg_lo, Register reg_hi) noexcept : reg_lo(reg_lo), reg_hi(reg_hi) {}
	void encode(Encoder& encoder, const Instruction& instruction, std::uint32_t operand) const override;

	Register reg_lo;
	Register reg_hi;
};

} // namespace iced_x86::internal
