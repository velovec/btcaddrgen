#include "utils.h"

#include <openssl/ripemd.h>
#include <openssl/sha.h>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

namespace utils {

std::vector<uint8_t> ripemd160(const uint8_t *data, int len) {
  // Prepare output data
  std::vector<uint8_t> md;
  md.resize(RIPEMD160_DIGEST_LENGTH);

  // Calculate RIPEMD160
  RIPEMD160_CTX ctx;
  RIPEMD160_Init(&ctx);
  RIPEMD160_Update(&ctx, data, len);
  RIPEMD160_Final(md.data(), &ctx);

  return md;
}

std::vector<uint8_t> sha256(const uint8_t *data, int len) {
  // Prepare output data
  std::vector<uint8_t> md;
  md.resize(SHA256_DIGEST_LENGTH);

  // Calculate SHA256
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, data, len);
  SHA256_Final(md.data(), &ctx);

  // Returns
  return md;
}

std::vector<uint8_t> double_sha256(const uint8_t *data, int len) {
  auto result = sha256(data, len);

  return sha256(result.data(), result.size());
}

std::vector<uint8_t> hash160(const uint8_t *data, int len) {
  auto result = sha256(data, len);

  return ripemd160(result.data(), result.size());
}

std::vector<uint8_t> checksum(const uint8_t *data, int len) {
  auto result = double_sha256(data, len);

  std::vector<uint8_t> checksum;
  checksum.resize(4);

  memcpy(checksum.data(), result.data(), 4);

  return checksum;
}

std::vector<uint8_t> concat(const uint8_t *src, int src_len,
                            const uint8_t *data, int data_len) {
  std::vector<uint8_t> long_result;
  long_result.resize(src_len + data_len);
  memcpy(long_result.data(), src, src_len);
  memcpy(long_result.data() + src_len, data, data_len);

  return long_result;
}

std::string BinaryToHexString(const unsigned char *bin_data, size_t size,
                              bool is_little_endian) {
  std::stringstream ss_hex;
  if (is_little_endian) {
    // Little-endian
    for (int i = size - 1; i >= 0; --i) {
      ss_hex << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(bin_data[i]);
    }
  } else {
    // Normal binary buffer.
    for (int i = 0; i < size; ++i) {
      ss_hex << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(bin_data[i]);
    }
  }
  return ss_hex.str();
}

bool ImportFromHexString(const std::string &hex_str,
                         std::vector<uint8_t> &out_data) {
  int len = hex_str.size();
  if (len % 2 != 0) {
    return false;
  }
  int size = len / 2;
  out_data.resize(size);
  for (int i = 0; i < size; ++i) {
    std::string hex1 = hex_str.substr(i * 2, 2);
    std::stringstream hex_ss;
    int val;
    hex_ss << std::hex << hex1;
    hex_ss >> val;
    out_data[i] = val;
  }
  return true;
}

// RabbitMQ

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}
}