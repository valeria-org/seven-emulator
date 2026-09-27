// SPDX-License-Identifier: MIT
// Copyright (C) 2018-present iced project and contributors

#include "iced_x86/block_encoder.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "iced_x86/code.hpp"
#include "internal/block_encoder/block.hpp"
#include "internal/block_encoder/instr.hpp"
#include "internal/iced_assert.hpp"

namespace iced_x86 {
namespace internal {
namespace {

struct BlockInfo {
	Block block;
	// Same as the block's start and end indexes, but inlined here for a small perf increase.
	std::size_t start;
	std::size_t end;

	BlockInfo(std::uint32_t bitness, std::uint64_t rip, bool return_reloc_infos, std::size_t start_, std::size_t end_)
		: block(bitness, rip, return_reloc_infos, start_, end_), start(start_), end(end_) {}
};

TargetInstr get_target(const BlockEncInt& benc, const InstrBase& base, std::uint64_t address) noexcept {
	if ((address != 0 || !benc.has_multiple_zero_ip_instrs) && base.orig_ip == address)
		return TargetInstr::new_owner();
	// `to_instr_index` is sorted in reverse order
	const auto& v = benc.to_instr_index;
	auto it = std::lower_bound(v.begin(), v.end(), address,
		[](const std::pair<std::uint64_t, std::size_t>& a, std::uint64_t b) { return a.first > b; });
	if (it != v.end() && it->first == address)
		return TargetInstr::new_instr(it->second);
	return TargetInstr::new_address(address);
}

IcedError multiple_instructions_same_ip_error(std::uint64_t ip) {
	// "Multiple instructions with the same IP: 0x{ip:X}"
	static const char HEX_DIGITS[] = "0123456789ABCDEF";
	char hex[16];
	std::size_t hex_len = 0;
	do {
		hex[hex_len++] = HEX_DIGITS[ip & 0xF];
		ip >>= 4;
	} while (ip != 0);
	std::string message("Multiple instructions with the same IP: 0x");
	while (hex_len > 0)
		message.push_back(hex[--hex_len]);
	return IcedError(std::move(message));
}

class BlockEncoderImpl {
public:
	BlockEncoderImpl(std::uint32_t bitness, std::uint32_t options) : blocks_(), all_instrs_(), all_ips_(), benc_(bitness, options) {}

