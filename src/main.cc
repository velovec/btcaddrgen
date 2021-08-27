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

#include "btcaddr.h"
#include "btcwif.h"

#define BUFF_SIZE 1024

int main(int argc, const char *argv[]) {
  for (int i = 0; i < 100000; i++ ) {
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>();
    auto pubKey = pKey->CreatePubKey();
    unsigned char hash160[20];
    auto addr = btc::Address::FromPublicKey(pubKey.get_pub_key_data(), 0, hash160);

    auto addrString = addr.ToString();
    auto pKeyWif = btc::wif::PrivateKeyToWif(pKey->get_priv_key_data());

    // std::cout << "Address: " << addr.ToString() << std::endl;
    // std::cout << "Private key: " << base58::EncodeBase58(pKey->get_priv_key_data()) << std::endl;
  }

  return 0;
}
