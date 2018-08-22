
#include "qzip_cookie.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define CHUNK (512*1024*1024)

static char buf[CHUNK];

static int def_legacy(FILE *fin, FILE *fout)
{
    size_t bytes_read = 0;

    FILE *fout_ = qzip_hook(fout, "w");
    assert(fout_ != NULL);

    do {
        bytes_read = fread(buf, 1, CHUNK, fin);
        fwrite(buf, 1, bytes_read, fout_);
    } while (bytes_read == CHUNK);

    return 0;
}

static int def_stream(FILE *fin, FILE *fout)
{
    size_t bytes_read = 0;

    FILE *fout_ = qzip_stream_hook(fout, "w");
    assert(fout_ != NULL);;

    do {
        bytes_read = fread(buf, 1, CHUNK, fin);
        fwrite(buf, 1, bytes_read, fout_);
    } while (bytes_read == CHUNK);

    return 0;
}

int main(int argc, char **argv)
{
    int rc = 0;

    switch (argc) {
        case 1:
            rc = def_legacy(stdin, stdout);
            break;
        case 2:
            rc = def_stream(stdin, stdout);
            break;
        default:
            printf("Usage: %s [-s] < source > dest\n", argv[0]);
            exit(EXIT_FAILURE);
    }

    assert(rc == 0);

    return 0;
}
