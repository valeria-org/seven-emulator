#include <gtest/gtest.h>

#include <cmath>

#include <iced_x86/code.hpp>
#include <iced_x86/instruction.hpp>
#include <iced_x86/memory_operand.hpp>
#include <iced_x86/register.hpp>

#include "kubera_test_support.hpp"

namespace {

using namespace kubera::test;

TEST(KuberaSimd, VpAndUsesExplicitSources) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with3(
      iced_x86::Code::VEX_Vpand_xmm_xmm_xmmm128,
      iced_x86::Register::XMM0,
      iced_x86::Register::XMM1,
      iced_x86::Register::XMM2).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "vpand xmm0, xmm1, xmm2"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_xmm_u64(state, 0, 0xFFFF'FFFF'FFFF'FFFFull, 0xFFFF'FFFF'FFFF'FFFFull);
               set_xmm_u64(state, 1, 0xFF00'FF00'0F0F'0F0Full, 0xAAAA'5555'1234'5678ull);
               set_xmm_u64(state, 2, 0x0F0F'F0F0'FFFF'0000ull, 0xFFFF'0000'FFFF'0000ull);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(xmm_u64(state, 0, 0), 0x0F00'F000'0F0F'0000ull);
               EXPECT_EQ(xmm_u64(state, 0, 1), 0xAAAA'0000'1234'0000ull);
             });
}

TEST(KuberaSimd, VaddssPreservesUpperLanesFromSrc1) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with3(
      iced_x86::Code::VEX_Vaddss_xmm_xmm_xmmm32,
      iced_x86::Register::XMM0,
      iced_x86::Register::XMM1,
      iced_x86::Register::XMM2).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "vaddss xmm0, xmm1, xmm2"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_xmm_u32x4(state,
                             0,
                             std::bit_cast<std::uint32_t>(100.0f),
                             0xDEAD'BEEFu,
                             0xCAFEBABEu,
                             0x1357'2468u);
               set_xmm_u32x4(state,
                             1,
                             std::bit_cast<std::uint32_t>(1.5f),
                             0x1122'3344u,
                             0x5566'7788u,
                             0x99AA'BBCCu);
               set_xmm_u32x4(state,
                             2,
                             std::bit_cast<std::uint32_t>(2.25f),
                             0x0102'0304u,
                             0x0506'0708u,
                             0x090A'0B0Cu);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_FLOAT_EQ(xmm_scalar<float>(state, 0), 3.75f);
               EXPECT_EQ(xmm_u32(state, 0, 1), 0x1122'3344u);
               EXPECT_EQ(xmm_u32(state, 0, 2), 0x5566'7788u);
               EXPECT_EQ(xmm_u32(state, 0, 3), 0x99AA'BBCCu);
             });
}

TEST(KuberaSimd, SseCvtsi2ssConvertsAndPreservesUpperLanes) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Cvtsi2ss_xmm_rm64,
      iced_x86::Register::XMM0,
      iced_x86::Register::RAX).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "cvtsi2ss xmm0, rax"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               state.gpr[0] = 42;
               set_xmm_u32x4(state, 0, 0xDEAD'BEEFu, 0x1122'3344u, 0x5566'7788u, 0x99AA'BBCCu);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_FLOAT_EQ(xmm_scalar<float>(state, 0), 42.0f);
               EXPECT_EQ(xmm_u32(state, 0, 1), 0x1122'3344u);
               EXPECT_EQ(xmm_u32(state, 0, 2), 0x5566'7788u);
               EXPECT_EQ(xmm_u32(state, 0, 3), 0x99AA'BBCCu);
             });
}


TEST(KuberaSimd, Sse2Cvttsd2siTruncates) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Cvttsd2si_r64_xmmm64,
      iced_x86::Register::RAX,
      iced_x86::Register::XMM1).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "cvttsd2si rax, xmm1"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_xmm_scalar<double>(state, 1, 3.75);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(state.gpr[0], 3u);
             });
}

