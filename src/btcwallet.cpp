//
// Created by velovec on 28.08.2021.
//

#include "btcwallet.h"

namespace btc {

  Wallet Wallet::Generate() {
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>();

    return Wallet::FromPrivateKey(pKey)
  }

  Wallet Wallet::FromPrivateKey(int pKey) {
    Wallet wallet;

    wallet.privateKey = pKey;

    auto pubKey = pKey->CreatePubKey();
    unsigned char hash160[20];

    wallet.addres->btc::Address::FromPublicKey(pubKey.get_pub_key_data(), 0, hash160);

    return wallet;
  }

  Address Wallet::GetAddress() {
    return this->address;
  }
}
