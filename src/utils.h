#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>
#include <amqp.h>
#include <stdint.h>

namespace utils {

std::vector<uint8_t> ripemd160(const uint8_t *data, int len);

std::vector<uint8_t> sha256(const uint8_t *data, int len);

void die(const char *fmt, ...);
extern void die_on_error(int x, char const *context);
extern void die_on_amqp_error(amqp_rpc_reply_t x, char const *context);

extern void amqp_dump(void const *buffer, size_t len);

extern uint64_t now_microseconds(void);
extern void microsleep(int usec);

}  // namespace utils

#endif
