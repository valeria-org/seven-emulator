// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "iced_x86/block_encoder.hpp"
#include "iced_x86/encoder.hpp"
#include "iced_x86/iced_error.hpp"
#include "internal/encoder/encoder_internal.hpp"

namespace iced_x86::internal {

// Rust uses `Rc<RefCell<BlockData>>` shared by the block and the instruction. In C++ the block owns all
// its `BlockData`s and an instruction stores the index of its `BlockData` (its instruction is always part
// of the same block).
struct BlockData {
	std::uint64_t data = 0;
	std::uint64_t address_ = 0;
	bool address_initd = false;
	bool is_valid = true;

	Result<std::uint64_t> address() const noexcept {
		if (is_valid && address_initd)
			return address_;
		return IcedError("Internal error");
	}
};

class Block {
public:
	Encoder encoder;
	std::uint64_t rip;

	Block(std::uint32_t bitness, std::uint64_t rip_, bool return_reloc_infos, std::size_t start_index, std::size_t end_index);

	bool is_in_block(std::size_t instr_index) const noexcept { return start_index_ <= instr_index && instr_index < end_index_; }

	// Returns the index of the new `BlockData`, see `data()`
	std::uint32_t alloc_pointer_location();
	BlockData& data(std::uint32_t index) noexcept { return data_vec_[index]; }
	const BlockData& data(std::uint32_t index) const noexcept { return data_vec_[index]; }

	void initialize_data(std::uint64_t base_addr);
	Result<void> write_data();

	std::size_t buffer_pos() const noexcept { return EncoderInternal::position(encoder); }
	void write_byte(std::uint32_t value) { EncoderInternal::write_byte_internal(encoder, value); }
	std::vector<std::uint8_t> take_buffer() { return encoder.take_buffer(); }
	std::vector<RelocInfo> take_reloc_infos();
	void dispose() noexcept;

	bool can_add_reloc_infos() const noexcept { return has_reloc_infos_; }
	void add_reloc_info(const RelocInfo& reloc_info) {
		if (has_reloc_infos_)
			reloc_infos_.push_back(reloc_info);
	}

private:
	std::vector<RelocInfo> reloc_infos_;
	std::vector<BlockData> data_vec_;
	std::uint64_t alignment_;
	// Indexes into `data_vec_`
	std::vector<std::uint32_t> valid_data_;
	std::uint64_t valid_data_address_;
	std::uint64_t valid_data_address_aligned_;
	// start and end indexes (exclusive) of its instructions, eg. all_instrs[start_index_..end_index_]
	std::size_t start_index_;
	std::size_t end_index_;
	bool has_reloc_infos_;
};

} // namespace iced_x86::internal
