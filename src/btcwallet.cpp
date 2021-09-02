//
// Created by velovec on 28.08.2021.
//

#include "btcwallet.h"

namespace btc {

  Wallet Wallet::Generate() {
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>();

    Wallet wallet;
    wallet.SetPrivateKey(pKey);

    return wallet;
  }

  Address Wallet::GetAddress() {
    auto pubKey = this.privateKey->CreatePubKey();
    unsigned char hash160[20];

    return btc::Address::FromPublicKey(pubKey.get_pub_key_data(), 0, hash160);
  }

  std::string Wallet::GetPrivateKey() {
    return base58::EncodeBase58(this->privateKey->get_priv_key_data());
  }

  void Wallet::SetPrivateKey(std::shared_ptr<ecdsa::Key> privateKey) {
    this->privateKey = privateKey;
  }
}
