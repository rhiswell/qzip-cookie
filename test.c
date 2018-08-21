// vim: set sw=4 ts=4 sts=4 et tw=78
//
// ATTENTION: Just check if API is runnable but not check it's correctness.
//

#include "qzip_cookie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#define MAXDATA  (2*1024*1024)
#define MAXPATH 1024

static char fpath_buf[MAXPATH];
static char fdata_buf[MAXDATA];

static void def(FILE *fin, FILE *fout)
{
    size_t bytes_read = 0;

    do {
        bytes_read = fread(fdata_buf, 1, MAXDATA, fin);
        fwrite(fdata_buf, 1, bytes_read, fout);
    } while (bytes_read == MAXDATA);
}

void test_gzip(const char *fpath)
{

    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.gz", fpath);
    FILE *gz_fout = gzip_fopen(fpath_buf, "w");
    assert(gz_fout != NULL);
    def(fin, gz_fout);
    fclose(gz_fout);
    fclose(fin);

    printf("Test gzip done\n");
}

void test_qzip(const char *fpath)
{
    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.qz", fpath);
    FILE *qz_fout = qzip_fopen(fpath_buf, "w");
    assert(qz_fout != NULL);
    def(fin, qz_fout);
    fclose(qz_fout);
    fclose(fin);

    printf("Test qzip done\n");
}

void test_qzip_stream(const char *fpath)
{
    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.qz_s", fpath);
    FILE *qz_s_fout = qzip_stream_fopen(fpath_buf, "w");
    assert(qz_s_fout != NULL);
    def(fin, qz_s_fout);
    fclose(qz_s_fout);
    fclose(fin);

    printf("Test qzip stream done\n");
}

void print_usage(const char *progname)
{
    printf("Usage: %s [options] <file_to_test>\n", progname);
    printf("Program options:\n");
    printf("    -c  --case <INT>    Test specified cookie API\n");
    printf("    -h  --help          This message\n");
}

int main(int argc, char **argv)
{
    int  test_case = 0;
    char *fin_path = NULL;

    // \begin parse commandline args
    int opt;

    static struct option long_options[] = {
        {"case", required_argument, 0, 'c'},
        {"help", no_argument,       0, 'h'},
        {0,      0,                 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "f:c:h", long_options, NULL)) != -1) {

        switch (opt) {
            case 'c':
                test_case = atoi(optarg);
                break;
            case 'h':
            case '?':
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        fin_path = argv[optind++];
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    // \end parse commandline args

    switch (test_case) {
        case 1:
            test_gzip(fin_path);
            break;
        case 2:
            test_qzip(fin_path);
            break;
        case 3:
            test_qzip_stream(fin_path);
            break;
        case 0:
        default:
            test_gzip(fin_path);
            test_qzip(fin_path);
            test_qzip_stream(fin_path);
            break;
    }

    return 0;
}
