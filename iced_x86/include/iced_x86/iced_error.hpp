// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstdlib>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace iced_x86 {

/// iced error. It's returned (inside a `Result<T>`) by all methods that can fail.
///
/// The library never throws exceptions and can be compiled with `-fno-exceptions -fno-rtti`.
class IcedError {
public:
	/// Creates an error from a static string (no allocation)
	explicit IcedError(const char* message) noexcept : static_message_(message) {}
	/// Creates an error from a dynamic string
	explicit IcedError(std::string message) : static_message_(nullptr), message_(std::move(message)) {}

	/// Gets the error message
	const char* message() const noexcept { return static_message_ != nullptr ? static_message_ : message_.c_str(); }

private:
	const char* static_message_;
	std::string message_;
};

namespace internal {
[[noreturn]] inline void result_unwrap_failed() noexcept { std::abort(); }
} // namespace internal

/// Either a value (`is_ok()`) or an `IcedError` (`is_err()`), similar to Rust's `Result<T, IcedError>`.
///
/// Accessing `value()`, `*result` or `result->` when it's an error (or `error()` when it's a value) aborts the process.
template <typename T>
class [[nodiscard]] Result {
public:
	/// Creates a success result
	Result(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value) : ok_(true) { new (&value_) T(value); }
	/// Creates a success result
	Result(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value) : ok_(true) { new (&value_) T(std::move(value)); }
	/// Creates an error result
	Result(IcedError error) : ok_(false) { new (&error_) IcedError(std::move(error)); }

	Result(const Result& other) : ok_(other.ok_) {
		if (ok_)
			new (&value_) T(other.value_);
		else
			new (&error_) IcedError(other.error_);
	}
	Result(Result&& other) noexcept(std::is_nothrow_move_constructible<T>::value) : ok_(other.ok_) {
		if (ok_)
			new (&value_) T(std::move(other.value_));
		else
			new (&error_) IcedError(std::move(other.error_));
	}
	Result& operator=(const Result& other) {
		if (this != &other) {
			destroy();
			ok_ = other.ok_;
			if (ok_)
				new (&value_) T(other.value_);
			else
				new (&error_) IcedError(other.error_);
		}
		return *this;
	}
	Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
		if (this != &other) {
			destroy();
			ok_ = other.ok_;
			if (ok_)
				new (&value_) T(std::move(other.value_));
			else
				new (&error_) IcedError(std::move(other.error_));
		}
		return *this;
	}
	~Result() { destroy(); }

	/// `true` if it contains a value
	bool is_ok() const noexcept { return ok_; }
	/// `true` if it contains an error
	bool is_err() const noexcept { return !ok_; }
	/// `true` if it contains a value (same as `is_ok()`, `std::optional`-like name)
	bool has_value() const noexcept { return ok_; }
	/// `true` if it contains a value
	explicit operator bool() const noexcept { return ok_; }

	/// Gets the value. Aborts if it's an error.
	T& value() & noexcept {
		if (!ok_)
			internal::result_unwrap_failed();
		return value_;
	}
	/// Gets the value. Aborts if it's an error.
	const T& value() const& noexcept {
		if (!ok_)
			internal::result_unwrap_failed();
		return value_;
	}
	/// Gets the value. Aborts if it's an error.
	T&& value() && noexcept {
		if (!ok_)
			internal::result_unwrap_failed();
		return std::move(value_);
	}
	/// Gets the value or `default_value` if it's an error
	T value_or(T default_value) const& { return ok_ ? value_ : std::move(default_value); }

	/// Gets the value (same as `value()`). Aborts if it's an error.
	T& operator*() & noexcept { return value(); }
	/// Gets the value (same as `value()`). Aborts if it's an error.
	const T& operator*() const& noexcept { return value(); }
	/// Gets the value (same as `value()`). Aborts if it's an error.
	T&& operator*() && noexcept { return std::move(*this).value(); }
	/// Accesses a member of the value, eg. `result->size()`. Aborts if it's an error.
	T* operator->() noexcept { return &value(); }
	/// Accesses a member of the value, eg. `result->size()`. Aborts if it's an error.
	const T* operator->() const noexcept { return &value(); }

	/// Gets the error. Aborts if it's a value.
	const IcedError& error() const& noexcept {
		if (ok_)
			internal::result_unwrap_failed();
		return error_;
	}

private:
	void destroy() noexcept {
		if (ok_)
			value_.~T();
		else
			error_.~IcedError();
	}

	bool ok_;
	union {
		T value_;
		IcedError error_;
	};
};

/// A result without a value: success (`is_ok()`) or an `IcedError` (`is_err()`)
template <>
class [[nodiscard]] Result<void> {
public:
	/// Creates a success result
	Result() noexcept : ok_(true), error_(nullptr) {}
	/// Creates an error result
	Result(IcedError error) : ok_(false), error_(std::move(error)) {}

	/// `true` if it's a success result
	bool is_ok() const noexcept { return ok_; }
	/// `true` if it contains an error
	bool is_err() const noexcept { return !ok_; }
	/// `true` if it's a success result (same as `is_ok()`)
	bool has_value() const noexcept { return ok_; }
	/// `true` if it's a success result
	explicit operator bool() const noexcept { return ok_; }
	/// Aborts if it's an error
	void value() const noexcept {
		if (!ok_)
			internal::result_unwrap_failed();
	}

	/// Gets the error. Aborts if it's a success result.
	const IcedError& error() const& noexcept {
		if (ok_)
			internal::result_unwrap_failed();
		return error_;
	}

private:
	bool ok_;
	IcedError error_;
};

} // namespace iced_x86
