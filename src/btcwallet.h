//
// Created by velovec on 28.08.2021.
//

#include <ecdsa/key.h>
#include <ecdsa/base58.h>
#include <memory>

#include "btcaddr.h"

#ifndef BTCADDRGEN_BTCWALLET_H
#define BTCADDRGEN_BTCWALLET_H

namespace btc {
  class Wallet {
   public:
    static Wallet Generate();

    Address GetAddress();
    std::string GetPrivateKey();

   private:
    void SetPrivateKey(std::shared_ptr<ecdsa::Key> privateKey);

    Wallet() {};

    std::shared_ptr<ecdsa::Key> privateKey;
  };
}

#endif  // BTCADDRGEN_BTCWALLET_H
