#include "xlog.h"
#include "hexdump.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

    bool first = true;
    while((sz = xlog_reader_next(r, (void **)&buf)) > 0) {
        if(!first) {
            printf("\n");
        }
        hexdump(buf, sz);
        free(buf);

        first = false;
    }

    if(sz < 0) {
        fprintf(stderr, "Error reading xlog: %zd\n", sz);
        xlog_reader_close(r);
        return EXIT_FAILURE;
    }

    xlog_reader_close(r);

    return EXIT_SUCCESS;
}
