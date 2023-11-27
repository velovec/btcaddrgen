#include <cstdint>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdarg>
#include <signal.h>

#include <ecdsa/key.h>
#include <ecdsa/pub_key.h>
#include <ecdsa/rnd_man.h>
#include <ecdsa/rnd_openssl.h>
#include <ecdsa/rnd_os.h>

#include <fstream>

#include "btcwallet.h"
#include "btcaddr.h"
#include "utils.h"

#define BLOCK_COUNT 100

using namespace std;

bool running = true;

int block_count = 0;
const auto start_time = std::chrono::system_clock::now();
long last_time = std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count();

std::vector<uint8_t> target_hash160;

void generate(void (*callback)(const std::vector<uint8_t>&, bool)) {
  std::vector<uint8_t> rnd;

  rnd::RandManager rnd_man(32);
  rnd_man.Begin();
  rnd_man.Rand<rnd::Rand_OpenSSL<128>>();
  rnd_man.Rand<rnd::Rand_OS>();
  rnd = rnd_man.End();

  for (uint8_t i = 0; i < 255; i++) {
    rnd[rnd.size() - 1] = i;

    callback(rnd, false);
  }

  rnd[rnd.size() - 1] = 255;
  callback(rnd, true);

  rnd.clear();
}

void on_generate(const std::vector<uint8_t>& pKeyData, bool last) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);
  pKey->CalculatePublicKey(true);
  std::shared_ptr<ecdsa::PubKey> pubKey = pKey->CreatePubKey();

  std::vector<uint8_t> pubKeyData = pubKey.get_pub_key_data()
  std::vector<uint8_t> hash160 = utils::hash160(pubKeyData.data(), pubKeyData.size());

  if (has160 == target_hash160) {
    std::cout << "match found";
    
  }
  
  if (last) {
    block_count++;

    if (block_count == BLOCK_COUNT) {
      const auto p1 = std::chrono::system_clock::now();
      long time = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

      std::cout << "<!--XSUPERVISOR:BEGIN-->BLOCK_END:";
      std::cout << block_count << ":";
      std::cout << time - last_time << ":";
      std::cout << time;
      std::cout << "<!--XSUPERVISOR:END-->" << std::endl;
      std::cout << " [B] Block[" << block_count << "] calculated in " << time - last_time << " second(s)" << std::endl;

      block_count = 0;
      last_time = time;
    }
  }
}

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

int main(int argc, const char *argv[]) {
  target_hash160 = { 32, 212, 90, 106, 118, 37, 53, 112, 12, 233, 224, 178, 22, 227, 25, 148, 51, 93, 184, 165 };
  
  while (running) {
    generate(on_generate);
  }

  return 0;
}
