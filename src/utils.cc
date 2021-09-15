#include "utils.h"

#include <amqp.h>
#include <amqp_framing.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>
#include <cstdarg>
#include <cstdint>

#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

namespace utils {

std::vector<uint8_t> ripemd160(const uint8_t *data, int len) {
  // Prepare output data
  std::vector<uint8_t> md;
  md.resize(RIPEMD160_DIGEST_LENGTH);

  // Calculate RIPEMD160
  RIPEMD160_CTX ctx;
  RIPEMD160_Init(&ctx);
  RIPEMD160_Update(&ctx, data, len);
  RIPEMD160_Final(md.data(), &ctx);

  return md;
}

std::vector<uint8_t> sha256(const uint8_t *data, int len) {
  // Prepare output data
  std::vector<uint8_t> md;
  md.resize(SHA256_DIGEST_LENGTH);

  // Calculate SHA256
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, data, len);
  SHA256_Final(md.data(), &ctx);

  // Returns
  return md;
}

std::vector<uint8_t> double_sha256(const uint8_t *data, int len) {
  auto result = sha256(data, len);

  return sha256(result.data(), result.size());
}

std::vector<uint8_t> hash160(const uint8_t *data, int len) {
  auto result = sha256(data, len);

  return ripemd160(result.data(), result.size());
}

std::vector<uint8_t> checksum(const uint8_t *data, int len) {
  auto result = double_sha256(data, len);

  std::vector<uint8_t> checksum;
  checksum.resize(4);

  memcpy(checksum.data(), result.data(), 4);

  return checksum;
}

std::vector<uint8_t> concat(const uint8_t *src, int src_len, const uint8_t *data, int data_len) {
  std::vector<uint8_t> long_result;
  long_result.resize(src_len + data_len);
  memcpy(long_result.data(), src, src_len);
  memcpy(long_result.data() + src_len, data, data_len);

  return long_result;
}

std::string BinaryToHexString(const unsigned char *bin_data, size_t size, bool is_little_endian) {
  std::stringstream ss_hex;
  if (is_little_endian) {
    // Little-endian
    for (int i = size - 1; i >= 0; --i) {
      ss_hex << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(bin_data[i]);
    }
  } else {
    // Normal binary buffer.
    for (int i = 0; i < size; ++i) {
      ss_hex << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(bin_data[i]);
    }
  }
  return ss_hex.str();
}

bool ImportFromHexString(const std::string &hex_str, std::vector<uint8_t> &out_data) {
  int len = hex_str.size();
  if (len % 2 != 0) {
    return false;
  }
  int size = len / 2;
  out_data.resize(size);
  for (int i = 0; i < size; ++i) {
    std::string hex1 = hex_str.substr(i * 2, 2);
    std::stringstream hex_ss;
    int val;
    hex_ss << std::hex << hex1;
    hex_ss >> val;
    out_data[size - i - 1] = val;
  }
  return true;
}

// RabbitMQ

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

void die_on_error(int x, char const *context) {
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x));
    exit(1);
  }
}

void die_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
  switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "%s: missing RPC reply type!\n", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
          amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
          fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
                  context, m->reply_code, (int)m->reply_text.len,
                  (char *)m->reply_text.bytes);
          break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
          amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
          fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
                  context, m->reply_code, (int)m->reply_text.len,
                  (char *)m->reply_text.bytes);
          break;
        }
        default:
          fprintf(stderr, "%s: unknown server error, method id 0x%08X\n",
                  context, x.reply.id);
          break;
      }
      break;
  }

  exit(1);
}

static void dump_row(long count, int numinrow, int *chs) {
  int i;

  printf("%08lX:", count - numinrow);

  if (numinrow > 0) {
    for (i = 0; i < numinrow; i++) {
      if (i == 8) {
        printf(" :");
      }
      printf(" %02X", chs[i]);
    }
    for (i = numinrow; i < 16; i++) {
      if (i == 8) {
        printf(" :");
      }
      printf("   ");
    }
    printf("  ");
    for (i = 0; i < numinrow; i++) {
      if (isprint(chs[i])) {
        printf("%c", chs[i]);
      } else {
        printf(".");
      }
    }
  }
  printf("\n");
}

static int rows_eq(int *a, int *b) {
  int i;

  for (i = 0; i < 16; i++)
    if (a[i] != b[i]) {
      return 0;
    }

  return 1;
}

void amqp_dump(void const *buffer, size_t len) {
  unsigned char *buf = (unsigned char *)buffer;
  long count = 0;
  int numinrow = 0;
  int chs[16];
  int oldchs[16] = {0};
  int showed_dots = 0;
  size_t i;

  for (i = 0; i < len; i++) {
    int ch = buf[i];

    if (numinrow == 16) {
      int j;

      if (rows_eq(oldchs, chs)) {
        if (!showed_dots) {
          showed_dots = 1;
          printf(
              "          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
        }
      } else {
        showed_dots = 0;
        dump_row(count, numinrow, chs);
      }

      for (j = 0; j < 16; j++) {
        oldchs[j] = chs[j];
      }

      numinrow = 0;
    }

    count++;
    chs[numinrow++] = ch;
  }

  dump_row(count, numinrow, chs);

  if (numinrow != 0) {
    printf("%08lX:\n", count);
  }
}

}  // namespace utils
