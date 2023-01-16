#include "hexdump.h"

void hexdump(void *buf, size_t sz) {
    printf("size: %zu\n", sz);
    for (size_t i = 0; i < sz / 16; i++) {
        printf("%08zx: ", i * 16);
        for (size_t j = 0; j < 16; j++) {
            printf("%02x ", ((uint8_t *)buf)[i * 16 + j]);
        }
        printf(" ");
        for (size_t j = 0; j < 16; j++) {
            printf("%c", ((uint8_t *)buf)[i * 16 + j]);
        }
        printf("\n");
    }
}