TEST(KuberaSimd, Sse2PextrwExtractsSelectedWord) {
  std::vector<std::uint8_t> bytes{0x66, 0x0F, 0xC5, 0xC1, 0x01};

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               state.gpr[0] = 0xFFFF'FFFF'FFFF'FFFFull;
               set_xmm_u32x4(state, 1, 0xABCD'5678u, 0, 0, 0);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(state.gpr[0], 0xABCDu);
             });
}


TEST(KuberaSimd, Sse3HaddpsProducesHorizontalSums) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Haddps_xmm_xmmm128,
      iced_x86::Register::XMM0,
      iced_x86::Register::XMM1).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "haddps xmm0, xmm1"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_xmm_u32x4(state,
                             0,
                             std::bit_cast<std::uint32_t>(1.0f),
                             std::bit_cast<std::uint32_t>(2.0f),
                             std::bit_cast<std::uint32_t>(10.0f),
                             std::bit_cast<std::uint32_t>(20.0f));
               set_xmm_u32x4(state,
                             1,
                             std::bit_cast<std::uint32_t>(3.0f),
                             std::bit_cast<std::uint32_t>(4.0f),
                             std::bit_cast<std::uint32_t>(30.0f),
                             std::bit_cast<std::uint32_t>(40.0f));
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_FLOAT_EQ(xmm_scalar<float>(state, 0), 3.0f);
               EXPECT_FLOAT_EQ(std::bit_cast<float>(xmm_u32(state, 0, 1)), 30.0f);
               EXPECT_FLOAT_EQ(std::bit_cast<float>(xmm_u32(state, 0, 2)), 7.0f);
               EXPECT_FLOAT_EQ(std::bit_cast<float>(xmm_u32(state, 0, 3)), 70.0f);
             });
}

TEST(KuberaSimd, Sse42Crc32MatchesCastagnoliReference) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Crc32_r64_rm64,
      iced_x86::Register::RAX,
      iced_x86::Register::RBX).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "crc32 rax, rbx"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               state.gpr[0] = 0x1234'5678u;
               state.gpr[3] = 0x0123'4567'89AB'CDEFu;
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               const auto expected = static_cast<std::uint64_t>(crc32c_update(0x1234'5678u, 0x0123'4567'89AB'CDEFu, 8));
               EXPECT_EQ(state.gpr[0], expected);
             });
}

// AES state blocks below are FIPS-197 Appendix B (cipher example) intermediates, byte 0 first.
std::uint64_t aes_block_u64(std::string_view hex, std::size_t lane) {
  const auto bytes = seven::parse_hex_bytes(hex);
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(bytes[lane * 8 + i]) << (i * 8);
  return value;
}

void set_aes_block(seven::CpuState& state, std::size_t index, std::string_view hex) {
  set_xmm_u64(state, index, aes_block_u64(hex, 0), aes_block_u64(hex, 1));
}

constexpr std::string_view kFipsRound1Start = "193de3bea0f4e22b9ac68d2ae9f84808";
constexpr std::string_view kFipsRound1AfterShiftRows = "d4bf5d30e0b452aeb84111f11e2798e5";
constexpr std::string_view kFipsRound1AfterMixColumns = "046681e5e0cb199a48f8d37a2806264c";
constexpr std::string_view kFipsRound1Key = "a0fafe1788542cb123a339392a6c7605";
constexpr std::string_view kFipsRound2Start = "a49c7ff2689f352b6b5bea43026a5049";

TEST(KuberaSimd, AesencMatchesFips197RoundAndPreservesUpperBits) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Aesenc_xmm_xmmm128,
      iced_x86::Register::XMM5,
      iced_x86::Register::XMM1).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aesenc xmm5, xmm1"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_aes_block(state, 5, kFipsRound1Start);
               set_aes_block(state, 1, kFipsRound1Key);
