// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "internal/block_encoder/block.hpp"

#include <utility>

#include "internal/iced_assert.hpp"

namespace iced_x86::internal {

Block::Block(std::uint32_t bitness, std::uint64_t rip_, bool return_reloc_infos, std::size_t start_index, std::size_t end_index)
	: encoder(bitness), rip(rip_), reloc_infos_(), data_vec_(), alignment_(static_cast<std::uint64_t>(bitness) / 8), valid_data_(),
	  valid_data_address_(0), valid_data_address_aligned_(0), start_index_(start_index), end_index_(end_index),
	  has_reloc_infos_(return_reloc_infos) {}

std::uint32_t Block::alloc_pointer_location() {
	ICED_ASSERT(data_vec_.size() < UINT32_MAX);
	auto index = static_cast<std::uint32_t>(data_vec_.size());
	data_vec_.push_back(BlockData{});
	return index;
}

void Block::initialize_data(std::uint64_t base_addr) {
	valid_data_address_ = base_addr;

	std::uint64_t addr = (base_addr + alignment_ - 1) & ~(alignment_ - 1);
	valid_data_address_aligned_ = addr;
	for (std::size_t i = 0; i < data_vec_.size(); i++) {
		auto& data = data_vec_[i];
		if (!data.is_valid)
			continue;
		data.address_ = addr;
		data.address_initd = true;
		valid_data_.push_back(static_cast<std::uint32_t>(i));
		addr += alignment_;
	}
}

Result<void> Block::write_data() {
	if (valid_data_.empty())
		return {};
	for (std::uint64_t i = 0, count = valid_data_address_aligned_ - valid_data_address_; i < count; i++)
		EncoderInternal::write_byte_internal(encoder, 0xCC);
	switch (alignment_) {
	case 8:
		for (auto index : valid_data_) {
			const auto& data = data_vec_[index];
			if (has_reloc_infos_) {
				auto address = data.address();
				if (address.is_err())
					return address.error();
				reloc_infos_.push_back(RelocInfo(RelocKind::Offset64, address.value()));
			}
			std::uint64_t d64 = data.data;
			auto d = static_cast<std::uint32_t>(d64);
			EncoderInternal::write_byte_internal(encoder, d);
			EncoderInternal::write_byte_internal(encoder, d >> 8);
			EncoderInternal::write_byte_internal(encoder, d >> 16);
			EncoderInternal::write_byte_internal(encoder, d >> 24);
			d = static_cast<std::uint32_t>(d64 >> 32);
			EncoderInternal::write_byte_internal(encoder, d);
			EncoderInternal::write_byte_internal(encoder, d >> 8);
			EncoderInternal::write_byte_internal(encoder, d >> 16);
			EncoderInternal::write_byte_internal(encoder, d >> 24);
		}
		break;

	default:
		ICED_UNREACHABLE();
	}

	return {};
}

std::vector<RelocInfo> Block::take_reloc_infos() {
	has_reloc_infos_ = false;
	return std::move(reloc_infos_);
}

void Block::dispose() noexcept {
	data_vec_.clear();
	valid_data_.clear();
}

} // namespace iced_x86::internal
