// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include "iced_x86/code_size.hpp"
#include "iced_x86/iced_constants.hpp"
#include "iced_x86/tuple_type.hpp"
#include "internal/encoder/ops.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

namespace iced_x86::internal {

// All operand handlers (`Op`s) used by the op code handlers (generated). `EncOpCodeHandler::operands` are indexes into this table.
extern const Op* const OPS_TABLE[];

// The kind of an `EncOpCodeHandler` (Rust uses one struct per kind)
enum class OpCodeHandlerKind : std::uint8_t {
	Invalid,
	DeclareData,
	ZeroBytes,
	Legacy,
	VEX,
	XOP,
	EVEX,
	MVEX,
	D3NOW,
};
inline constexpr std::size_t OP_CODE_HANDLER_KIND_COUNT = 9;

// Rust creates one handler object per `Code` value at runtime (in a Vec). C++ uses a compact POD struct
// whose values are generated (src/encoder/op_code_handlers_table.cpp) so the table lives in read-only
// memory and no heap memory is used. The handler kind specific data is stored in a union.
struct EncOpCodeHandler {
	static constexpr std::uint32_t MAX_OPERANDS = 5;

	struct DeclareDataData {
		std::uint8_t elem_size;
	};
	struct LegacyData {
		std::uint8_t table_byte1;
		std::uint8_t table_byte2;
		std::uint8_t mandatory_prefix;
	};
	struct VexData {
		std::uint8_t table;
		std::uint8_t last_byte;
		std::uint8_t mask_w_l;
		std::uint8_t mask_l;
		bool w1;
	};
	struct XopData {
		std::uint8_t table;
		std::uint8_t last_byte;
	};
	struct EvexData {
		std::uint8_t table;
		std::uint8_t p1_bits;
		std::uint8_t ll_bits;
		std::uint8_t mask_w;
		std::uint8_t mask_ll;
		TupleType tuple_type;
	};
	struct MvexData {
		std::uint8_t table;
		std::uint8_t p1_bits;
		std::uint8_t mask_w;
	};
	struct D3nowData {
		std::uint8_t immediate;
	};
	union Data {
		std::uint8_t none;
		DeclareDataData declare_data;
		LegacyData legacy;
		VexData vex;
		XopData xop;
		EvexData evex;
		MvexData mvex;
		D3nowData d3now;

		constexpr Data() noexcept : none(0) {}
		constexpr Data(DeclareDataData d) noexcept : declare_data(d) {}
		constexpr Data(LegacyData d) noexcept : legacy(d) {}
		constexpr Data(VexData d) noexcept : vex(d) {}
		constexpr Data(XopData d) noexcept : xop(d) {}
		constexpr Data(EvexData d) noexcept : evex(d) {}
		constexpr Data(MvexData d) noexcept : mvex(d) {}
		constexpr Data(D3nowData d) noexcept : d3now(d) {}
	};

	std::uint32_t enc_flags3; // EncFlags3
	std::uint16_t op_code;
	OpCodeHandlerKind kind;
	std::uint8_t operands_len;
	// Indexes into `OPS_TABLE`
	std::uint8_t operands[MAX_OPERANDS];
	std::int8_t group_index;
	std::int8_t rm_group_index;
	CodeSize op_size;
	CodeSize addr_size;
	bool is_2byte_opcode;
	bool is_special_instr;
	Data u;

	const Op* operand(std::size_t index) const noexcept { return OPS_TABLE[operands[index]]; }
};

// One handler per `Code` value (generated)
extern const EncOpCodeHandler OP_CODE_HANDLERS[IcedConstants::CODE_ENUM_COUNT];

using OpCodeHandlerEncodeFn = void (*)(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction);
// Index = OpCodeHandlerKind
extern const OpCodeHandlerEncodeFn OP_CODE_HANDLER_ENCODE_FNS[OP_CODE_HANDLER_KIND_COUNT];

std::optional<std::int8_t> evex_try_convert_to_disp8n(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction, std::int32_t displ);
std::optional<std::int8_t> mvex_try_convert_to_disp8n(const EncOpCodeHandler* self, Encoder& encoder, const Instruction& instruction, std::int32_t displ);

// Error message used by the invalid handler
inline constexpr const char* INVALID_HANDLER_ERROR_MESSAGE = "Can't encode an invalid instruction";

} // namespace iced_x86::internal
