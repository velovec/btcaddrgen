//
// Created by velovec on 28.08.2021.
//

#include "btcwallet.h"

#include <iostream>
#include <utility>

#include "utils.h"

namespace btc {

  Wallet Wallet::Generate() {
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>();

    Wallet wallet;
    wallet.SetPrivateKey(pKey);

    return wallet;
  }

  Wallet Wallet::FromHexPrivateKey(const std::string& hex) {
    std::vector<uint8_t> pKeyData;
    bool success = utils::ImportFromHexString(hex, pKeyData);

    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

    Wallet wallet;
    wallet.SetPrivateKey(pKey);

    return wallet;
  }

  Wallet Wallet::FromWifPrivateKey(const std::string& wif) {
    std::vector<uint8_t> pKeyData = btc::wif::WifToPrivateKey(wif);
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

    Wallet wallet;
    wallet.SetPrivateKey(pKey);

    return wallet;
  }

  Wallet Wallet::FromPrivateKeyData(const std::vector<uint8_t> pKeyData) {
    std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

    Wallet wallet;
    wallet.SetPrivateKey(pKey);

    return wallet;
  }

  Address Wallet::GetAddress(AddressType type) {
    if (type == A1U) {
      this->privateKey->CalculatePublicKey(false);
    } else {
      this->privateKey->CalculatePublicKey(true);
    }

    auto pubKey = this->privateKey->CreatePubKey();
    unsigned char hash160[20];

    switch (type) {
      case A1C:
      case A1U:
        return btc::Address::A1FromPublicKey(pubKey.get_pub_key_data(), hash160);
      case A3:
        return btc::Address::A3FromPublicKey(pubKey.get_pub_key_data(), hash160);
      case B32PK:
        return btc::Address::B32PKFromPublicKey(pubKey.get_pub_key_data());
      case B32S:
        return btc::Address::B32SFromPublicKey(pubKey.get_pub_key_data());
    }
  }

  std::string Wallet::GetPrivateKey() {
    return btc::wif::PrivateKeyToWif(this->privateKey->get_priv_key_data());
  }

  std::vector<uint8_t> Wallet::GetPrivateKeyData() {
    return this->privateKey->get_priv_key_data();
  }

  void Wallet::SetPrivateKey(std::shared_ptr<ecdsa::Key> privateKey) {
    this->privateKey = std::move(privateKey);
  }

  void Wallet::ShowPrivateKeyInfo() {
    this->privateKey->CalculatePublicKey(true);
    auto pubKey = this->privateKey->CreatePubKey();
    unsigned char hash160[20];

    btc::Address addr =  btc::Address::A1FromPublicKey(pubKey.get_pub_key_data(), hash160);
    std::cout << "A1C: " << addr.ToString() << std::endl;

    addr =  btc::Address::A3FromPublicKey(pubKey.get_pub_key_data(), hash160);
    std::cout << "A3: " << addr.ToString() << std::endl;

    addr =  btc::Address::B32PKFromPublicKey(pubKey.get_pub_key_data());
    std::cout << "B32PK: " << addr.ToString() << std::endl;

    addr =  btc::Address::B32SFromPublicKey(pubKey.get_pub_key_data());
    std::cout << "B32S: " << addr.ToString() << std::endl;

    std::cout << "H160C: " << utils::BinaryToHexString(hash160, 20, false)
              << std::endl;

    this->privateKey->CalculatePublicKey(false);
    pubKey = this->privateKey->CreatePubKey();

    addr =  btc::Address::A1FromPublicKey(pubKey.get_pub_key_data(), hash160);

    std::cout << "A1U: " << addr.ToString() << std::endl;
    std::cout << "H160U: " << utils::BinaryToHexString(hash160, 20, false)
              << std::endl;

  }
}
