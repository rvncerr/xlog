#ifndef XLOG_ENDIAN_H
#define XLOG_ENDIAN_H

#include <stdint.h>

void put_le32(uint8_t *p, uint32_t v);
uint32_t get_le32(const uint8_t *p);

#endif // XLOG_ENDIAN_H
