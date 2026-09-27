// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <new>
#include <optional>
#include <utility>

#include "iced_x86/symbol_resolver.hpp"

namespace iced_x86::internal {

/// An optional `SymbolResult` (Rust: `Option<SymbolResult>`). It's the same as `std::optional<SymbolResult>` except that
/// creating an empty instance is free. (GCC zero fills the whole `std::optional<SymbolResult>` when it creates an empty
/// one which is slow and the formatters create one for every immediate/branch/memory operand.)
class OptionalSymbolResult {
public:
	OptionalSymbolResult() noexcept : has_value_(false) {}
	~OptionalSymbolResult() { reset(); }
	OptionalSymbolResult(const OptionalSymbolResult&) = delete;
	OptionalSymbolResult& operator=(const OptionalSymbolResult&) = delete;

	/// Moves the symbol result (if any) to this instance
	void set(std::optional<SymbolResult>&& symbol) noexcept {
		reset();
		if (symbol) {
			new (static_cast<void*>(storage_)) SymbolResult(std::move(*symbol));
			has_value_ = true;
		}
	}

	void reset() noexcept {
		if (has_value_) {
			get().~SymbolResult();
			has_value_ = false;
		}
	}

	bool has_value() const noexcept { return has_value_; }
	explicit operator bool() const noexcept { return has_value_; }
	const SymbolResult& operator*() const noexcept { return get(); }
	const SymbolResult* operator->() const noexcept { return &get(); }

private:
	const SymbolResult& get() const noexcept { return *std::launder(reinterpret_cast<const SymbolResult*>(storage_)); }
	SymbolResult& get() noexcept { return *std::launder(reinterpret_cast<SymbolResult*>(storage_)); }

	alignas(SymbolResult) unsigned char storage_[sizeof(SymbolResult)];
	bool has_value_;
};

} // namespace iced_x86::internal
