#ifndef XLOG_CRC32C_H
#define XLOG_CRC32C_H

#include "xlog.h"
#include <stdint.h>
#include <stddef.h>

XLOG_API uint32_t crc32c(uint32_t crc, const void *buf, size_t size);

#endif //XLOG_CRC32C_H