#if SEVEN_VECTOR_BITS > 128
               state.vectors[5].value |= seven::SimdUint(0x0123'4567'89AB'CDEFull) << 128;
#endif
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(xmm_u64(state, 5, 0), aes_block_u64(kFipsRound2Start, 0));
               EXPECT_EQ(xmm_u64(state, 5, 1), aes_block_u64(kFipsRound2Start, 1));
#if SEVEN_VECTOR_BITS > 128
               EXPECT_EQ(xmm_u64(state, 5, 2), 0x0123'4567'89AB'CDEFull);
#endif
             });
}

TEST(KuberaSimd, AesimcInvertsMixColumns) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Aesimc_xmm_xmmm128,
      iced_x86::Register::XMM0,
      iced_x86::Register::XMM1).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aesimc xmm0, xmm1"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) { set_aes_block(state, 1, kFipsRound1AfterMixColumns); },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(xmm_u64(state, 0, 0), aes_block_u64(kFipsRound1AfterShiftRows, 0));
               EXPECT_EQ(xmm_u64(state, 0, 1), aes_block_u64(kFipsRound1AfterShiftRows, 1));
             });
}

TEST(KuberaSimd, AeskeygenassistMatchesFips197FirstExpansionWord) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with3(
      iced_x86::Code::Aeskeygenassist_xmm_xmmm128_imm8,
      iced_x86::Register::XMM0,
      iced_x86::Register::XMM1,
      0x01).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aeskeygenassist xmm0, xmm1, 1"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) { set_aes_block(state, 1, "2b7e151628aed2a6abf7158809cf4f3c"); },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               // FIPS-197 A.1, i = 4: SubWord(w3) = 01 8a 84 eb; SubWord(RotWord(w3)) ^ Rcon = 8b 84 eb 01.
               EXPECT_EQ(xmm_u32(state, 0, 2), 0xEB84'8A01u);
               EXPECT_EQ(xmm_u32(state, 0, 3), 0x01EB'848Bu);
             });
}

#if SEVEN_VECTOR_BITS >= 256
TEST(KuberaSimd, VaesdeclastYmmUndoesShiftRowsSubBytesPerLane) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with3(
      iced_x86::Code::VEX_Vaesdeclast_ymm_ymm_ymmm256,
      iced_x86::Register::YMM0,
      iced_x86::Register::YMM1,
      iced_x86::Register::YMM2).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "vaesdeclast ymm0, ymm1, ymm2"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               set_aes_block(state, 1, kFipsRound1AfterShiftRows);
               state.vectors[1].value |= state.vectors[1].value << 128;
               // Lane 1 round key = 1 in byte 0, lane 0 key = 0: lanes must be processed independently.
               state.vectors[2].value = seven::SimdUint(1) << 128;
               state.vectors[0].value = ~seven::SimdUint(0);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               EXPECT_EQ(xmm_u64(state, 0, 0), aes_block_u64(kFipsRound1Start, 0));
               EXPECT_EQ(xmm_u64(state, 0, 1), aes_block_u64(kFipsRound1Start, 1));
               EXPECT_EQ(xmm_u64(state, 0, 2), aes_block_u64(kFipsRound1Start, 0) ^ 1u);
               EXPECT_EQ(xmm_u64(state, 0, 3), aes_block_u64(kFipsRound1Start, 1));
#if SEVEN_VECTOR_BITS > 256
               EXPECT_EQ(xmm_u64(state, 0, 4), 0u);
#endif
             });
}
#endif

// Key Locker. Test IWKey: IntegrityKey = 10..1f, EncryptionKey = 20..3f. The expected handles were
// produced by the Intel Key Locker spec's reference wrap code (343965-001, appendix A.5) for that IWKey,
// wrapping the FIPS-197 C.1 / C.3 AES keys 00..0f / 00..1f with no restrictions.
constexpr std::string_view kKlIntegrityKey = "101112131415161718191a1b1c1d1e1f";
constexpr std::string_view kKlEncryptionKeyLow = "202122232425262728292a2b2c2d2e2f";
constexpr std::string_view kKlEncryptionKeyHigh = "303132333435363738393a3b3c3d3e3f";
constexpr std::string_view kKlHandle128 =
    "00000000000000000000000000000000d539910383cfa15cbc83be836ca3d0db04476b2b0b711cf265e399e1e72d4226";
