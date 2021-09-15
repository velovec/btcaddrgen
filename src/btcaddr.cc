#include "btcaddr.h"

#include <cstdint>
#include <cstring>

#include "ecdsa/base58.h"

#include "utils.h"
#include "bech32.h"

namespace btc {
  /**
       * Convert from one power-of-2 number base to another.
       *
       * Examples using ConvertBits<8, 5, true>():
       * 000000 -> 0000000000
       * 202020 -> 0400100200
       * 757575 -> 0e151a170a
       * abcdef -> 150f061e1e
       * ffffff -> 1f1f1f1f1e
   */
  template<int frombits, int tobits, bool pad, typename O, typename I>
  bool ConvertBits(const O& outfn, I it, I end) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;

    while (it != end) {
      acc = ((acc << frombits) | *it) & max_acc;
      bits += frombits;
      while (bits >= tobits) {
        bits -= tobits;
        outfn((acc >> bits) & maxv);
      }
      ++it;
    }

    if (pad) {
      if (bits) outfn((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
      return false;
    }
    return true;
  }

  Address Address::A1FromPublicKey(const std::vector<uint8_t> &pub_key,
                                   unsigned char *out_hash160) {
    // 1. SHA256 + RIPEMD160
    auto m = utils::hash160(pub_key.data(), pub_key.size());

    // 3. Add 0x00 on front
    unsigned char prefix_char = 0x00;
    m = utils::concat(&prefix_char, 1, m.data(), m.size());

    // 4. SHA256 twice
    auto c = utils::checksum(m.data(), m.size());

    // 5. Take first 4 bytes only and add to temp
    std::vector<uint8_t> long_result = utils::concat(
      m.data(), m.size(), c.data(), 4
    );

    // Copying hash160 if required.
    if (out_hash160) {
        memcpy(out_hash160, long_result.data() + 1, 20);
    }

    // 6. Base58
    Address addr;
    addr.addr_str_ = base58::EncodeBase58(long_result);

    // Returns address object
    return addr;
  }

  Address Address::A3FromPublicKey(const std::vector<uint8_t> &pub_key,
                                   unsigned char *out_hash160) {
    // 1. SHA256 + RIPEMD160
    auto result = utils::hash160(pub_key.data(), pub_key.size());

    unsigned char prefix_redeem[] = { '\x00', '\x14'};

    result = utils::concat(prefix_redeem, 2, result.data(), result.size());

    auto redeem_script = utils::hash160(result.data(), result.size());

    // 3. Add 0x05 on front
    unsigned char prefix_char = 0x05;
    auto m = utils::concat(&prefix_char, 1, redeem_script.data(), redeem_script.size());

    // 4. SHA256 twice
    auto c = utils::checksum(m.data(), m.size());

    // 5. Take first 4 bytes only and add to temp
    std::vector<uint8_t> long_result = utils::concat(
        m.data(), m.size(), c.data(), 4
    );

    // Copying hash160 if required.
    if (out_hash160) {
      memcpy(out_hash160, long_result.data() + 1, 20);
    }

    // 6. Base58
    Address addr;
    addr.addr_str_ = base58::EncodeBase58(long_result);

    // Returns address object
    return addr;
  }

  Address Address::B32PKFromPublicKey(const std::vector<uint8_t> &pub_key) {
    // 1. SHA256 + RIPEMD160
    auto result = utils::hash160(pub_key.data(), pub_key.size());

    Address addr;
    unsigned char witness_version = '\x00';

    std::vector<unsigned char> decoded;
    decoded.reserve((result.size() * 5) / 8);
    bool success = ConvertBits<8, 5, false>(
        [&](unsigned char c) { decoded.push_back(c); }, result.begin(), result.end()
    );

    addr.addr_str_ = bech32::Encode("bc", utils::concat(
      &witness_version, 1, decoded.data(), decoded.size()
    ));

    // Returns address object
    return addr;

  }

  Address Address::B32SFromPublicKey(const std::vector<uint8_t> &pub_key) {
    // 1. SHA256
    auto result = utils::sha256(pub_key.data(), pub_key.size());

    Address addr;
    unsigned char witness_version = '\x00';

    std::vector<unsigned char> decoded;
    decoded.reserve((result.size() * 5) / 8);
    bool success = ConvertBits<8, 5, false>(
      [&](unsigned char c) { decoded.push_back(c); }, result.begin(), result.end()
    );

    addr.addr_str_ = bech32::Encode("bc", utils::concat(
      &witness_version, 1, decoded.data(), decoded.size()
    ));

    // Returns address object
    return addr;
  }
}  // namespace btc
