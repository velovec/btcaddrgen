#include <cstdint>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <tuple>
#include <vector>
#include <cstdint>

#include <openssl/sha.h>

#include <ecdsa/base58.h>
#include <ecdsa/key.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "btcwallet.h"
#include "btcaddr.h"
#include "btcwif.h"
#include "utils.h"

#define KEYS_PER_MESSAGE 200

int counter = 0;
std::string message = "";
bool running = true;

char const *exchange = "btc";
char const *routingkey = "btc";

void generate_random(amqp_connection_state_t conn, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  while (running) {
    btc::Wallet baseWallet = btc::Wallet::Generate();
    std::vector<uint8_t> baseKey = baseWallet.GetPrivateKeyData();

    for (uint8_t i = 0; i < 255; i++) {
      baseKey[baseKey.size() - 1] = i;

      callback(conn, baseKey);
    }

    baseKey[baseKey.size() - 1] = 255;
    callback(conn, baseKey);
  }
}

void generate_direct(amqp_connection_state_t conn, int depth, std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  if (depth == 0) {
    callback(conn, vector);
    return;
  }
  vector.resize(33 - depth);

  for (int i = 0; i < 255; i++) {
    vector[vector.size() - 1] = i;
    generate_direct(conn, depth - 1, vector, callback);
  }

  vector[vector.size() - 1] = 255;
  generate_direct(conn, depth - 1, vector, callback);
}

void generate_direct_range(amqp_connection_state_t conn, int depth, int from, int to, std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  vector.resize(33 - depth);

  for (int i = from; i < to; i++) {
    vector[vector.size() - 1] = i;
    generate_direct(conn, depth - 1, vector, callback);
  }

  vector[vector.size() - 1] = to;
  generate_direct(conn, depth - 1, vector, callback);
}

void generate_reverse(amqp_connection_state_t conn, int depth, std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  if (depth == 0) {
    callback(conn, vector);
    return;
  }
  vector.resize(33 - depth);

  for (int i = 255; i > 0; i--) {
    vector[vector.size() - 1] = i;
    generate_reverse(conn, depth - 1, vector, callback);
  }

  vector[vector.size() - 1] = 0;
  generate_reverse(conn, depth - 1, vector, callback);
}

void generate_reverse_range(amqp_connection_state_t conn, int depth, int from, int to std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  vector.resize(33 - depth);

  for (int i = from; i > to; i--) {
    vector[vector.size() - 1] = i;
    generate_reverse(conn, depth - 1, vector, callback);
  }

  vector[vector.size() - 1] = to;
  generate_reverse(conn, depth - 1, vector, callback);
}

void on_generate(amqp_connection_state_t conn, const std::vector<uint8_t>& pKeyData) {
  std::shared_ptr<ecdsa::Key> pKey = std::make_shared<ecdsa::Key>(pKeyData);

  btc::Wallet wallet;
  wallet.SetPrivateKey(pKey);

  if (counter > 0) {
    message += ";";
  }

  message += wallet.GetPrivateKey() + ":";
  message += wallet.GetAddress(btc::A1C).ToString() + ":";
  message += wallet.GetAddress(btc::A1U).ToString() + ":";
  message += wallet.GetAddress(btc::A3).ToString() + ":";
  message += wallet.GetAddress(btc::B32PK).ToString() + ":";
  message += wallet.GetAddress(btc::B32S).ToString();

  counter++;

  if (counter == KEYS_PER_MESSAGE - 1) {
    {
      amqp_basic_properties_t props;
      props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
      props.content_type = amqp_cstring_bytes("text/plain");
      props.delivery_mode = 2; /* persistent delivery mode */
      utils::die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), 0, 0, &props, amqp_cstring_bytes(message.c_str())), " [x] AMPQ error: unable to publish");
    }

    counter = 0;
    message = "";
  }
}

int main(int argc, const char *argv[]) {

  char const *hostname = std::getenv("AMQP_HOST");
  char const *generation_type = std::getenv("GENERATOR");

  char const *from_str = std::getenv("FROM");
  char const *to_str = std::getenv("TO");

  int from = from_str ? std::atoi(from_str) : 0;
  int to = to_str ? std::atoi(to_str) : 255;

  int port = 5672, status;

  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  conn = amqp_new_connection();

  std::cout << " [I] AMQP: " << hostname << ":" << port << "/" << exchange << "@" << routingkey << std::endl;

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    utils::die(" [x] AMQP error: unable to create TCP socket");
  }

  std::cout << " [!] AMQP TCP socket created" << std::endl;

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    utils::die(" [x] AMQP error: unable to open TCP socket");
  }

  std::cout << " [!] AMQP TCP socket opened" << std::endl;

  utils::die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), " [x] AMQP error: unable to log in");
  amqp_channel_open(conn, 1);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [x] AMQP error: unable to open channel");

  std::cout << " [!] AMQP channel opened" << std::endl;

  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [x] AMQP error: unable to declare exchange");

  std::cout << " [!] AMQP connection established" << std::endl;
  std::cout << " [!] Generator: " << generation_type << " from: " << from << " to: " << to << std::endl;

  switch (generation_type) {
    case "direct":
      generate_direct_range(conn, 32, from, to, std::vector<uint8_t>(), on_generate);
      break;
    case "reverse":
      generate_reverse_range(conn, 32, to, from, std::vector<uint8_t>(), on_generate);
      break;
    case "random":
    default:
      generate_random(conn, on_generate);
      break;
  }


  std::cout << " [+] AMQP payload sent" << std::endl;

  utils::die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), " [x] AMQP error: unable to close channel");
  utils::die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), " [x] AMQP error: unable to close connection");
  utils::die_on_error(amqp_destroy_connection(conn), " [x] AMQP error: unable to destroy connection");

  return 0;
}
