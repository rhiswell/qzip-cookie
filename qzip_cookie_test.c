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
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

#include "qatzip.h"

#define MAXDATA QC_MAXDATA
#define MAXPATH (1024)

static char fpath_buf[MAXPATH];
static char fdata_buf[MAXDATA];

typedef struct {
    struct timeval time_s;
    struct timeval time_e;
} run_time_t;

static run_time_t run_time;

// Refer to QATzip/utils/qzip.c:displayStats
void display_stats(run_time_t *run_time, unsigned int insize)
{
    unsigned long us_begin = 0;
    unsigned long us_end   = 0;
    double us_diff         = 0;

    us_begin = run_time->time_s.tv_sec * 1e6 + run_time->time_s.tv_usec;
    us_end   = run_time->time_e.tv_sec * 1e6 + run_time->time_e.tv_usec;
    us_diff  = (us_end - us_begin);

    assert(0 != us_diff);
    assert(0 != insize);
    double throughput = (insize * 8) / us_diff;     // in MBit/s

    printf("Time taken:     %9.3lf ms\n", us_diff / 1000);
    printf("Throughput:     %9.3lf Mbit/s\n", throughput);
}

void display_speedup(run_time_t *run_time_old, run_time_t *run_time_new)
{
    unsigned long us_begin_old, us_end_old;
    unsigned long us_begin_new, us_end_new;
    double us_diff_old, us_diff_new;

    us_begin_old = run_time_old->time_s.tv_sec * 1e6 + run_time_old->time_s.tv_usec;
    us_end_old   = run_time_old->time_e.tv_sec * 1e6 + run_time_old->time_e.tv_usec;
    us_diff_old  = (us_end_old - us_begin_old);

    us_begin_new = run_time_new->time_s.tv_sec * 1e6 + run_time_new->time_s.tv_usec;
    us_end_new   = run_time_new->time_e.tv_sec * 1e6 + run_time_new->time_e.tv_usec;
    us_diff_new  = (us_end_new - us_begin_new);

    assert(us_diff_old > 0);
    assert(us_diff_new > 0);
    double speedup = (us_diff_old / us_diff_new);

    printf("Speedup:        %9.3lf x\n", speedup);
}

off_t file_size(const char *fpath)
{
    struct stat fstat;

    int rc = stat(fpath, &fstat);
    assert(rc == 0);

    return fstat.st_size;
}

static void def(FILE *fin, FILE *fout)
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    do {
        bytes_read = fread(fdata_buf, 1, MAXDATA, fin);
        QC_DEBUG("Reading input file (%d Bytes) to buffer at %x\n", bytes_read, fdata_buf);
        bytes_written = fwrite(fdata_buf, 1, bytes_read, fout);
        assert(bytes_written == bytes_read);
    } while (bytes_read == MAXDATA);
}

void test_gzip(const char *fpath)
{
    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.gz", fpath);
    FILE *gz_fout = gzip_fopen(fpath_buf, "w");
    assert(gz_fout != NULL);

    gettimeofday(&run_time.time_s, NULL);
    def(fin, gz_fout);
    gettimeofday(&run_time.time_e, NULL);

    fclose(gz_fout);
    fclose(fin);

    printf("Test gzip done\n");
    display_stats(&run_time, file_size(fpath));
}

void test_qzip(const char *fpath)
{
    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.qz", fpath);
    FILE *qz_fout = qzip_fopen(fpath_buf, "w");
    assert(qz_fout != NULL);

    gettimeofday(&run_time.time_s, NULL);
    def(fin, qz_fout);
    gettimeofday(&run_time.time_e, NULL);

    fclose(qz_fout);
    fclose(fin);

    printf("Test qzip done\n");
    display_stats(&run_time, file_size(fpath));
}

