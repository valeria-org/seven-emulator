// Intel Key Locker (spec 343965-001): LOADIWKEY, ENCODEKEY128/256, AES{ENC,DEC}{128,256}KL and the
// WIDE variants. Handles are wrapped/unwrapped with the spec's AES-GCM-SIV construction (appendix A.5),
// so handles produced here are bit-identical to what hardware produces for the same IWKey.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "seven/handler_helpers.hpp"

#include "aes_internal.hpp"

namespace seven::handlers {

namespace {

constexpr std::uint64_t kCr4KeyLocker = 1ull << 19;

// Handle AAD (handle bytes [15:0]) fields.
constexpr std::uint32_t kHandleCpl0Only = 1u << 0;
constexpr std::uint32_t kHandleNoEncrypt = 1u << 1;
constexpr std::uint32_t kHandleNoDecrypt = 1u << 2;
constexpr std::uint32_t kHandleRestrictions = kHandleCpl0Only | kHandleNoEncrypt | kHandleNoDecrypt;
constexpr std::uint32_t kHandleKeyTypeShift = 24;
constexpr std::uint32_t kHandleKeyTypeMask = 0xFu << kHandleKeyTypeShift;

template <std::size_t KeyBytes>
using Handle = std::array<std::uint8_t, 32 + KeyBytes>;  // AAD | integrity tag | ciphertext

template <std::size_t KeyBytes>
constexpr std::uint32_t kHandleKeyType = KeyBytes == 16 ? 0u : 1u;

[[nodiscard]] ExecutionResult fault(ExecutionContext& ctx, StopReason reason) {
  return {reason, 0, ExceptionInfo{reason, ctx.state.rip, 0}, ctx.instr.code()};
}

[[nodiscard]] bool key_locker_enabled(const CpuState& state) { return (state.cr[4] & kCr4KeyLocker) != 0; }

[[nodiscard]] bool cpl_is_zero(const CpuState& state) { return (state.sreg[1] & 0x3u) == 0; }

[[nodiscard]] std::size_t xmm_index(iced_x86::Register reg) {
  return static_cast<std::size_t>(reg) - static_cast<std::size_t>(iced_x86::Register::XMM0);
}

[[nodiscard]] aes::Block read_xmm(const CpuState& state, std::size_t index) { return aes::load(state.vectors[index].value, 0); }

// Legacy-encoded instructions replace bits [127:0] and preserve the rest of the vector register.
void write_xmm(CpuState& state, std::size_t index, const aes::Block& block) {
  auto low_mask = ~SimdUint(0) >> (kVectorBits - 128);  // non-const: uintwide's operator~ is non-const
  auto& slot = state.vectors[index].value;
  slot = (slot & ~low_mask) | aes::to_bits(block);
}

// All Key Locker instructions clear OF/SF/AF/PF/CF; ZF reports failure.
void set_flags(CpuState& state, bool zf) {
  state.rflags &= ~(kFlagCF | kFlagPF | kFlagAF | kFlagZF | kFlagSF | kFlagOF);
  if (zf) state.rflags |= kFlagZF;
}

[[nodiscard]] std::uint32_t load_u32(const std::uint8_t* bytes) {
  return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
         (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

struct U128 {
  std::uint64_t lo = 0;
  std::uint64_t hi = 0;
};

[[nodiscard]] U128 load_u128(const std::uint8_t* bytes) {
  U128 out{};
  for (std::size_t i = 0; i < 8; ++i) {
    out.lo |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
    out.hi |= static_cast<std::uint64_t>(bytes[i + 8]) << (i * 8);
  }
  return out;
}

[[nodiscard]] aes::Block to_block(U128 value) {
  aes::Block out{};
  for (std::size_t i = 0; i < 8; ++i) {
    out[i] = static_cast<std::uint8_t>(value.lo >> (i * 8));
    out[i + 8] = static_cast<std::uint8_t>(value.hi >> (i * 8));
  }
  return out;
}

[[nodiscard]] U128 clmul64(std::uint64_t a, std::uint64_t b) {
  U128 out{};
  for (unsigned i = 0; i < 64; ++i) {
    if (((b >> i) & 1u) == 0u) continue;
    out.lo ^= a << i;
    if (i != 0) out.hi ^= a >> (64 - i);
  }
  return out;
}

// POLYVAL step (RFC 8452): (a ^ b) * h * x^-128. Mirrors the spec's Horner_Step PCLMULQDQ sequence.
[[nodiscard]] U128 horner_step(U128 a, U128 b, U128 h) {
  a.lo ^= b.lo;
  a.hi ^= b.hi;
  const auto low_product = clmul64(a.lo, h.lo);
  const auto high_product = clmul64(a.hi, h.hi);
  auto middle = clmul64(a.lo, h.hi);
  const auto middle2 = clmul64(a.hi, h.lo);
  middle.lo ^= middle2.lo;
  middle.hi ^= middle2.hi;
  U128 low{low_product.lo, low_product.hi ^ middle.lo};
  const U128 high{high_product.lo ^ middle.hi, high_product.hi};
  constexpr std::uint64_t kPolyHigh = 0xC200000000000000ull;
  for (int i = 0; i < 2; ++i) {
    const auto folded = clmul64(low.lo, kPolyHigh);
    low = U128{low.hi ^ folded.lo, low.lo ^ folded.hi};
  }
  return {high.lo ^ low.lo, high.hi ^ low.hi};
}

// SIV tag: AES256(EncryptionKey, POLYVAL(IntegrityKey, AAD || plaintext || lengths) with bit 127 cleared).
[[nodiscard]] aes::Block siv_tag(const KeyLockerIwKey& iwkey, const aes::KeySchedule& schedule, const std::uint8_t* aad,
                                 std::span<const std::uint8_t> plaintext) {
  const auto h = load_u128(iwkey.integrity_key.data());
  auto s = horner_step(load_u128(aad), U128{}, h);
  for (std::size_t offset = 0; offset < plaintext.size(); offset += 16) {
    s = horner_step(load_u128(plaintext.data() + offset), s, h);
  }
  s = horner_step(s, U128{16 * 8, plaintext.size() * 8}, h);
  s.hi &= ~(1ull << 63);
  return aes::encrypt(schedule, to_block(s));
}

// CTR keystream: counter = tag with bit 127 set, block index added to the low dword (32-bit wrap).
[[nodiscard]] aes::Block keystream_block(const aes::KeySchedule& schedule, const aes::Block& tag, std::uint32_t index) {
  auto counter = tag;
  counter[15] |= 0x80u;
  const auto low = load_u32(counter.data()) + index;
  for (std::size_t i = 0; i < 4; ++i) counter[i] = static_cast<std::uint8_t>(low >> (i * 8));
  return aes::encrypt(schedule, counter);
}

template <std::size_t KeyBytes>
[[nodiscard]] Handle<KeyBytes> wrap_key(const KeyLockerIwKey& iwkey, std::uint32_t metadata, const std::array<std::uint8_t, KeyBytes>& key) {
  const auto schedule = aes::expand_key(iwkey.encryption_key);
  Handle<KeyBytes> handle{};
  for (std::size_t i = 0; i < 4; ++i) handle[i] = static_cast<std::uint8_t>(metadata >> (i * 8));
  const auto tag = siv_tag(iwkey, schedule, handle.data(), key);
  for (std::size_t i = 0; i < 16; ++i) handle[16 + i] = tag[i];
  for (std::size_t block = 0; block < KeyBytes / 16; ++block) {
    const auto stream = keystream_block(schedule, tag, static_cast<std::uint32_t>(block));
    for (std::size_t i = 0; i < 16; ++i) handle[32 + 16 * block + i] = static_cast<std::uint8_t>(stream[i] ^ key[16 * block + i]);
  }
  return handle;
}

template <std::size_t KeyBytes>
[[nodiscard]] std::optional<std::array<std::uint8_t, KeyBytes>> unwrap_key(const KeyLockerIwKey& iwkey, const Handle<KeyBytes>& handle) {
  const auto schedule = aes::expand_key(iwkey.encryption_key);
  aes::Block tag{};
  for (std::size_t i = 0; i < 16; ++i) tag[i] = handle[16 + i];
  std::array<std::uint8_t, KeyBytes> key{};
  for (std::size_t block = 0; block < KeyBytes / 16; ++block) {
    const auto stream = keystream_block(schedule, tag, static_cast<std::uint32_t>(block));
    for (std::size_t i = 0; i < 16; ++i) key[16 * block + i] = static_cast<std::uint8_t>(stream[i] ^ handle[32 + 16 * block + i]);
  }
  if (siv_tag(iwkey, schedule, handle.data(), key) != tag) return std::nullopt;
  return key;
}

// Restriction/format checks performed before unwrapping (reserved AAD bits 127:28 and 23:3,
// key type, CPL0-only, and the no-encrypt/no-decrypt restriction relevant to this operation).
template <std::size_t KeyBytes>
[[nodiscard]] bool handle_usable(const CpuState& state, const Handle<KeyBytes>& handle, std::uint32_t forbidden_restriction) {
  const auto aad = load_u32(handle.data());
  if ((aad & ~(kHandleRestrictions | kHandleKeyTypeMask)) != 0) return false;
  for (std::size_t i = 4; i < 16; ++i) {
    if (handle[i] != 0) return false;
  }
  if ((aad & kHandleCpl0Only) != 0 && !cpl_is_zero(state)) return false;
  if ((aad & forbidden_restriction) != 0) return false;
  return ((aad & kHandleKeyTypeMask) >> kHandleKeyTypeShift) == kHandleKeyType<KeyBytes>;
}

// AES{ENC,DEC}{128,256}KL xmm, m384/m512 and AES{ENC,DEC}WIDE{128,256}KL m384/m512 (XMM0-7).
template <std::size_t KeyBytes, bool Encrypt, bool Wide>
ExecutionResult aes_key_locker(ExecutionContext& ctx) {
  if (!key_locker_enabled(ctx.state)) return fault(ctx, StopReason::invalid_opcode);
  Handle<KeyBytes> handle{};
  const auto read = detail::read_memory_checked(ctx, detail::memory_address(ctx), handle.data(), handle.size());
  if (!read.ok()) return read;

  const auto key = handle_usable<KeyBytes>(ctx.state, handle, Encrypt ? kHandleNoEncrypt : kHandleNoDecrypt)
                       ? unwrap_key<KeyBytes>(ctx.state.iwkey, handle)
                       : std::nullopt;
  set_flags(ctx.state, !key.has_value());
  if (!key) return {};

  const auto schedule = aes::expand_key(*key);
  const auto crypt = [&](std::size_t index) {
    const auto block = read_xmm(ctx.state, index);
    write_xmm(ctx.state, index, Encrypt ? aes::encrypt(schedule, block) : aes::decrypt(schedule, block));
  };
  if constexpr (Wide) {
    for (std::size_t index = 0; index < 8; ++index) crypt(index);
  } else {
    crypt(xmm_index(ctx.instr.op_register(0)));
  }
  return {};
}

// ENCODEKEY128 r32, r32 (key in XMM0, handle -> XMM0-2) / ENCODEKEY256 (key in XMM1:XMM0, handle -> XMM0-3).
template <std::size_t KeyBytes>
ExecutionResult encode_key(ExecutionContext& ctx) {
  if (!key_locker_enabled(ctx.state)) return fault(ctx, StopReason::invalid_opcode);
  const auto restrictions = static_cast<std::uint32_t>(detail::read_register(ctx.state, ctx.instr.op_register(1)));
  if ((restrictions & ~kHandleRestrictions) != 0) return fault(ctx, StopReason::general_protection);

  std::array<std::uint8_t, KeyBytes> key{};
  for (std::size_t block = 0; block < KeyBytes / 16; ++block) {
    const auto source = read_xmm(ctx.state, block);
    for (std::size_t i = 0; i < 16; ++i) key[16 * block + i] = source[i];
  }
  const auto& iwkey = ctx.state.iwkey;
  const auto handle = wrap_key<KeyBytes>(iwkey, restrictions | (kHandleKeyType<KeyBytes> << kHandleKeyTypeShift), key);
  for (std::size_t block = 0; block < handle.size() / 16; ++block) {
    aes::Block out{};
    for (std::size_t i = 0; i < 16; ++i) out[i] = handle[16 * block + i];
    write_xmm(ctx.state, block, out);
  }
  for (std::size_t index = 4; index <= 6; ++index) write_xmm(ctx.state, index, aes::Block{});

  const auto key_info = (iwkey.no_backup ? 1u : 0u) | (static_cast<std::uint32_t>(iwkey.key_source) << 1);
  detail::write_register(ctx.state, ctx.instr.op_register(0), key_info, 4);
  set_flags(ctx.state, false);
  return {};
}

}  // namespace

// LOADIWKEY xmm1, xmm2, <EAX>, <XMM0>: EncryptionKey = xmm1:xmm2, IntegrityKey = XMM0.
// KeySource 1 (hardware-randomized IWKey) is not enumerated (CPUID.19H:ECX[1] = 0), so it #GPs like
// on any part without it; keeping the IWKey software-defined keeps emulation deterministic.
ExecutionResult handle_code_LOADIWKEY_XMM_XMM(ExecutionContext& ctx) {
  if (!key_locker_enabled(ctx.state)) return fault(ctx, StopReason::invalid_opcode);
  if (!cpl_is_zero(ctx.state)) return fault(ctx, StopReason::general_protection);
  const auto control = static_cast<std::uint32_t>(ctx.state.gpr[0]);
  if ((control & ~1u) != 0) return fault(ctx, StopReason::general_protection);

  const auto high = read_xmm(ctx.state, xmm_index(ctx.instr.op_register(0)));
  const auto low = read_xmm(ctx.state, xmm_index(ctx.instr.op_register(1)));
  const auto integrity = read_xmm(ctx.state, 0);
  auto& iwkey = ctx.state.iwkey;
  for (std::size_t i = 0; i < 16; ++i) {
    iwkey.encryption_key[i] = low[i];
    iwkey.encryption_key[16 + i] = high[i];
    iwkey.integrity_key[i] = integrity[i];
  }
  iwkey.no_backup = (control & 1u) != 0;
  iwkey.key_source = 0;
  set_flags(ctx.state, false);
  return {};
}

ExecutionResult handle_code_ENCODEKEY128_R32_R32(ExecutionContext& ctx) { return encode_key<16>(ctx); }
ExecutionResult handle_code_ENCODEKEY256_R32_R32(ExecutionContext& ctx) { return encode_key<32>(ctx); }

ExecutionResult handle_code_AESENC128KL_XMM_M384(ExecutionContext& ctx) { return aes_key_locker<16, true, false>(ctx); }
ExecutionResult handle_code_AESDEC128KL_XMM_M384(ExecutionContext& ctx) { return aes_key_locker<16, false, false>(ctx); }
ExecutionResult handle_code_AESENC256KL_XMM_M512(ExecutionContext& ctx) { return aes_key_locker<32, true, false>(ctx); }
ExecutionResult handle_code_AESDEC256KL_XMM_M512(ExecutionContext& ctx) { return aes_key_locker<32, false, false>(ctx); }
ExecutionResult handle_code_AESENCWIDE128KL_M384(ExecutionContext& ctx) { return aes_key_locker<16, true, true>(ctx); }
ExecutionResult handle_code_AESDECWIDE128KL_M384(ExecutionContext& ctx) { return aes_key_locker<16, false, true>(ctx); }
ExecutionResult handle_code_AESENCWIDE256KL_M512(ExecutionContext& ctx) { return aes_key_locker<32, true, true>(ctx); }
ExecutionResult handle_code_AESDECWIDE256KL_M512(ExecutionContext& ctx) { return aes_key_locker<32, false, true>(ctx); }

}  // namespace seven::handlers