constexpr std::string_view kKlHandle256 =
    "000000010000000000000000000000003c85f1f1c888d9a2c247f9cbb5477c3289188112764749f48f44bb73792c47e3"
    "8cfcf6bb29ff54e6d0f58605bda8ea10";
constexpr std::string_view kFipsPlaintext = "00112233445566778899aabbccddeeff";
constexpr std::string_view kFipsAes128Ciphertext = "69c4e0d86a7b0430d8cdb78070b4c55a";
constexpr std::string_view kFipsAes256Ciphertext = "8ea2b7ca516745bfeafc49904b496089";
constexpr std::uint64_t kHandleAddress = 0x3000;

void enable_key_locker_with_test_iwkey(seven::CpuState& state) {
  state.cr[4] |= 1ull << 19;  // CR4.KL
  const auto integrity = seven::parse_hex_bytes(kKlIntegrityKey);
  const auto low = seven::parse_hex_bytes(kKlEncryptionKeyLow);
  const auto high = seven::parse_hex_bytes(kKlEncryptionKeyHigh);
  for (std::size_t i = 0; i < 16; ++i) {
    state.iwkey.integrity_key[i] = integrity[i];
    state.iwkey.encryption_key[i] = low[i];
    state.iwkey.encryption_key[16 + i] = high[i];
  }
}

void map_handle(seven::Memory& memory, std::string_view hex, std::size_t flip_byte = 0, std::uint8_t flip_mask = 0) {
  auto handle = seven::parse_hex_bytes(hex);
  handle[flip_byte] ^= flip_mask;
  memory.map(kHandleAddress, 0x1000);
  ASSERT_TRUE(memory.write(kHandleAddress, handle.data(), handle.size()));
}

void expect_xmm_block(const seven::CpuState& state, std::size_t index, std::string_view hex) {
  EXPECT_EQ(xmm_u64(state, index, 0), aes_block_u64(hex, 0)) << "xmm" << index;
  EXPECT_EQ(xmm_u64(state, index, 1), aes_block_u64(hex, 1)) << "xmm" << index;
}

TEST(KuberaSimd, LoadiwkeyLoadsWrappingKey) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Loadiwkey_xmm_xmm, iced_x86::Register::XMM1, iced_x86::Register::XMM2).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "loadiwkey xmm1, xmm2"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               state.cr[4] |= 1ull << 19;
               set_aes_block(state, 0, kKlIntegrityKey);
               set_aes_block(state, 1, kKlEncryptionKeyHigh);
               set_aes_block(state, 2, kKlEncryptionKeyLow);
               state.gpr[0] = 1;  // NoBackup, KeySource 0
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               seven::CpuState expected{};
               enable_key_locker_with_test_iwkey(expected);
               EXPECT_EQ(state.iwkey.integrity_key, expected.iwkey.integrity_key);
               EXPECT_EQ(state.iwkey.encryption_key, expected.iwkey.encryption_key);
               EXPECT_TRUE(state.iwkey.no_backup);
               EXPECT_EQ(state.rflags & seven::kFlagZF, 0u);
             });
}