// This function will write compressed data to stderr
void bench_qzip(const char *fpath, int chunk_size)
{
    run_time_t base_run_time;
    run_time_t my_run_time;

    // Use mmap to eliminate overhead of kernel-to-user memory copy
    int fd = open(fpath, O_RDONLY);
    assert(fd >= 0);

    size_t fsize = file_size(fpath);
    assert(fsize > 0);

    char *addr = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(addr != NULL);

    size_t bytes_to_write, bytes_written, off;

    // \begin bench baseline
    // `qzip_hook` has disabled IO stream's internal buffer so that call to `fwrite`
    // will directly deliver data-to-write to `qzip_write` and then processed by QAT
    FILE *fout = qzip_hook(stderr, "w");
    assert(fout != NULL);
    gettimeofday(&base_run_time.time_s, NULL);
    for (off = 0; off < fsize; off += chunk_size) {
        bytes_to_write = ((fsize - off) < chunk_size) ? (fsize - off) : chunk_size;
        bytes_written  = fwrite(addr + off, 1, bytes_to_write, fout);
        assert(bytes_written == bytes_to_write);
    }
    fclose(fout);
    gettimeofday(&base_run_time.time_e, NULL);
    display_stats(&base_run_time, fsize);
    // \end bench baseline

    // \begin bench my version
    FILE *my_fout = my_qzip_hook(stderr, "w");
    assert(my_fout != NULL);
    gettimeofday(&my_run_time.time_s, NULL);
    for (off = 0; off < fsize; off += chunk_size) {
        bytes_to_write = ((fsize - off) < chunk_size) ? (fsize - off) : chunk_size;
        bytes_written  = fwrite(addr + off, 1, bytes_to_write, my_fout);
    }
    fclose(my_fout);
    gettimeofday(&my_run_time.time_e, NULL);
    display_stats(&my_run_time, fsize);
    // \end bench my version

    display_speedup(&base_run_time, &my_run_time);
}

// This function will write compressed data to stderr
void test_my_qzip(const char *fpath, int chunk_size)
{
    run_time_t my_run_time;

    // Use mmap to eliminate overhead of kernel-to-user memory copy
    int fd = open(fpath, O_RDONLY);
    assert(fd >= 0);

    size_t fsize = file_size(fpath);
    assert(fsize > 0);

    char *addr = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(addr != NULL);

    size_t bytes_to_write, bytes_written, off;

    FILE *my_fout = my_qzip_hook(stderr, "w");
    assert(my_fout != NULL);
    gettimeofday(&my_run_time.time_s, NULL);
    for (off = 0; off < fsize; off += chunk_size) {
        bytes_to_write = ((fsize - off) < chunk_size) ? (fsize - off) : chunk_size;
        bytes_written  = fwrite(addr + off, 1, bytes_to_write, my_fout);
    }
    fclose(my_fout);
    gettimeofday(&my_run_time.time_e, NULL);
    display_stats(&my_run_time, fsize);
}

void test_qzip_stream(const char *fpath)
{
    FILE *fin = fopen(fpath, "r");
    assert(fin != NULL);

    sprintf(fpath_buf, "%s.qz_s", fpath);
    FILE *qz_s_fout = qzip_stream_fopen(fpath_buf, "w");
    assert(qz_s_fout != NULL);

    gettimeofday(&run_time.time_s, NULL);
    def(fin, qz_s_fout);
    gettimeofday(&run_time.time_e, NULL);

    fclose(qz_s_fout);
    fclose(fin);

    printf("Test qzip stream done\n");
    display_stats(&run_time, file_size(fpath));
}

void print_usage(const char *progname)
{
    printf("Usage: %s [options] <file_to_test>\n", progname);
    printf("Program options:\n");
    printf("    -c  --case <INT>    Test specified cookie API (default 0 that means all)\n");
    printf("    -s  --chunksz <INT> Size to write (default 64)\n");
    printf("    -h  --help          This message\n");
}

int main(int argc, char **argv)
{
    int  test_case  = 0;
    int  chunk_size = (64*1024);    // 64 KB
    char *fin_path  = NULL;

    // \begin parse commandline args
    int opt;

    static struct option long_options[] = {
        {"case",    required_argument, 0, 'c'},
        {"chunksz", required_argument, 0, 's'},
        {"help",    no_argument,       0, 'h'},
        {0,         0,                 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "f:c:s:h", long_options, NULL)) != -1) {

        switch (opt) {
            case 'c':
                test_case = atoi(optarg);
                break;
            case 's':
                chunk_size = atoi(optarg);
                assert(chunk_size > 0);
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

    // case 1..3: read from file and write into file at the same directory
    // case 4..5: read from mmapped file and write into stderr
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
        case 4:
            test_my_qzip(fin_path, chunk_size);
            break;
        case 5:
            bench_qzip(fin_path, chunk_size);
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
