#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>

#include <string>
#include <vector>

namespace utils {

std::vector<uint8_t> ripemd160(const uint8_t *data, int len);

std::vector<uint8_t> sha256(const uint8_t *data, int len);

std::vector<uint8_t> double_sha256(const uint8_t *data, int len);

std::vector<uint8_t> hash160(const uint8_t *data, int len);

std::vector<uint8_t> checksum(const uint8_t *data, int len);

std::vector<uint8_t> concat(const uint8_t *src, int src_len, const uint8_t *data, int data_len);

std::string BinaryToHexString(const unsigned char *bin_data, size_t size, bool is_little_endian);

bool ImportFromHexString(const std::string &hex_str, std::vector<uint8_t> &out_data);

void die(const char *fmt, ...);

}  // namespace utils

#endif
