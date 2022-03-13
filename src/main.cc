#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdarg>

#include <ecdsa/key.h>
#include <ecdsa/rnd_man.h>
#include <ecdsa/rnd_openssl.h>
#include <ecdsa/rnd_os.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <fstream>

#include "btcwallet.h"
#include "btcaddr.h"
#include "utils.h"

#include "bloom.h"

using namespace std;

struct bloom bloom1;
struct bloom bloom2;
struct bloom bloom3;

char const *messageExchange = "btc";

char const *addressRoutingKey = "result";

void generate_random(int depth, std::vector<uint8_t> vector, amqp_connection_state_t conn, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&, bool)) {
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

      callback(conn, data, false);
    }

    data[data.size() - 1] = 255;
    callback(conn, data, true);

    data.clear();
  }
}

int bloom_check_all(struct bloom * bloom, std::string key) {
  return bloom_check(bloom, key.c_str(), key.length());
}

void send(amqp_connection_state_t conn, std::string message) {
  amqp_basic_properties_t props;
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("text/plain");
  props.delivery_mode = 2; /* persistent delivery mode */
  utils::die_on_error(amqp_basic_publish(
    conn, 1, amqp_cstring_bytes(messageExchange),
    amqp_cstring_bytes(addressRoutingKey), 0,
    0, &props, amqp_cstring_bytes(message.c_str())),
    " [x] AMPQ error: unable to publish"
  );
}

void on_generate(amqp_connection_state_t conn, const std::vector<uint8_t>& pKeyData, bool last) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

  btc::Wallet wallet;
  wallet.SetPrivateKey(pKey);

  int res;

  string a1c = wallet.GetAddress(btc::A1C).ToString();
  res = bloom_check_all(&bloom1, a1c);
  if (res > 0) {
    std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a1c << std::endl;
    send(conn, wallet.GetPrivateKey() + ":" + a1c);
  }

  string a1u = wallet.GetAddress(btc::A1U).ToString();
  res = bloom_check_all(&bloom1, a1u);
  if (res > 0) {
    std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a1u << std::endl;
    send(conn, wallet.GetPrivateKey() + ":" + a1u);
  }

  string a3 = wallet.GetAddress(btc::A3).ToString();
  res = bloom_check_all(&bloom2, a3);
  if (res > 0) {
    std::cout << " [M] Matched[" << res << "] " << wallet.GetPrivateKey() << " " << a3 << std::endl;
    send(conn, wallet.GetPrivateKey() + ":" + a3);
  }

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

amqp_connection_state_t connect(const char *hostname, int port) {
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  conn = amqp_new_connection();

  std::cout << " [I] AMQP: Establishing connection to " << hostname << ":" << port << "..." << std::endl;

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    utils::die(" [!] AMPQ error: unable to create TCP socket");
  }

  int status = amqp_socket_open(socket, hostname, port);
  if (status) {
    utils::die(" [!] AMQP error: unable to open TCP socket");
  }

  utils::die_on_amqp_error(
    amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "publisher", "AuthT0ken"),
    " [!] AMQP error: unable to log in"
  );
  amqp_channel_open(conn, 1);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to open channel");

  std::cout << " [I] AMQP: connection to " << hostname << ":" << port << " established" << std::endl;

  return conn;
}

void declare_exchange(amqp_connection_state_t conn) {
  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(messageExchange), amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to declare exchange");

  std::cout << " [I] AMQP: exchange declared" << std::endl;
}

amqp_bytes_t declare_queue(amqp_connection_state_t conn, const char* routingKey, bool exclusive, bool durable) {
  amqp_bytes_t queueName;
  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(
      conn, 1, exclusive ? amqp_empty_bytes : amqp_cstring_bytes(routingKey),
      0, durable ? 1 : 0, exclusive ? 1 : 0, exclusive ? 1 : 0, amqp_empty_table
    );

    utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to declare queue");
    queueName = amqp_bytes_malloc_dup(r->queue);
    if (queueName.bytes == NULL) {
      utils::die(" [!] AMQP error: out of memory while copying queue name");
    }
  }

  amqp_queue_bind(conn, 1, queueName, amqp_cstring_bytes(messageExchange), amqp_cstring_bytes(routingKey), amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to bind queue");

  std::cout << " [I] AMQP: queue '" << (const char*) queueName.bytes << "' declared and bound to '" << routingKey << "'" << std::endl;

  return queueName;
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
  char const *hostname = std::getenv("AMQP_HOST");
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

  // Connect AMQP
  amqp_connection_state_t conn = connect(hostname, 5672);
  declare_exchange(conn);

  amqp_bytes_t addressQueue = declare_queue(conn, addressRoutingKey, false, true);

  // Generation
  std::vector<uint8_t> blockData;
  if (!utils::ImportFromHexString("", blockData)) {
    die("Unable to read blockData");
  }

  generate_random(31, blockData, conn, on_generate);

  // Cleanup
  amqp_bytes_free(addressQueue);

  bloom_free(&bloom1);
  bloom_free(&bloom2);
  bloom_free(&bloom3);

  utils::die_on_amqp_error(
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
    " [!] AMQP error: unable to close channel"
  );

  utils::die_on_amqp_error(
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
    " [!] AMQP error: unable to close connection"
  );

  utils::die_on_error(
    amqp_destroy_connection(conn),
    " [!] AMQP error: unable to terminate connection"
  );

  return 0;
}
