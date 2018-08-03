
#include "qzip_cookie.h"

#include <stdio.h>
#include <assert.h>

#define CHUNK (16*1024)

static char buf[CHUNK];

static int def(FILE *fin, FILE *fout)
{
    size_t bytes_read = 0;

    FILE *fout_ = qzip_hook(fout, "w");
    assert(fout_ != NULL);

    do {
        bytes_read = fread(buf, 1, CHUNK, fin);
        fwrite(buf, 1, bytes_read, fout_);
    } while (bytes_read == CHUNK);
}

int main(int argc, char **argv)
{
    int ret = 0;

    ret = def(stdin, stdout);
    assert(ret == 0);

    return 0;
}
