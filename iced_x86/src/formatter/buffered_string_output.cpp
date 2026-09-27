// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/formatter/buffered_string_output.hpp"

namespace iced_x86::internal {

void BufferedStringOutput::allocate(FormatterStringBuffer& buffer) {
	buffer.data.reset(new char[FormatterStringBuffer::INITIAL_CAPACITY]);
	buffer.capacity = FormatterStringBuffer::INITIAL_CAPACITY;
}

void BufferedStringOutput::grow(std::size_t size) {
	char* old_data = buffer_->data.get();
	const auto used = static_cast<std::size_t>(pos_ - old_data);
	std::size_t new_capacity = buffer_->capacity * 2;
	if (new_capacity < used + size)
		new_capacity = used + size;
	std::unique_ptr<char[]> new_data(new char[new_capacity]);
	std::memcpy(new_data.get(), old_data, used);
	buffer_->data = std::move(new_data);
	buffer_->capacity = new_capacity;
	pos_ = buffer_->data.get() + used;
	end_ = buffer_->data.get() + new_capacity;
}

} // namespace iced_x86::internal
