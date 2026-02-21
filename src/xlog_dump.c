#include "xlog.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void hexdump(void *buf, size_t sz) {
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
            printf("%c", isprint(((uint8_t *)buf)[sz / 16 * 16 + j]) ? ((uint8_t *)buf)[sz / 16 * 16 + j] : '.');
        }
        printf("|\n");
    }
    printf("%08zx\n", sz);
}

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Usage: xlog_dump <xlog>\n");
        return EXIT_FAILURE;
    }

    xlog_reader_t *r = xlog_reader_open(argv[1]);
    if(!r) {
        fprintf(stderr, "Failed to open xlog file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    char *buf;
    ssize_t sz;

    while((sz = xlog_reader_next(r, (void **)&buf)) > 0) {
        hexdump(buf, sz);
        printf("\n");
        free(buf);
    }

    if(sz < 0) {
        fprintf(stderr, "Error reading xlog: %zd\n", sz);
        xlog_reader_close(r);
        return EXIT_FAILURE;
    }

    xlog_reader_close(r);

    return EXIT_SUCCESS;
}
