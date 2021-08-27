//
// Created by velovec on 28.08.2021.
//

#include <ecdsa/key.h>
#include "btcaddr.h"

#ifndef BTCADDRGEN_BTCWALLET_H
#define BTCADDRGEN_BTCWALLET_H

namespace btc {
  class Wallet {
   public:
    static Wallet FromPrivateKey(std::shared_ptr<ecdsa::Key> pKey);
    static Wallet Generate();

    Address GetAddress();

   private:
    Wallet();

    std::shared_ptr<ecdsa::Key> privateKey;
    Address address;
  };
}

#endif  // BTCADDRGEN_BTCWALLET_H