TEST(KuberaSimd, Encodekey128ProducesSpecHandle) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Encodekey128_r32_r32, iced_x86::Register::EDX, iced_x86::Register::ECX).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "encodekey128 edx, ecx"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               enable_key_locker_with_test_iwkey(state);
               set_aes_block(state, 0, "000102030405060708090a0b0c0d0e0f");
               set_xmm_u64(state, 5, ~0ull, ~0ull);
               state.gpr[1] = 0;
               state.gpr[2] = ~0ull;
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               for (std::size_t i = 0; i < 3; ++i) expect_xmm_block(state, i, kKlHandle128.substr(32 * i, 32));
               expect_xmm_block(state, 5, "00000000000000000000000000000000");
               EXPECT_EQ(state.gpr[2], 0u);  // NoBackup = 0, KeySource = 0, zero-extended
             });
}

TEST(KuberaSimd, Encodekey256ProducesSpecHandle) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Encodekey256_r32_r32, iced_x86::Register::EDX, iced_x86::Register::ECX).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "encodekey256 edx, ecx"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory&) {
               enable_key_locker_with_test_iwkey(state);
               set_aes_block(state, 0, "000102030405060708090a0b0c0d0e0f");
               set_aes_block(state, 1, "101112131415161718191a1b1c1d1e1f");
               state.gpr[1] = 0;
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               for (std::size_t i = 0; i < 4; ++i) expect_xmm_block(state, i, kKlHandle256.substr(32 * i, 32));
             });
}

TEST(KuberaSimd, Aesenc128klEncryptsWithHandle) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Aesenc128kl_xmm_m384,
      iced_x86::Register::XMM5,
      iced_x86::MemoryOperand::with_base_index(iced_x86::Register::RAX, iced_x86::Register::None)).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aesenc128kl xmm5, [rax]"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory& memory) {
               enable_key_locker_with_test_iwkey(state);
               map_handle(memory, kKlHandle128);
               state.gpr[0] = kHandleAddress;
               set_aes_block(state, 5, kFipsPlaintext);
               state.rflags |= seven::kFlagZF | seven::kFlagCF;
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               expect_xmm_block(state, 5, kFipsAes128Ciphertext);
               EXPECT_EQ(state.rflags & (seven::kFlagZF | seven::kFlagCF), 0u);
             });
}

TEST(KuberaSimd, Aesdecwide256klDecryptsAllEightBlocks) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with1(
      iced_x86::Code::Aesdecwide256kl_m512,
      iced_x86::MemoryOperand::with_base_index(iced_x86::Register::RAX, iced_x86::Register::None)).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aesdecwide256kl [rax]"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory& memory) {
               enable_key_locker_with_test_iwkey(state);
               map_handle(memory, kKlHandle256);
               state.gpr[0] = kHandleAddress;
               for (std::size_t i = 0; i < 8; ++i) set_aes_block(state, i, kFipsAes256Ciphertext);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               for (std::size_t i = 0; i < 8; ++i) expect_xmm_block(state, i, kFipsPlaintext);
               EXPECT_EQ(state.rflags & seven::kFlagZF, 0u);
             });
}

TEST(KuberaSimd, Aesenc128klRejectsTamperedHandle) {
  std::vector<std::uint8_t> bytes;
  const auto instr = iced_x86::Instruction::with2(
      iced_x86::Code::Aesenc128kl_xmm_m384,
      iced_x86::Register::XMM5,
      iced_x86::MemoryOperand::with_base_index(iced_x86::Register::RAX, iced_x86::Register::None)).value();
  ASSERT_TRUE(encode_to_bytes(instr, bytes, "aesenc128kl xmm5, [rax]"));

  run_single(bytes,
             [](seven::CpuState& state, seven::Memory& memory) {
               enable_key_locker_with_test_iwkey(state);
               map_handle(memory, kKlHandle128, 40, 0x01);  // one ciphertext bit
               state.gpr[0] = kHandleAddress;
               set_aes_block(state, 5, kFipsPlaintext);
             },
             [](const seven::ExecutionResult&, const seven::CpuState& state, const seven::Memory&) {
               expect_xmm_block(state, 5, kFipsPlaintext);
               EXPECT_NE(state.rflags & seven::kFlagZF, 0u);
             });
}

}  // namespace
