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

#define BUFF_SIZE 1024

int main(int argc, const char *argv[]) {

  char const *hostname = std::getenv("AMQP_HOST");
  int port = 5672, status;
  char const *exchange = "btc";
  char const *routingkey = "btc";

  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  conn = amqp_new_connection();

  std::cout << " [I] AMQP: " << hostname << ":" << port << "/" << exchange << "@" << routingkey << std::endl;

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    utils::die(" [x] AMQP error: unable to create TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    utils::die(" [x] AMQP error: unable to open TCP socket");
  }

  utils::die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), " [x] AMQP error: unable to log in");
  amqp_channel_open(conn, 1);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [x] AMQP error: unable to open channel");

  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), " [x] AMQP error: unable to declare exchange");

  std::cout << " [!] AMQP connection established" << std::endl;
  for (;;) {
    std::string message = "";
    btc::Wallet baseWallet = btc::Wallet::Generate();
    std::vector<uint8_t> baseKey = baseWallet.GetPrivateKeyData();

    for (uint8_t i = 0; i <= 255; i++) {
      baseKey[baseKey.size() - 1] = i;

      btc::Wallet wallet = btc::Wallet::FromPrivateKeyData(baseKey);

      if (i > 0) {
        message += ";";
      }

      message += wallet.GetPrivateKey() + ":";
      message += wallet.GetAddress(btc::A1C).ToString() + ":";
      message += wallet.GetAddress(btc::A1U).ToString() + ":";
      message += wallet.GetAddress(btc::A3).ToString() + ":";
      message += wallet.GetAddress(btc::B32PK).ToString() + ":";
      message += wallet.GetAddress(btc::B32S).ToString();
    }

    {
      amqp_basic_properties_t props;
      props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
      props.content_type = amqp_cstring_bytes("text/plain");
      props.delivery_mode = 1; /* persistent delivery mode */
      utils::die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), 0, 0, &props, amqp_cstring_bytes(message.c_str())), " [x] AMPQ error: unable to publish");
    }
  }
  std::cout << " [+] AMQP payload sent" << std::endl;

  utils::die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), " [x] AMQP error: unable to close channel");
  utils::die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), " [x] AMQP error: unable to close connection");
  utils::die_on_error(amqp_destroy_connection(conn), " [x] AMQP error: unable to destroy connection");

  return 0;
}
