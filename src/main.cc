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

char const *messageExchange = "btc";

char const *addressRoutingKey = "btc";
char const *taskRoutingKey = "task";
char const *reportRoutingKey = "report";

void generate_random(amqp_connection_state_t conn, int depth, int from, int to, std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
  while (running) {
    std::vector<uint8_t> rnd;
    rnd.resize(depth - 1);

    rnd::RandManager rnd_man(depth - 1);
    rnd_man.Begin();
    rnd_man.Rand<rnd::Rand_OpenSSL<128>>();
    rnd_man.Rand<rnd::Rand_OS>();
    rnd = rnd_man.End();

    for (int i = 0; i < depth; i++) {
      vector.push_back(rnd.at(i));
    }

    for (uint8_t i = 0; i < 255; i++) {
      vector[vector.size() - 1] = i;

      callback(conn, vector);
    }

    vector[vector.size() - 1] = 255;
    callback(conn, vector);
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

void generate_reverse_range(amqp_connection_state_t conn, int depth, int from, int to, std::vector<uint8_t> vector, void (*callback)(amqp_connection_state_t conn, const std::vector<uint8_t>&)) {
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
      utils::die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes(messageExchange), amqp_cstring_bytes(resultRoutingKey), 0, 0, &props, amqp_cstring_bytes(message.c_str())), " [x] AMPQ error: unable to publish");
    }

    counter = 0;
    message = "";
  }
}

amqp_connection_state_t connect(const char *hostname, int port) {
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    die(" [!] AMPQ error: unable to create TCP socket");
  }

  int status = amqp_socket_open(socket, hostname, port);
  if (status) {
    die(" [!] AMQP error: unable to open TCP socket");
  }

  die_on_amqp_error(
      amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
      " [!] AMQP error: unable to log in"
  );
  amqp_channel_open(conn, 1);
  die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to open channel");

  std::cout << " [I] AMQP: connection to " << hostname << ":" << port << " established" << std::endl;

  return conn;
}

void declare_exchange(amqp_connection_state_t conn) {
  amqp_exchange_declare(conn, 1, amqp_cstring_bytes(messageExchange), amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to declare exchange");

  std::cout << " [I] AMQP: exchange declared" << std::endl;
}

amqp_bytes_t declare_queue(amqp_connection_state_t conn, const char* routingKey, bool exclusive, bool durable) {
  amqp_bytes_t queueName;
  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(
        conn, 1, exclusive ? amqp_empty_bytes : amqp_cstring_bytes(routingKey),
        0, durable ? 1 : 0, exclusive ? 1 : 0, exclusive ? 1 : 0, amqp_empty_table
    );

    die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to declare queue");
    queueName = amqp_bytes_malloc_dup(r->queue);
    if (queueName.bytes == NULL) {
      die(" [!] AMQP error: out of memory while copying queue name");
    }
  }

  amqp_queue_bind(conn, 1, queueName, amqp_cstring_bytes(messageExchange), amqp_cstring_bytes(routingKey), amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to bind queue");

  std::cout << " [I] AMQP: queue '" << (const char*) queueName.bytes << "' declared and bound to '" << routingKey << "'" << std::endl;

  return queueName;
}

static void start_consuming(amqp_connection_state_t conn, amqp_bytes_t taskQueue) {
  amqp_basic_consume(conn, 1, taskQueue, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  die_on_amqp_error(amqp_get_rpc_reply(conn), " [!] AMQP error: unable to start consuming");

  while (running) {
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(conn);

    res = amqp_consume_message(conn, &envelope, NULL, 0);
    die_on_amqp_error(res, " [!] AMQP error: unable to consume message");

    bool ack = true;
    if (envelope.message.body.len > 0) {
      char* messageBody = (char *) envelope.message.body.bytes;
      std::stringstream messageBodyStream(messageBody);
      std::string line;

      std::getline(messageBodyStream, line, ';');
      std::vector<uint8_t> blockData;
      if (!utils::ImportFromHexString(line, blockData)) {
        ack = false;
      } else {
        std::cout << " [!] Got new block: " << line << std::endl;

        std::getline(messageBodyStream, line, ';');
        uint8_t from = std::stoul(line);

        std::getline(messageBodyStream, line, ';');
        uint8_t to = std::stoul(line);

        std::getline(messageBodyStream, line, ';');
        int depth = std::stoi(line);

        std::getline(messageBodyStream, line, ';');

        const char *direct = "direct";
        const char *reverse = "reverse";
        const char *random = "random";

        if (line.c_str() == *direct) {
          std::cout << " [!] Generator: DIRECT from: " << from << " to: " << to << std::endl;
          generate_direct_range(conn, depth, from, to, blockData, on_generate);
        } else if (line.c_str() == *reverse) {
          std::cout << " [!] Generator: REVERSE from: " << from << " to: " << to << std::endl;
          generate_reverse_range(conn, depth, to, from, blockData, on_generate);
        } else if (line.c_str() == *random) {
          std::cout << " [!] Generator: RANDOM" << std::endl;
          generate_random(conn, depth, from, to, blockData, on_generate);
        } else {
          std::cout << " [!] Generator: FALLBACK (RANDOM)" << std::endl;
          generate_random(conn, depth, from, to, blockData, on_generate);
        }
      }
    } else {
      std::cout << " [!] AMQP warning: empty message" << std::endl;
    }

    if (ack) {
      die_on_error(amqp_basic_ack(conn, 1, envelope.delivery_tag, false), " [x] AMQP error: unable to ACK message");
    } else {
      die_on_error(amqp_basic_nack(conn, 1, envelope.delivery_tag, false, true), " [x] AMQP error: unable to NACK message");
    }

    amqp_destroy_envelope(&envelope);
  }
}

int main(int argc, const char *argv[]) {

  char const *hostname = std::getenv("AMQP_HOST");

  // Connect AMQP
  amqp_connection_state_t conn = connect(hostname, 5672);
  declare_exchange(conn);

  amqp_bytes_t addressQueue = declare_queue(conn, addressRoutingKey, false, false);
  amqp_bytes_t taskQueue = declare_queue(conn, taskRoutingKey, false, false);
  amqp_bytes_t reportQueue = declare_queue(conn, reportRoutingKey, false, true);

  start_consuming(conn, taskQueue);

  // Cleanup
  amqp_bytes_free(addressQueue);
  amqp_bytes_free(taskQueue);
  amqp_bytes_free(reportQueue);

  die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), " [!] AMQP error: unable to close channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), " [!] AMQP error: unable to close connection");
  die_on_error(amqp_destroy_connection(conn), " [!] AMQP error: unable to terminate connection");

  return 0;
}
