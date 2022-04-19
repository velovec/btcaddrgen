#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdarg>

#include <ecdsa/key.h>
#include <ecdsa/rnd_man.h>
#include <ecdsa/rnd_openssl.h>
#include <ecdsa/rnd_os.h>

#include <fstream>

#include "btcwallet.h"
#include "btcaddr.h"
#include "utils.h"

#include "bloom.h"

using namespace std;

struct bloom bloom1;
struct bloom bloom2;
struct bloom bloom3;

void generate_random(int depth, std::vector<uint8_t> vector, void (*callback)(const std::vector<uint8_t>&, bool)) {
  for (;;) {
    std::vector<uint8_t> data = vector;

    std::vector<uint8_t> rnd;
    rnd.resize(depth - 1);

    rnd::RandManager rnd_man(depth - 1);
    rnd_man.Begin();
    rnd_man.Rand<rnd::Rand_OpenSSL<128>>();
    rnd_man.Rand<rnd::Rand_OS>();
    rnd = rnd_man.End();

//    cout << "Iteration " << w << ": " << base58::EncodeBase58(rnd) << endl;

    for (int i = 0; i < depth - 1; i++) {
      data.push_back(rnd.at(i));
    }

    for (uint8_t i = 0; i < 255; i++) {
      data[data.size() - 1] = i;

      callback(data, false);
    }

    data[data.size() - 1] = 255;
    callback(data, true);

    data.clear();
  }
}

int bloom_check_all(struct bloom * bloom, std::string key) {
  return bloom_check(bloom, key.c_str(), key.length());
}

void on_generate(const std::vector<uint8_t>& pKeyData, bool last) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

  btc::Wallet wallet;
  wallet.SetPrivateKey(pKey);

  int res;

  string a1c = wallet.GetAddress(btc::A1C).ToString();
  res = bloom_check_all(&bloom1, a1c);
  if (res > 0) {
    std::cout << "<!--XSUPERVISOR:BEGIN-->" << wallet.GetPrivateKey() << ":" << a1c << "<!--XSUPERVISOR:END-->" << std::endl;
  }

  string a1u = wallet.GetAddress(btc::A1U).ToString();
  res = bloom_check_all(&bloom1, a1u);
  if (res > 0) {
    std::cout << "<!--XSUPERVISOR:BEGIN-->" << wallet.GetPrivateKey() << ":" << a1u << "<!--XSUPERVISOR:END-->" << std::endl;
  }

//  string a3 = wallet.GetAddress(btc::A3).ToString();
//  res = bloom_check_all(&bloom2, a3);
//  if (res > 0) {
//    std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a3 << std::endl;
//    send(conn, wallet.GetPrivateKey() + ":" + a3);
//  }

//  string b32pk = wallet.GetAddress(btc::B32PK).ToString();
//  res = bloom_check_all(&bloom3, b32pk);
//  if (res > 0) {
//    std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << b32pk << std::endl;
//    send(conn, wallet.GetPrivateKey() + ":" + b32pk);
//  }

//  string b32s = wallet.GetAddress(btc::B32S).ToString();
//  res = bloom_check_all(&bloom3, b32s);
//  if (res > 0) {
//   std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << b32s << std::endl;
//    send(conn, wallet.GetPrivateKey() + ":" + b32s);
//  }
}

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

struct bloom_metadata {
  string bloom1_name;
  long long bloom1_size;

  string bloom2_name;
  long long bloom2_size;

  string bloom3_name;
  long long bloom3_size;
};

bloom_metadata read_metadata(const char* filename) {
  struct bloom_metadata b;
  std::ifstream infile(filename);

  const string delimiter = ":";
  string line;

  infile >> line;
  b.bloom1_name = line.substr(0, line.find(delimiter));
  b.bloom1_size = stoll(line.substr(line.find(delimiter) + 1, line.length()));

  infile >> line;
  b.bloom2_name = line.substr(0, line.find(delimiter));
  b.bloom2_size = stoll(line.substr(line.find(delimiter) + 1, line.length()));

  infile >> line;
  b.bloom3_name = line.substr(0, line.find(delimiter));
  b.bloom3_size = stoll(line.substr(line.find(delimiter) + 1, line.length()));

  return b;
}

int main(int argc, const char *argv[]) {
  bloom_metadata meta = read_metadata("/bloom_data/bloom.metadata");

  cout << "Metadata loaded:" << endl;
  cout << "  " << meta.bloom1_name << " (" << meta.bloom1_size << ")" << endl;
  cout << "  " << meta.bloom2_name << " (" << meta.bloom2_size << ")" << endl;
  cout << "  " << meta.bloom3_name << " (" << meta.bloom3_size << ")" << endl;

  int res = 0;
  cout << "Initializing bloom filter 1 with size " << meta.bloom1_size << "..." << endl;
  res += bloom_init(&bloom1, meta.bloom1_size, 0.00000001);
  cout << "Initializing with code " << res << endl;
  cout << "Initializing bloom filter 2 with size " << meta.bloom2_size << "..." << endl;
  res += bloom_init(&bloom2, meta.bloom2_size, 0.00000001);
  cout << "Initializing with code " << res << endl;
  cout << "Initializing bloom filter 3 with size " << meta.bloom3_size << "..." << endl;
  res += bloom_init(&bloom3, meta.bloom3_size, 0.00000001);
  cout << "Initializing with code " << res << endl;

  if (res != 0) {
    die("Unable to initialize bloom filter");
  }

  cout << "Loading bloom filters..." << endl;
  cout << "Loading bloom filter 1 '" << meta.bloom1_name << "'" << endl;
  bloom_load(&bloom1, meta.bloom1_name.c_str());
  cout << "Loading bloom filter 2 '" << meta.bloom2_name << "'" << endl;
  bloom_load(&bloom2, meta.bloom2_name.c_str());
  cout << "Loading bloom filter 3 '" << meta.bloom2_name << "'" << endl;
  bloom_load(&bloom3, meta.bloom3_name.c_str());
  cout << "Bloom filter loaded" << endl;

  // Generation
  std::vector<uint8_t> blockData;
  if (!utils::ImportFromHexString("", blockData)) {
    die("Unable to read blockData");
  }

  generate_random(31, blockData, on_generate);

  bloom_free(&bloom1);
  bloom_free(&bloom2);
  bloom_free(&bloom3);

  return 0;
}
