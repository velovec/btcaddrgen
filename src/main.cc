#include <cstdint>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <tuple>
#include <vector>

#include <openssl/sha.h>

#include <ecdsa/base58.h>
#include <ecdsa/key.h>

#include "args.h"
#include "btcaddr.h"
#include "btcwif.h"

#define BUFF_SIZE 1024

/**
 * Show help document.
 *
 * @param args The argument manager
 */
void ShowHelp(const Args &args) {
  std::cout << "BTCAddr(ess)Gen(erator)" << std::endl
            << "  An easy to use Bitcoin Address offline generator."
            << std::endl
            << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  ./btcaddrgen [arguments...]" << std::endl << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << args.GetArgsHelpString() << std::endl;
}

std::tuple<std::vector<uint8_t>, bool> HashFile(const std::string &path) {
  std::vector<uint8_t> md;

  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    std::cerr << "Cannot open file " << path << std::endl;
    return std::make_tuple(md, false);
  }

  // Hash file contents
  SHA512_CTX ctx;
  SHA512_Init(&ctx);

  // Reading...
  char buff[BUFF_SIZE];
  while (!input.eof()) {
    input.read(buff, BUFF_SIZE);
    size_t buff_size = input.gcount();
    SHA512_Update(&ctx, buff, buff_size);
  }

  // Get md buffer.
  md.resize(SHA512_DIGEST_LENGTH);
  SHA512_Final(md.data(), &ctx);

  return std::make_tuple(md, true);
}

std::tuple<std::vector<uint8_t>, bool> Signing(std::shared_ptr<ecdsa::Key> pkey,
                                               const std::string &path) {
  std::vector<uint8_t> signature;

  std::vector<uint8_t> md;
  bool succ;
  std::tie(md, succ) = HashFile(path);
  if (!succ) {
    return std::make_tuple(signature, false);
  }

  std::tie(signature, succ) = pkey->Sign(md);
  if (!succ) {
    std::cerr << "Cannot signing file!" << std::endl;
    return std::make_tuple(signature, false);
  }

  return std::make_tuple(signature, true);
}

bool Verifying(const ecdsa::PubKey &pub_key, const std::string &path,
               const std::vector<uint8_t> &signature) {
  std::vector<uint8_t> md;
  bool succ;
  std::tie(md, succ) = HashFile(path);
  if (succ) {
    return pub_key.Verify(md, signature);
  }
  return false;
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

void ShowKeyInfo(std::shared_ptr<ecdsa::Key> pkey, unsigned char prefix_char) {
  auto pub_key = pkey->CreatePubKey();
  unsigned char hash160[20];
  auto addr = btc::Address::FromPublicKey(pub_key.get_pub_key_data(),
                                          prefix_char, hash160);
  std::cout << "Address: " << addr.ToString() << std::endl;
  std::cout << "Hash160(Little-endian): "
            << BinaryToHexString(hash160, 20, true) << std::endl;
  std::cout << "Hash160: " << BinaryToHexString(hash160, 20, false)
            << std::endl;
  std::cout << "Public key: " << base58::EncodeBase58(pkey->get_pub_key_data())
            << std::endl;
  std::cout << "Private key: "
            << base58::EncodeBase58(pkey->get_priv_key_data()) << std::endl;
  std::cout << "Private key(WIF): "
            << btc::wif::PrivateKeyToWif(pkey->get_priv_key_data())
            << std::endl;
  std::cout << "Private key(HEX): "
            << BinaryToHexString(pkey->get_priv_key_data().data(),
                                 pkey->get_priv_key_data().size(), true)
            << std::endl;
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
    out_data[size - i - 1] = val;
  }
  return true;
}

/// Main program.
int main(int argc, const char *argv[]) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>();
  auto pubKey = pKey->CreatePubKey();
  unsigned char hash160[20];
  auto addr = btc::Address::FromPublicKey(pubKey.get_pub_key_data(), 0, hash160);

  std::cout << "Address: " << addr.ToString() << std::endl;
  std::cout << "Private key: " << base58::EncodeBase58(pKey->get_priv_key_data()) << std::endl;

  return 0;
}
