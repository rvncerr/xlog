#include "hexdump.h"

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

void hexdump(void *buf, size_t sz) {
    for (size_t i = 0; i < sz / 16; i++) {
        printf("%08zx  ", i * 16);
        for (size_t j = 0; j < 16; j++) {
            if(j == 8) printf(" ");
            printf("%02x ", ((uint8_t *)buf)[i * 16 + j]);
        }
        printf("  |");
        for (size_t j = 0; j < 16; j++) {
            printf("%c", isprint(((uint8_t *)buf)[i * 16 + j]) ? ((uint8_t *)buf)[i * 16 + j] : '.');
        }
        printf("|\n");
    }
    if (sz % 16) {
        printf("%08zx  ", sz / 16 * 16);
        for (size_t j = 0; j < sz % 16; j++) {
            if(j == 8) printf(" ");
            printf("%02x ", ((uint8_t *)buf)[sz / 16 * 16 + j]);
        }
        for (size_t j = 0; j < 16 - sz % 16; j++) {
            if(sz % 16 + j == 8) printf(" ");
            printf("   ");
        }
        printf("  |");
        for (size_t j = 0; j < sz % 16; j++) {
            printf("%c", isprint(((uint8_t *) buf)[sz / 16 * 16 + j]) ? ((uint8_t *) buf)[sz / 16 * 16 + j] : '.');
        }
        printf("|\n");
    }
    printf("%08zx\n", sz);
}
