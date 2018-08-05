// vim: set sw=4 ts=4 sts=4 et tw=78
//
// ATTENTION: Just check if API is runnable but not check it's correctness.
//

#include "qzip_cookie.h"

#include <stdio.h>
#include <assert.h>

#define MAXBUF  (2*1024*1024)

static char buf[MAXBUF];

int main(void)
{
    const char *fin_name = "calgary";
    size_t bytes_read = 0;
    FILE *fin = NULL;

    fin = fopen(fin_name, "r");
    assert(fin != NULL);

//    // \begin test gzip cookie
//    sprintf(buf, "%s.gz", fin_name);
//    FILE *gz_fout = gzip_fopen(buf, "w");
//    assert(gz_fout != NULL);
//
//    do {
//        bytes_read = fread(buf, 1, MAXBUF, fin);
//        fwrite(buf, 1, bytes_read, gz_fout);
//    } while (bytes_read == MAXBUF);
//    fclose(gz_fout);
//    fclose(fin);
//
//    printf("Test gzip done\n");
//    // \end test gzip cookie
//
//    // \begin test qzip cookie
//    fin = fopen(fin_name, "r");
//    assert(fin != NULL);
//
//    sprintf(buf, "%s.qz", fin_name);
//    FILE *qz_fout = qzip_fopen(buf, "w");
//    assert(qz_fout != NULL);
//
//    do {
//        bytes_read = fread(buf, 1, MAXBUF, fin);
//        fwrite(buf, 1, bytes_read, qz_fout);
//        //fflush(qz_fout);
//    } while (bytes_read == MAXBUF);
//    fclose(qz_fout);
//    fclose(fin);
//
//    printf("Test qzip done\n");
//    // \end test qzip cookie

    // \begin test qzip stream cookie
    fin = fopen(fin_name, "r");
    assert(fin != NULL);

    sprintf(buf, "%s.qz_s", fin_name);
    FILE *qz_s_fout = qzip_stream_fopen(buf, "w");
    assert(qz_s_fout != NULL);
    do {
        bytes_read = fread(buf, 1, MAXBUF, fin);
        fwrite(buf, 1, bytes_read, qz_s_fout);
    } while (bytes_read == MAXBUF);
    fclose(qz_s_fout);
    fclose(fin);

    printf("Test qzip stream done\n");
    // \end test qzip stream cookie

    return 0;
}
