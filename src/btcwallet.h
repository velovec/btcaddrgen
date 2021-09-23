//
// Created by velovec on 28.08.2021.
//

#include "ecdsa/key.h"
#include "ecdsa/base58.h"
#include <memory>

#include "btcaddr.h"
#include "btcwif.h"

#ifndef BTCADDRGEN_BTCWALLET_H
#define BTCADDRGEN_BTCWALLET_H

namespace btc {
  class Wallet {
   public:
    static Wallet Generate();
    static Wallet FromWifPrivateKey(const std::string& wif);
    static Wallet FromHexPrivateKey(const std::string& hex);
    static Wallet FromPrivateKeyData(const std::vector<uint8_t> pKeyData);

    Address GetAddress(AddressType type);
    std::vector<uint8_t> GetPrivateKeyData();
    std::string GetPrivateKey();
    void ShowPrivateKeyInfo();

   private:
    void SetPrivateKey(std::shared_ptr<ecdsa::Key> privateKey);

    Wallet() {};

    std::shared_ptr<ecdsa::Key> privateKey;
  };
}

#endif  // BTCADDRGEN_BTCWALLET_H