	// Not inlined so the caller's stack frame stays small
	ICED_NOINLINE Result<void> initialize(const InstructionBlock* instr_blocks, std::size_t block_count);
	ICED_NOINLINE Result<std::vector<BlockEncoderResult>> encode2();

private:
	std::vector<BlockInfo> blocks_;
	std::vector<InstrEntry> all_instrs_;
	std::vector<std::uint64_t> all_ips_;
	BlockEncInt benc_;
};

Result<void> BlockEncoderImpl::initialize(const InstructionBlock* instr_blocks, std::size_t block_count) {
	const std::uint32_t options = benc_.options;
	std::size_t total_instr_len = 0;
	for (std::size_t i = 0; i < block_count; i++)
		total_instr_len += instr_blocks[i].count();
	blocks_.reserve(block_count);
	all_instrs_.reserve(total_instr_len);
	all_ips_.reserve(total_instr_len);

	// Rust creates the blocks in this loop and then sorts them by rip. Our blocks own an encoder so we sort
	// (rip, block index) first and create the blocks in sorted order.
	struct BlockPos {
		std::uint64_t rip;
		std::size_t index;
		std::size_t start_index;
		std::size_t end_index;
	};
	std::vector<BlockPos> block_pos;
	block_pos.reserve(block_count);

	std::size_t instr_count = 0;
	for (std::size_t block_index = 0; block_index < block_count; block_index++) {
		const auto& instr_block = instr_blocks[block_index];
		const Instruction* instructions = instr_block.instructions();
		const std::size_t count = instr_block.count();
		instr_count += count;
		std::uint64_t ip = instr_block.rip();
		const std::size_t start_index = all_instrs_.size();
		for (std::size_t i = 0; i < count; i++) {
			const auto& instruction = instructions[i];
			all_instrs_.emplace_back();
			auto& entry = all_instrs_.back();
			entry.base = InstrBase{0, instruction.ip(), false};
			InstrUtils::create(benc_, entry.base, entry.instr, instruction);
			ICED_DEBUG_ASSERT(entry.base.size != 0 || instruction.code() == Code::Zero_bytes);
			ip += entry.base.size;
			all_ips_.push_back(ip);
		}
		const std::size_t end_index = all_instrs_.size();
		block_pos.push_back(BlockPos{instr_block.rip(), block_index, start_index, end_index});
	}
	// Optimize from low to high addresses
	std::sort(block_pos.begin(), block_pos.end(), [](const BlockPos& a, const BlockPos& b) {
		if (a.rip != b.rip)
			return a.rip < b.rip;
		return a.index < b.index;
	});
	const bool return_reloc_infos = (options & BlockEncoderOptions::RETURN_RELOC_INFOS) != 0;
	for (const auto& pos : block_pos)
		blocks_.emplace_back(benc_.bitness, pos.rip, return_reloc_infos, pos.start_index, pos.end_index);

	benc_.to_instr_index.reserve(instr_count);
	// There must not be any instructions with the same IP, except if IP = 0 (default value)
	std::size_t num_ip_0 = 0;
	for (const auto& info : blocks_) {
		// Reverse here since we'll sort them in reverse order, see below
		for (std::size_t i = info.end; i > info.start;) {
			i--;
			const std::uint64_t orig_ip = all_instrs_[i].base.orig_ip;
			bool insert;
			if (orig_ip == 0) {
				num_ip_0++;
				insert = num_ip_0 == 1;
			} else
				insert = true;
			if (insert)
				benc_.to_instr_index.emplace_back(orig_ip, i);
		}
	}
	// We sort them in reverse order so that if we must remove the 'ip==0' entry, we just need to pop()
	std::sort(benc_.to_instr_index.begin(), benc_.to_instr_index.end(),
		[](const std::pair<std::uint64_t, std::size_t>& a, const std::pair<std::uint64_t, std::size_t>& b) { return a.first > b.first; });
	if (num_ip_0 > 1) {
		benc_.has_multiple_zero_ip_instrs = true;
		if (!benc_.to_instr_index.empty()) {
			ICED_DEBUG_ASSERT(benc_.to_instr_index.back().first == 0);
			if (benc_.to_instr_index.back().first == 0)
				benc_.to_instr_index.pop_back();
		}
	}
	if (!benc_.to_instr_index.empty()) {
		std::uint64_t prev_ip = benc_.to_instr_index[0].first;
		for (std::size_t i = 1; i < benc_.to_instr_index.size(); i++) {
			const std::uint64_t ip = benc_.to_instr_index[i].first;
			if (ip == prev_ip)
				return multiple_instructions_same_ip_error(ip);
			prev_ip = ip;
		}
	}

	for (const auto& info : blocks_) {
		std::uint64_t ip = info.block.rip;
		for (std::size_t i = info.start; i < info.end; i++) {
			auto& entry = all_instrs_[i];
			all_ips_[i] = ip;
			if (!entry.base.done) {
				auto target = entry.instr.get_target_instr();
				*target.first = get_target(benc_, entry.base, target.second);
			}
			ip += entry.base.size;
		}
	}

	return {};
}

Result<std::vector<BlockEncoderResult>> BlockEncoderImpl::encode2() {
	// 5 iters is enough even if millions of instructions are encoded. < 10 instructions are optimized per loop
	// iteration after only a few loop iters. It's not worth optimizing the remaining few instructions.
	for (int iter = 0; iter < 5; iter++) {
		bool updated = false;
		for (auto& info : blocks_) {
			std::uint64_t gained = 0;
			InstrContext ctx{info.block, all_ips_.data(), info.block.rip};
			for (std::size_t i = info.start; i < info.end; i++) {
				auto& entry = all_instrs_[i];
				auto& base = entry.base;
				ctx.all_ips[i] = ctx.ip;
				// If it can't be optimized further, don't call its optimize() fn for a nice speedup
				if (!base.done) {
					const std::uint32_t old_size = base.size;
					if (entry.instr.optimize(base, ctx, gained)) {
						const std::uint32_t instr_size = base.size;
						if (instr_size > old_size)
							return IcedError("Internal error");
						if (instr_size < old_size) {
							gained += old_size - instr_size;
							updated = true;
						}
					} else if (base.size != old_size)
						return IcedError("Internal error");
				}
				ctx.ip += base.size;
			}
		}
		if (!updated)
			break;
	}

	for (auto& info : blocks_) {
		const std::size_t index = info.end - 1;
		if (info.end != 0) {
			const std::uint64_t last_ip = all_ips_[index];
			const std::uint64_t after_addr = last_ip + all_instrs_[index].base.size;
			info.block.initialize_data(after_addr);
		} else
			ICED_DEBUG_ASSERT(info.end == 0);
	}

	const std::uint32_t options = benc_.options;
	const bool return_new_instruction_offsets = (options & BlockEncoderOptions::RETURN_NEW_INSTRUCTION_OFFSETS) != 0;
	const bool return_constant_offsets = (options & BlockEncoderOptions::RETURN_CONSTANT_OFFSETS) != 0;
	std::vector<BlockEncoderResult> result_vec;
	result_vec.reserve(blocks_.size());
	for (auto& info : blocks_) {
		const std::size_t instr_count = info.end - info.start;
		std::vector<std::uint32_t> new_instruction_offsets;
		if (return_new_instruction_offsets)
			new_instruction_offsets.reserve(instr_count);
		std::vector<ConstantOffsets> constant_offsets;
		if (return_constant_offsets)
			constant_offsets.reserve(instr_count);
		InstrContext ctx{info.block, all_ips_.data(), info.block.rip};
		for (std::size_t i = info.start; i < info.end; i++) {
			auto& entry = all_instrs_[i];
			const std::size_t buffer_pos = ctx.block.buffer_pos();
			auto result = entry.instr.encode(entry.base, ctx);
			if (result.is_err())
				return result.error();
			if (return_constant_offsets)
				constant_offsets.push_back(result.value().constant_offsets);
			const bool is_original_instruction = result.value().is_original_instruction;
			const std::size_t size = ctx.block.buffer_pos() - buffer_pos;
			if (size != entry.base.size)
				return IcedError("Internal error");
			if (return_new_instruction_offsets)
				new_instruction_offsets.push_back(is_original_instruction ? static_cast<std::uint32_t>(ctx.ip - ctx.block.rip) : UINT32_MAX);
			ctx.ip += size;
		}
		auto write_result = info.block.write_data();
		if (write_result.is_err())
			return write_result.error();
		result_vec.emplace_back();
		auto& result = result_vec.back();
		result.rip = info.block.rip;
		result.code_buffer = info.block.take_buffer();
		result.reloc_infos = info.block.take_reloc_infos();
		result.new_instruction_offsets = std::move(new_instruction_offsets);
		result.constant_offsets = std::move(constant_offsets);
		info.block.dispose();
	}

	return result_vec;
}

} // namespace
} // namespace internal

Result<BlockEncoderResult> BlockEncoder::encode(std::uint32_t bitness, const InstructionBlock& block, std::uint32_t options) {
	auto result = encode_slice(bitness, &block, 1, options);
	if (result.is_err())
		return result.error();
	auto& result_vec = result.value();
	ICED_DEBUG_ASSERT(result_vec.size() == 1);
	return std::move(result_vec[0]);
}

Result<std::vector<BlockEncoderResult>> BlockEncoder::encode_slice(std::uint32_t bitness, const InstructionBlock* blocks, std::size_t count,
	std::uint32_t options) {
	if (bitness != 16 && bitness != 32 && bitness != 64)
		return IcedError("Invalid bitness");
	internal::BlockEncoderImpl impl(bitness, options);
	auto init_result = impl.initialize(blocks, count);
	if (init_result.is_err())
		return init_result.error();
	return impl.encode2();
}

} // namespace iced_x86
