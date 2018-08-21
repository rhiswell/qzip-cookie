// vim: set sw=4 ts=4 sts=4 et tw=78

#include "qzip_cookie.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <zlib.h>
#include "cpa.h"
#include "cpa_dc.h"
#include "qz_utils.h"
#include "qatzip.h"
#include "qatzip_internal.h"

#define PAGESIZE (4*1024)
#define HUGEPAGE (2*1024*1024)

#ifdef QZ_COOKIE_DEBUG
#define DEBUG_INFO(fmt, args...) { printf("DEBUG: " fmt "\n", ##args); }
#else
#define DEBUG_INFO(fmt, args...) { do {} while(0); }
#endif

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

// \begin buffer manager
typedef struct {
    char            *buf;
    unsigned int    size;
    unsigned int    consumed;
} bufm_t;

static inline void
bufm_flush(bufm_t *bufm, FILE *fout)
{
    if (bufm->consumed > 0) {
        size_t bytes_written = fwrite(bufm->buf, 1, bufm->consumed, fout);
        assert(bytes_written == bufm->consumed);
        bufm->consumed = 0;
    }
}

static inline int
bufm_init(bufm_t *bufm, unsigned int size)
{
    if (NULL == (bufm->buf = (char *)malloc(size))) {
        return 1;
    }

    else
    {
        bufm->size = size;
        bufm->consumed = 0;
    }

    return 0;
}

static inline void
bufm_destor(bufm_t *bufm)
{
    free(bufm->buf);
}
// \end buffer manager

// \begin qzip cookie
typedef struct {
    QzSession_T       qz_sess;
    QzSessionParams_T qz_sess_params;
    FILE              *fp;
} qzip_cookie_t;

// Refer to QATzip/utils/qzip.c:doProcessFile
static ssize_t
qzip_cookie_write(void *cookie, const char *buf, size_t size)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)(cookie);
    QzSession_T *qz_sess = &(qz_cookie->qz_sess);
    const char *src = buf;
    unsigned int src_len = size;
    // Ensure output buffer is big enough to push input buffer
    unsigned int dst_len = (size < PAGESIZE) ? PAGESIZE : size;
    unsigned int done = 0;
    unsigned int buf_processed = 0;
    unsigned int buf_remaining = size;
    unsigned int bytes_written = 0;
    unsigned int valid_dst_len = dst_len;
    int rc = QZ_FAIL;

    char *dst = (char *)malloc(dst_len);
    assert(dst != NULL);

    fprintf(stderr, "qzip_cookie_write: new buf with size %d\n", size);

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

