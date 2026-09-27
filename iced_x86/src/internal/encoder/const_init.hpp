// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

// Used by the generated encoder tables. They must be constant initialized (stored in read-only memory, no code runs
// at startup and no heap memory is used). If the compiler supports it, it verifies that the table is constant initialized.
#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#define ICED_CONSTINIT constinit
#elif defined(__clang__)
#define ICED_CONSTINIT [[clang::require_constant_initialization]]
#else
#define ICED_CONSTINIT
#endif
