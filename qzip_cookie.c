// vim: set sw=4 ts=4 sts=4 et tw=78

#include "qzip_cookie.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <zlib.h>
#include "qz_utils.h"
#include "qatzip.h"

#define _MAXBUF  4096

// \begin gzip cookie
// Details of cookie can refer to `man fopencookie`
static ssize_t
gzip_cookie_write(void *cookie, const char *buf, size_t size)
{
    return gzwrite((gzFile)cookie, (voidpc)buf, (unsigned)size);
}

static int
gzip_cookie_close(void *cookie)
{
    return gzclose((gzFile)cookie);
}

static cookie_io_functions_t gzip_write_funcs = {
    .write = gzip_cookie_write,
    .close = gzip_cookie_close
};

FILE *
gzip_fopen(const char *fname, const char *mode)
{
    gzFile zfd = gzopen(fname, mode);
    return fopencookie(zfd, mode, gzip_write_funcs);
}
// \end gzip cookie

// \begin qzip cookie
typedef struct {
    QzSession_T qz_sess;
    FILE        *fp;
} qzip_cookie_t;

static ssize_t
qzip_cookie_write(void *cookie, const char *buf, size_t size)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)(cookie);
    QzSession_T *qz_sess = &(qz_cookie->qz_sess);
    char *src = buf;
    unsigned int src_len = size;
    // Ensure output buffer is big enough to push input buffer
    unsigned int dst_len = (size < _MAXBUF) ? _MAXBUF : size;
    unsigned int done = 0;
    unsigned int buf_processed = 0;
    unsigned int buf_remaining = size;
    unsigned int bytes_written = 0;
    unsigned int valid_dst_len = dst_len;
    int rc = QZ_FAIL;

    // Size of output buf is equal with input buf's.
    char *dst = (char *)malloc(dst_len);
    assert(dst != NULL);

    while (!done) {
        rc = qzCompress(qz_sess, src, &src_len, dst, &dst_len, 1);

        if (rc != QZ_OK &&
            rc != QZ_BUF_ERROR &&
            rc != QZ_DATA_ERROR) {
            QZ_ERROR("qzip_cookie_write: failed with error: %d\n", rc);
            QZ_ERROR("qzip_cookie_write: src_len %d, dst_len %d\n", src_len, dst_len);
            break;
        }

        bytes_written = fwrite(dst, 1, dst_len, qz_cookie->fp);
        assert(bytes_written == dst_len);

        buf_processed += src_len;
        buf_remaining -= src_len;
        if (buf_remaining == 0) {
            done = 1;
        }
        src += src_len;
        src_len = buf_remaining;
        dst_len = valid_dst_len;
    }

    free(dst);

    return buf_processed;
}

static int
qzip_cookie_close(void *cookie)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)(cookie);
    QzSession_T *qz_sess = &(qz_cookie->qz_sess);

    fclose(qz_cookie->fp);
    qzTeardownSession(qz_sess);
    qzClose(qz_sess);
    free(qz_cookie);

    return 0;
}

static int
qzip_cookie_close2(void *cookie)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)(cookie);
    QzSession_T *qz_sess = &(qz_cookie->qz_sess);

    //fclose(qz_cookie->fp);
    qzTeardownSession(qz_sess);
    qzClose(qz_sess);
    free(qz_cookie);

    return 0;

}

static cookie_io_functions_t qzip_write_funcs = {
    .write = qzip_cookie_write,
    .close = qzip_cookie_close
};


static cookie_io_functions_t qzip_write_funcs_v2 = {
    .write = qzip_cookie_write,
    .close = qzip_cookie_close2
};

FILE *
qzip_fopen(const char *fname, const char *mode)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)calloc(1, sizeof(qzip_cookie_t));
    qz_cookie->fp = fopen(fname, mode);
    assert(qz_cookie->fp != NULL);
    return fopencookie(qz_cookie, mode, qzip_write_funcs);
}

FILE *
qzip_hook(FILE *fp, const char *mode)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)calloc(1, sizeof(qzip_cookie_t));
    qz_cookie->fp = fp;

    return fopencookie(qz_cookie, mode, qzip_write_funcs_v2);
}
// \end qzip cookie