// This API is used for pipe-like program where input/output file is
// stdio/stdout seperatly.
static int
qzip_cookie_close2(void *cookie)
{
    qzip_cookie_t *qz_cookie = (qzip_cookie_t *)(cookie);
    QzSession_T *qz_sess = &(qz_cookie->qz_sess);

    // Won't close stdout
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


static cookie_io_functions_t qzip_write2_funcs = {
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

    return fopencookie(qz_cookie, mode, qzip_write2_funcs);
}
// \end qzip cookie

// \begin qzip stream cookie
// Refer to test/main.c:qzCompressStreamAndDecompress
typedef struct {
    QzSession_T       qz_sess;
    QzSessionParams_T qz_sess_params;
    QzStream_T        qz_strm;
    bufm_t            qz_strm_bufm;
    FILE              *fp;
} qzip_stream_cookie_t;

static ssize_t
qzip_stream_cookie_write(void *cookie, const char *buf, size_t size)
{
    qzip_stream_cookie_t *qz_stream_cookie =
        (qzip_stream_cookie_t *)cookie;
    QzSession_T *qz_sess    = &(qz_stream_cookie->qz_sess);
    QzStream_T *qz_strm     = &(qz_stream_cookie->qz_strm);
    bufm_t *qz_strm_bufm    = &(qz_stream_cookie->qz_strm_bufm);
    const char *src         = buf;
    unsigned int src_len    = size;
    unsigned int slice_sz   = (qz_stream_cookie->qz_sess_params).hw_buff_sz / 4;
    unsigned int done       = 0;
    unsigned int consumed   = 0;
    unsigned int input_left = src_len - consumed;
    unsigned int bytes_written;
    int rc;

    QZ_DEBUG("qzip_stream_cookie_write: new buf (%d)\n", size);

    do {
        qz_strm->in     = src + consumed;
        qz_strm->out    = qz_strm_bufm->buf + qz_strm_bufm->consumed;
        qz_strm->in_sz  = (input_left > slice_sz) ? slice_sz : input_left;
        qz_strm->out_sz = qz_strm_bufm->size - qz_strm_bufm->consumed;

        QZ_DEBUG("qzip_stream_cookie_write: before: to_in %7d (%7d pending), remain %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);

        rc = qzCompressStream(qz_sess, qz_strm, 0);
        if (rc != QZ_OK) {
            QZ_ERROR("qzip_stream_cookie_write: failed with error: %d\n", rc);
            QZ_ERROR("qzip_stream_cookie_write: input_left %d, output_left %d\n",
                     input_left, qz_strm_bufm->size - qz_strm_bufm->consumed);
            break;
        }

        {
            consumed                += qz_strm->in_sz;
            qz_strm_bufm->consumed  += qz_strm->out_sz;

            input_left = src_len - consumed;
        }

        QZ_DEBUG("qzip_stream_cookie_write:  after: in_ed %7d (%7d pending), output %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);
        QZ_DEBUG("qzip_stream_cookie_write:  after: total consumed %d, input_left %d\n",
                 consumed, input_left);

        // When internal buffer is full, do data flush then reset buffer cursor
        if (qz_strm_bufm->consumed == qz_strm_bufm->size) {
            bufm_flush(qz_strm_bufm, qz_stream_cookie->fp);
        }
    } while (input_left);

    QZ_DEBUG("qzip_stream_cookie_write: end buf\n");

    return consumed;
}

static int
qzip_stream_cookie_close(void *cookie)
{
    qzip_stream_cookie_t *qz_stream_cookie =
        (qzip_stream_cookie_t *)cookie;
    QzSession_T *qz_sess    = &(qz_stream_cookie->qz_sess);
    QzStream_T *qz_strm     = &(qz_stream_cookie->qz_strm);
    bufm_t *qz_strm_bufm    = &(qz_stream_cookie->qz_strm_bufm);

    // Flush data buffer to make room
    bufm_flush(qz_strm_bufm, qz_stream_cookie->fp);
    // Flush pendding data in QAT stream, then flush data buffer
    int done = (0 == qz_strm->pending_in && 0 == qz_strm->pending_out) ? 1 : 0;
    while (!done) {
        qz_strm->in = NULL;
        qz_strm->out = qz_strm_bufm->buf + qz_strm_bufm->consumed;
        qz_strm->in_sz = 0;
        qz_strm->out_sz = qz_strm_bufm->size - qz_strm_bufm->consumed;

        QZ_DEBUG("qzip_stream_cookie_close: before: to_in %7d (%7d pending), remain %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);

        int rc = qzCompressStream(qz_sess, qz_strm, 1);
        if (rc != QZ_OK) {
            QZ_ERROR("qzip_stream_cookie_close: failed with error: %d\n", rc);
            break;
        }

        QZ_DEBUG("qzip_stream_cookie_close:  after: in_ed %7d (%7d pending), output %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);

        qz_strm_bufm->consumed  += qz_strm->out_sz;

        done = (0 == qz_strm->pending_in && 0 == qz_strm->pending_out) ? 1 : 0;
    } {
        bufm_flush(qz_strm_bufm, qz_stream_cookie->fp);
    }

    fclose(qz_stream_cookie->fp);
    bufm_destor(qz_strm_bufm);

    qzEndStream(qz_sess, qz_strm);
    qzTeardownSession(qz_sess);
    qzClose(qz_sess);
    free(qz_stream_cookie);

    return 0;
}

static cookie_io_functions_t qzip_stream_write_funcs = {
    .write = qzip_stream_cookie_write,
    .close = qzip_stream_cookie_close
};

FILE *
qzip_stream_fopen(const char *fname, const char *mode)
{
    qzip_stream_cookie_t *qz_stream_cookie =
        (qzip_stream_cookie_t *)calloc(1, sizeof(qzip_stream_cookie_t));
    assert(qz_stream_cookie != NULL);

    QzStream_T *qz_strm               = &(qz_stream_cookie->qz_strm);
    QzSessionParams_T *qz_sess_params = &(qz_stream_cookie->qz_sess_params);
    int rc;

    rc = qzGetDefaults(qz_sess_params);
    assert(rc == QZ_OK);

    bufm_t *qz_strm_bufm = &(qz_stream_cookie->qz_strm_bufm);
    // Allocate an internal buffer to save stream's output
    rc = bufm_init(qz_strm_bufm, HUGEPAGE);
    assert(rc == 0);

    // Open specific file to save compressed data
    qz_stream_cookie->fp = fopen(fname, mode);
    assert(qz_stream_cookie->fp != NULL);

    return fopencookie(qz_stream_cookie, mode, qzip_stream_write_funcs);
}

static int
qzip_stream_cookie_close2(void *cookie)
{
    qzip_stream_cookie_t *qz_stream_cookie =
        (qzip_stream_cookie_t *)cookie;
    QzSession_T *qz_sess    = &(qz_stream_cookie->qz_sess);
    QzStream_T *qz_strm     = &(qz_stream_cookie->qz_strm);
    bufm_t *qz_strm_bufm    = &(qz_stream_cookie->qz_strm_bufm);

    // Flush data buffer to make room
    bufm_flush(qz_strm_bufm, qz_stream_cookie->fp);
    // Flush pendding data in QAT stream, then flush data buffer
    int done = (0 == qz_strm->pending_in && 0 == qz_strm->pending_out) ? 1 : 0;
    while (!done) {
        qz_strm->in = NULL;
        qz_strm->out = qz_strm_bufm->buf + qz_strm_bufm->consumed;
        qz_strm->in_sz = 0;
        qz_strm->out_sz = qz_strm_bufm->size - qz_strm_bufm->consumed;

        QZ_DEBUG("qzip_stream_cookie_close: before: to_in %7d (%7d pending), remain %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);

        int rc = qzCompressStream(qz_sess, qz_strm, 1);
        if (rc != QZ_OK) {
            QZ_ERROR("qzip_stream_cookie_close: failed with error: %d\n", rc);
            break;
        }

        QZ_DEBUG("qzip_stream_cookie_close:  after: in_ed %7d (%7d pending), output %7d (%7d pending)\n",
                 qz_strm->in_sz, qz_strm->pending_in, qz_strm->out_sz, qz_strm->pending_out);

        qz_strm_bufm->consumed  += qz_strm->out_sz;

        done = (0 == qz_strm->pending_in && 0 == qz_strm->pending_out) ? 1 : 0;
    } {
        bufm_flush(qz_strm_bufm, qz_stream_cookie->fp);
    }

    //fclose(qz_stream_cookie->fp);
    bufm_destor(qz_strm_bufm);

    qzEndStream(qz_sess, qz_strm);
    qzTeardownSession(qz_sess);
    qzClose(qz_sess);
    free(qz_stream_cookie);

    return 0;
}

static cookie_io_functions_t qzip_stream_write2_funcs = {
    .write = qzip_stream_cookie_write,
    .close = qzip_stream_cookie_close2
};

FILE *
qzip_stream_hook(FILE *fp, const char *mode)
{
    qzip_stream_cookie_t *qz_stream_cookie =
        (qzip_stream_cookie_t *)calloc(1, sizeof(qzip_stream_cookie_t));
    assert(qz_stream_cookie != NULL);

    QzStream_T *qz_strm               = &(qz_stream_cookie->qz_strm);
    QzSessionParams_T *qz_sess_params = &(qz_stream_cookie->qz_sess_params);
    int rc;

    rc = qzGetDefaults(qz_sess_params);
    assert(rc == QZ_OK);

    bufm_t *qz_strm_bufm = &(qz_stream_cookie->qz_strm_bufm);
    // Allocate an internal buffer to save stream's output
    rc = bufm_init(qz_strm_bufm, HUGEPAGE);
    assert(rc == 0);

    //qz_stream_cookie->fp = fopen(fname, mode);
    //assert(qz_stream_cookie->fp != NULL);
    qz_stream_cookie->fp = fp;

    return fopencookie(qz_stream_cookie, mode, qzip_stream_write2_funcs);
}
// \end qzip stream cookie
