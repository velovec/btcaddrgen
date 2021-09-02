#include <cstdint>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <tuple>
#include <vector>

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

  char const *hostname = "webcam-sss.ru";
  int port = 5672, status;
  char const *exchange = "btc";
  char const *routingkey = "btc";

  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    utils::die("creating TCP socket");
  }

  status = amqp_socket_open(socket, hostname, port);
  if (status) {
    utils::die("opening TCP socket");
  }

  utils::die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "Logging in");
  amqp_channel_open(conn, 1);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring exchange");

  amqp_bytes_t queuename;
  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    utils::die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      return 1;
    }
  }

  amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), amqp_empty_table);
  utils::die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");


  for (int i = 0; i < 6000; i++ ) {
    btc::Wallet wallet = btc::Wallet::Generate();

    std::string addrString = wallet.GetAddress().ToString();

    {
      amqp_basic_properties_t props;
      props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
      props.content_type = amqp_cstring_bytes("text/plain");
      props.delivery_mode = 2; /* persistent delivery mode */
      utils::die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey), 0, 0, &props, amqp_cstring_bytes(addrString.c_str())), "Publishing");
    }
  }

  utils::die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
  utils::die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  utils::die_on_error(amqp_destroy_connection(conn), "Ending connection");

  return 0;
}
