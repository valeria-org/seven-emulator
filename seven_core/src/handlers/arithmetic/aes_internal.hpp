#pragma once

// Shared AES primitives for the AES-NI/VAES handlers (simd_int.cpp) and Key Locker (key_locker.cpp).
// Intel's AES state is the 16 bytes of a 128-bit lane, byte 0 = bits [7:0], laid out column-major:
// state[row][col] = byte[row + 4 * col].

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "seven/types.hpp"

namespace seven::handlers::aes {

using Block = std::array<std::uint8_t, 16>;

constexpr std::uint8_t xtime(std::uint8_t value) {
  return static_cast<std::uint8_t>((value << 1) ^ ((value & 0x80u) != 0u ? 0x1Bu : 0x00u));
}

struct Sboxes {
  std::array<std::uint8_t, 256> forward{};
  std::array<std::uint8_t, 256> inverse{};
};

// FIPS-197 S-box: GF(2^8) multiplicative inverse (via log/antilog over generator 3) + affine map.
constexpr Sboxes make_sboxes() {
  std::array<std::uint8_t, 256> antilog{};
  std::array<std::uint8_t, 256> log{};
  std::uint8_t power = 1;
  for (unsigned i = 0; i < 255; ++i) {
    antilog[i] = power;
    log[power] = static_cast<std::uint8_t>(i);
    power = static_cast<std::uint8_t>(power ^ xtime(power));
  }
  Sboxes boxes{};
  for (unsigned x = 0; x < 256; ++x) {
    const auto inv = x == 0 ? std::uint8_t{0} : antilog[(255u - log[x]) % 255u];
    const auto s = static_cast<std::uint8_t>(inv ^ std::rotl(inv, 1) ^ std::rotl(inv, 2) ^ std::rotl(inv, 3) ^ std::rotl(inv, 4) ^ 0x63u);
    boxes.forward[x] = s;
    boxes.inverse[s] = static_cast<std::uint8_t>(x);
  }
  return boxes;
}

inline constexpr Sboxes kSboxes = make_sboxes();

inline Block load(const SimdUint& bits, std::size_t byte_offset) {
  const auto lo = static_cast<std::uint64_t>(bits >> (byte_offset * 8));
  const auto hi = static_cast<std::uint64_t>(bits >> (byte_offset * 8 + 64));
  Block out{};
  for (std::size_t i = 0; i < 8; ++i) {
    out[i] = static_cast<std::uint8_t>(lo >> (i * 8));
    out[i + 8] = static_cast<std::uint8_t>(hi >> (i * 8));
  }
  return out;
}

// The block as the low 128 bits of a vector value.
inline SimdUint to_bits(const Block& block) {
  std::uint64_t lo = 0;
  std::uint64_t hi = 0;
  for (std::size_t i = 0; i < 8; ++i) {
    lo |= static_cast<std::uint64_t>(block[i]) << (i * 8);
    hi |= static_cast<std::uint64_t>(block[i + 8]) << (i * 8);
  }
  return SimdUint(lo) | (SimdUint(hi) << 64);
}

// ShiftRows then SubBytes (they commute).
inline Block shift_sub(const Block& state) {
  Block out{};
  for (std::size_t col = 0; col < 4; ++col) {
    for (std::size_t row = 0; row < 4; ++row) {
      out[row + 4 * col] = kSboxes.forward[state[row + 4 * ((col + row) & 3u)]];
    }
  }
  return out;
}

// InvShiftRows then InvSubBytes.
inline Block inv_shift_sub(const Block& state) {
  Block out{};
  for (std::size_t col = 0; col < 4; ++col) {
    for (std::size_t row = 0; row < 4; ++row) {
      out[row + 4 * col] = kSboxes.inverse[state[row + 4 * ((col + 4 - row) & 3u)]];
    }
  }
  return out;
}

inline Block mix_columns(Block state) {
  for (std::size_t col = 0; col < 16; col += 4) {
    const auto a0 = state[col];
    const auto a1 = state[col + 1];
    const auto a2 = state[col + 2];
    const auto a3 = state[col + 3];
    const auto all = static_cast<std::uint8_t>(a0 ^ a1 ^ a2 ^ a3);
    state[col] = static_cast<std::uint8_t>(a0 ^ all ^ xtime(static_cast<std::uint8_t>(a0 ^ a1)));
    state[col + 1] = static_cast<std::uint8_t>(a1 ^ all ^ xtime(static_cast<std::uint8_t>(a1 ^ a2)));
    state[col + 2] = static_cast<std::uint8_t>(a2 ^ all ^ xtime(static_cast<std::uint8_t>(a2 ^ a3)));
    state[col + 3] = static_cast<std::uint8_t>(a3 ^ all ^ xtime(static_cast<std::uint8_t>(a3 ^ a0)));
  }
  return state;
}

// InvMixColumns = MixColumns after multiplying each column by {05 00 04 00} (circulant).
inline Block inv_mix_columns(Block state) {
  for (std::size_t col = 0; col < 16; col += 4) {
    const auto u = xtime(xtime(static_cast<std::uint8_t>(state[col] ^ state[col + 2])));
    const auto v = xtime(xtime(static_cast<std::uint8_t>(state[col + 1] ^ state[col + 3])));
    state[col] ^= u;
    state[col + 1] ^= v;
    state[col + 2] ^= u;
    state[col + 3] ^= v;
  }
  return mix_columns(state);
}

inline Block xor_blocks(Block lhs, const Block& rhs) {
  for (std::size_t i = 0; i < lhs.size(); ++i) lhs[i] ^= rhs[i];
  return lhs;
}

// AESENC / AESENCLAST.
template <bool Last>
Block enc_round(const Block& state, const Block& round_key) {
  auto out = shift_sub(state);
  if constexpr (!Last) out = mix_columns(out);
  return xor_blocks(out, round_key);
}

// AESDEC / AESDECLAST (equivalent inverse cipher round).
template <bool Last>
Block dec_round(const Block& state, const Block& round_key) {
  auto out = inv_shift_sub(state);
  if constexpr (!Last) out = inv_mix_columns(out);
  return xor_blocks(out, round_key);
}

inline Block keygen_assist(const Block& src, std::uint32_t rcon) {
  const auto sub_word = [&src](std::size_t dword) {
    std::uint32_t out = 0;
    for (std::size_t i = 0; i < 4; ++i) {
      out |= static_cast<std::uint32_t>(kSboxes.forward[src[dword * 4 + i]]) << (i * 8);
    }
    return out;
  };
  const auto x1 = sub_word(1);
  const auto x3 = sub_word(3);
  const std::array<std::uint32_t, 4> dwords{x1, std::rotr(x1, 8) ^ rcon, x3, std::rotr(x3, 8) ^ rcon};
  Block out{};
  for (std::size_t i = 0; i < 16; ++i) {
    out[i] = static_cast<std::uint8_t>(dwords[i / 4] >> ((i % 4) * 8));
  }
  return out;
}

// Full-cipher support (Key Locker): FIPS-197 key expansion for 128/256-bit keys.
struct KeySchedule {
  std::array<Block, 15> round_keys{};
  std::size_t rounds = 0;
};

inline KeySchedule expand_key(std::span<const std::uint8_t> key) {
  const std::size_t nk = key.size() / 4;  // 4 (AES-128) or 8 (AES-256)
  KeySchedule schedule{};
  schedule.rounds = nk + 6;
  const std::size_t total_words = 4 * (schedule.rounds + 1);
  std::array<std::array<std::uint8_t, 4>, 60> words{};
  for (std::size_t i = 0; i < nk; ++i) {
    words[i] = {key[4 * i], key[4 * i + 1], key[4 * i + 2], key[4 * i + 3]};
  }
  std::uint8_t rcon = 1;
  for (std::size_t i = nk; i < total_words; ++i) {
    auto temp = words[i - 1];
    if (i % nk == 0) {
      temp = {static_cast<std::uint8_t>(kSboxes.forward[temp[1]] ^ rcon), kSboxes.forward[temp[2]], kSboxes.forward[temp[3]],
              kSboxes.forward[temp[0]]};
      rcon = xtime(rcon);
    } else if (nk > 6 && i % nk == 4) {
      for (auto& byte : temp) byte = kSboxes.forward[byte];
    }
    for (std::size_t b = 0; b < 4; ++b) words[i][b] = static_cast<std::uint8_t>(words[i - nk][b] ^ temp[b]);
  }
  for (std::size_t round = 0; round <= schedule.rounds; ++round) {
    for (std::size_t i = 0; i < 16; ++i) schedule.round_keys[round][i] = words[4 * round + i / 4][i % 4];
  }
  return schedule;
}

inline Block encrypt(const KeySchedule& schedule, Block block) {
  block = xor_blocks(block, schedule.round_keys[0]);
  for (std::size_t round = 1; round < schedule.rounds; ++round) block = enc_round<false>(block, schedule.round_keys[round]);
  return enc_round<true>(block, schedule.round_keys[schedule.rounds]);
}

inline Block decrypt(const KeySchedule& schedule, Block block) {
  block = xor_blocks(block, schedule.round_keys[schedule.rounds]);
  for (std::size_t round = schedule.rounds - 1; round > 0; --round) {
    block = dec_round<false>(block, inv_mix_columns(schedule.round_keys[round]));
  }
  return dec_round<true>(block, schedule.round_keys[0]);
}

}  // namespace seven::handlers::aes
