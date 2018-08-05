#ifndef _QZIP_COOKIE_H
#define _QZIP_COOKIE_H

#define _GNU_SOURCE
#include <stdio.h>

FILE * gzip_fopen(const char *fname, const char *mode);

FILE * qzip_fopen(const char *fname, const char *mode);
FILE * qzip_hook(FILE *fp, const char *mode);

FILE * qzip_stream_fopen(const char *fname, const char *mode);
FILE * qzip_stream_hook(FILE *fp, const char *mode);

#endif  // _QZIP_COOKIE_H
