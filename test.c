// vim: set sw=4 ts=4 sts=4 et tw=78

#include "qzip_cookie.h"

#include <stdio.h>
#include <assert.h>

#define MAXBUF  4096

static char buf[MAXBUF];

int main(void)
{
    const char *fin_name = "calgary";
    size_t bytes_read = 0;
    FILE *fin = NULL;

    fin = fopen(fin_name, "r");
    assert(fin != NULL);

    // \begin test gzip cookie
    sprintf(buf, "%s.gz", fin_name);
    FILE *gz_fout = gzip_fopen(buf, "w");
    assert(gz_fout != NULL);

    do {
        bytes_read = fread(buf, 1, MAXBUF, fin);
        fwrite(buf, 1, bytes_read, gz_fout);
    } while (bytes_read == MAXBUF);
    fclose(gz_fout);
    fclose(fin);

    printf("Test gzip done\n");
    // \end test gzip cookie

    // \begin test qzip cookie
    fin = fopen(fin_name, "r");
    assert(fin != NULL);

    sprintf(buf, "%s.qz", fin_name);
    FILE *qz_fout = qzip_fopen(buf, "w");
    assert(qz_fout != NULL);

    do {
        bytes_read = fread(buf, 1, MAXBUF, fin);
        fwrite(buf, 1, bytes_read, qz_fout);
        //fflush(qz_fout);
    } while (bytes_read == MAXBUF);
    fclose(qz_fout);
    fclose(fin);

    printf("Test qzip done\n");
    // \end test qzip cookie

    return 0;
}
