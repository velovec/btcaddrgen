#include <cstdint>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdarg>
#include <signal.h>

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

bool running = true;

int block_count = 0;
long last_time = 0;

void generate_random(short flag, void (*callback)(const std::vector<uint8_t>&, short, bool)) {
  std::vector<uint8_t> rnd;

  rnd::RandManager rnd_man(32);
  rnd_man.Begin();
  rnd_man.Rand<rnd::Rand_OpenSSL<128>>();
  rnd_man.Rand<rnd::Rand_OS>();
  rnd = rnd_man.End();

  for (uint8_t i = 0; i < 255; i++) {
    rnd[rnd.size() - 1] = i;

    callback(rnd, flag, false);
  }

  rnd[rnd.size() - 1] = 255;
  callback(rnd, flag, true);

  rnd.clear();
}

int bloom_check_all(struct bloom * bloom, std::string key) {
  return bloom_check(bloom, key.c_str(), key.length());
}

void on_generate(const std::vector<uint8_t>& pKeyData, short flag, bool last) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

  btc::Wallet wallet;
  wallet.SetPrivateKey(pKey);

  int res;

  if (flag & 0x01 == 0x01) {
    string a1c = wallet.GetAddress(btc::A1C).ToString();
    res = bloom_check_all(&bloom1, a1c);
    if (res > 0) {
      std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a1c << std::endl;
      std::cout << "<!--XSUPERVISOR:BEGIN-->MATCH:" << wallet.GetPrivateKey() << ":" << a1c << "<!--XSUPERVISOR:END-->" << std::endl;
    }

    string a1u = wallet.GetAddress(btc::A1U).ToString();
    res = bloom_check_all(&bloom1, a1u);
    if (res > 0) {
      std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a1u << std::endl;
      std::cout << "<!--XSUPERVISOR:BEGIN-->MATCH:" << wallet.GetPrivateKey() << ":" << a1u << "<!--XSUPERVISOR:END-->" << std::endl;
    }
  }

  if (flag & 0x02 == 0x02) {
    string a3 = wallet.GetAddress(btc::A3).ToString();
    res = bloom_check_all(&bloom2, a3);
    if (res > 0) {
      std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a3 << std::endl;
      std::cout << "<!--XSUPERVISOR:BEGIN-->MATCH:" << wallet.GetPrivateKey() << ":" << a3 << "<!--XSUPERVISOR:END-->" << std::endl;
    }
  }

  if (flag & 0x04 == 0x04) {
    string b32pk = wallet.GetAddress(btc::B32PK).ToString();
    res = bloom_check_all(&bloom3, b32pk);
    if (res > 0) {
      std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << b32pk << std::endl;
      std::cout << "<!--XSUPERVISOR:BEGIN-->MATCH:" << wallet.GetPrivateKey() << ":" << b32pk << "<!--XSUPERVISOR:END-->" << std::endl;
    }

    string b32s = wallet.GetAddress(btc::B32S).ToString();
    res = bloom_check_all(&bloom3, b32s);
    if (res > 0) {
      std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << b32s << std::endl;
      std::cout << "<!--XSUPERVISOR:BEGIN-->MATCH:" << wallet.GetPrivateKey() << ":" << b32s << "<!--XSUPERVISOR:END-->" << std::endl;
    }
  }

  if (last) {
    block_count++;

    if (block_count == 1000) {
      const auto p1 = std::chrono::system_clock::now();
      long time = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

      std::cout << "<!--XSUPERVISOR:BEGIN-->BLOCK_END:";
      std::cout << block_count << ":";
      std::cout << time - last_time < ":";
      std::cout << time;
      std::cout << "<!--XSUPERVISOR:END-->" << std::endl;

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

void read_bloom_filter() {
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

  std::cout << "<!--XSUPERVISOR:BEGIN-->BLOOM_LOAD<!--XSUPERVISOR:END-->" << std::endl;
}

void cleanup() {
  bloom_free(&bloom1);
  bloom_free(&bloom2);
  bloom_free(&bloom3);
}

void reload_bloom_filter() {
  cleanup();

  read_bloom_filter();
}

void sig_handler(int sig_num) {
  read_bloom_filter();
}

int main(int argc, const char *argv[]) {
  read_bloom_filter();
  signal(SIGUSR1, sig_handler);

  while (running) {
    generate_random(0x01, on_generate);
  }

  cleanup();
  return 0;
}
