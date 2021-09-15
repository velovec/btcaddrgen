#ifndef __BTC_ADDR_H__
#define __BTC_ADDR_H__

#include <cstdlib>

#include <string>
#include <vector>

namespace btc {

enum AddressType {
  A1C, A1U, A3, B32PK, B32S
};

class Address {
 public:
  /**
   * Convert a public key to address.
   *
   * @param pub_key Public key.
   * @param prefix_char Prefix character for address.
   * @param out_hash160 Hash160 data.
   *
   * @return New generated address object.
   */
  static Address A1FromPublicKey(const std::vector<uint8_t> &pub_key,
                                 unsigned char *out_hash160 = nullptr);
  static Address A3FromPublicKey(const std::vector<uint8_t> &pub_key,
                                 unsigned char *out_hash160 = nullptr);
  static Address B32PKFromPublicKey(const std::vector<uint8_t> &pub_key);
  static Address B32SFromPublicKey(const std::vector<uint8_t> &pub_key);

  /// Convert address object to string
  std::string ToString() const { return addr_str_; }

 private:
  Address() {}

 private:
  std::string addr_str_;
};

}  // namespace btc

#endif
