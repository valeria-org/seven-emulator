// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

// The pseudo op mnemonics used by all formatters (Rust: formatter/pseudo_ops.rs and formatter/fast/pseudo_ops_fast.rs)

#pragma once

#include <cstddef>
#include <string_view>

#include "internal/formatter/pseudo_ops_kind.hpp"

namespace iced_x86::internal::pseudo_ops_defs {

constexpr std::size_t PSEUDO_OPS_KIND_COUNT = static_cast<std::size_t>(PseudoOpsKind::vpcmpud6) + 1;

inline constexpr std::string_view CC[32] = {
	"eq",
	"lt",
	"le",
	"unord",
	"neq",
	"nlt",
	"nle",
	"ord",
	"eq_uq",
	"nge",
	"ngt",
	"false",
	"neq_oq",
	"ge",
	"gt",
	"true",
	"eq_os",
	"lt_oq",
	"le_oq",
	"unord_s",
	"neq_us",
	"nlt_uq",
	"nle_uq",
	"ord_s",
	"eq_us",
	"nge_uq",
	"ngt_uq",
	"false_os",
	"neq_os",
	"ge_oq",
	"gt_oq",
	"true_us",
};

inline constexpr std::string_view CC6[8] = {
	"eq",
	"lt",
	"le",
	"??",
	"neq",
	"nlt",
	"nle",
	"???",
};

inline constexpr std::string_view XOPCC[8] = {
	"lt",
	"le",
	"gt",
	"ge",
	"eq",
	"neq",
	"false",
	"true",
};

inline constexpr std::string_view PCMPCC[8] = {
	"eq",
	"lt",
	"le",
	"false",
	"neq",
	"nlt",
	"nle",
	"true",
};

inline constexpr std::string_view PCLMULQDQ[4] = {
	"pclmullqlqdq",
	"pclmulhqlqdq",
	"pclmullqhqdq",
	"pclmulhqhqdq",
};

inline constexpr std::string_view VPCLMULQDQ[4] = {
	"vpclmullqlqdq",
	"vpclmulhqlqdq",
	"vpclmullqhqdq",
	"vpclmulhqhqdq",
};

/// Pseudo ops `kind` = `prefix + cc[i] + suffix` for `i` in `0..size`
struct PseudoOpsDef {
	PseudoOpsKind kind;
	const std::string_view* cc;
	std::size_t size;
	std::string_view prefix;
	std::string_view suffix;
};

// clang-format off
inline constexpr PseudoOpsDef PSEUDO_OPS_DEFS[PSEUDO_OPS_KIND_COUNT] = {
	{PseudoOpsKind::cmpps, CC, 8, "cmp", "ps"},
	{PseudoOpsKind::vcmpps, CC, 32, "vcmp", "ps"},
	{PseudoOpsKind::cmppd, CC, 8, "cmp", "pd"},
	{PseudoOpsKind::vcmppd, CC, 32, "vcmp", "pd"},
	{PseudoOpsKind::cmpss, CC, 8, "cmp", "ss"},
	{PseudoOpsKind::vcmpss, CC, 32, "vcmp", "ss"},
	{PseudoOpsKind::cmpsd, CC, 8, "cmp", "sd"},
	{PseudoOpsKind::vcmpsd, CC, 32, "vcmp", "sd"},
	{PseudoOpsKind::pclmulqdq, PCLMULQDQ, 4, "", ""},
	{PseudoOpsKind::vpclmulqdq, VPCLMULQDQ, 4, "", ""},
	{PseudoOpsKind::vpcomb, XOPCC, 8, "vpcom", "b"},
	{PseudoOpsKind::vpcomw, XOPCC, 8, "vpcom", "w"},
	{PseudoOpsKind::vpcomd, XOPCC, 8, "vpcom", "d"},
	{PseudoOpsKind::vpcomq, XOPCC, 8, "vpcom", "q"},
	{PseudoOpsKind::vpcomub, XOPCC, 8, "vpcom", "ub"},
	{PseudoOpsKind::vpcomuw, XOPCC, 8, "vpcom", "uw"},
	{PseudoOpsKind::vpcomud, XOPCC, 8, "vpcom", "ud"},
	{PseudoOpsKind::vpcomuq, XOPCC, 8, "vpcom", "uq"},
	{PseudoOpsKind::vpcmpb, PCMPCC, 8, "vpcmp", "b"},
	{PseudoOpsKind::vpcmpw, PCMPCC, 8, "vpcmp", "w"},
	{PseudoOpsKind::vpcmpd, PCMPCC, 8, "vpcmp", "d"},
	{PseudoOpsKind::vpcmpq, PCMPCC, 8, "vpcmp", "q"},
	{PseudoOpsKind::vpcmpub, PCMPCC, 8, "vpcmp", "ub"},
	{PseudoOpsKind::vpcmpuw, PCMPCC, 8, "vpcmp", "uw"},
	{PseudoOpsKind::vpcmpud, PCMPCC, 8, "vpcmp", "ud"},
	{PseudoOpsKind::vpcmpuq, PCMPCC, 8, "vpcmp", "uq"},
	{PseudoOpsKind::vcmpph, CC, 32, "vcmp", "ph"},
	{PseudoOpsKind::vcmpsh, CC, 32, "vcmp", "sh"},
	{PseudoOpsKind::vcmpps8, CC, 8, "vcmp", "ps"},
	{PseudoOpsKind::vcmppd8, CC, 8, "vcmp", "pd"},
	{PseudoOpsKind::vpcmpd6, CC6, 8, "vpcmp", "d"},
	{PseudoOpsKind::vpcmpud6, CC6, 8, "vpcmp", "ud"},
};
// clang-format on

} // namespace iced_x86::internal::pseudo_ops_defs
