#ifndef XLOG_MURMUR3_H
#define XLOG_MURMUR3_H

#include <stdint.h>
#include <stddef.h>

extern uint32_t murmur3_seed;
uint32_t murmur3_32(const void *key, size_t len);

#endif // XLOG_MURMUR3_H
