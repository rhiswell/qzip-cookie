#ifndef _QZIP_COOKIE_H
#define _QZIP_COOKIE_H

#define _GNU_SOURCE
#include <stdio.h>

#define QC_MAXDATA  (512*1024*1024)

#ifdef QZ_COOKIE_DEBUG
#define QC_DEBUG(fmt, args...) { fprintf(stderr, "DEBUG: " fmt, ##args); }
#else
#define QC_DEBUG(fmt, args...) { do {} while(0); }
#endif

#define QC_PRINT(fmt, args...) { fprintf(stderr, "INFO: " fmt, ##args); }
#define QC_ERROR(fmt, args...) { fprintf(stderr, "ERROR: " fmt, ##args); }

FILE * gzip_fopen(const char *fname, const char *mode);

FILE * qzip_fopen(const char *fname, const char *mode);
FILE * qzip_hook(FILE *fp, const char *mode);
FILE * my_qzip_hook(FILE *fp, const char *mode);

FILE * qzip_stream_fopen(const char *fname, const char *mode);
FILE * qzip_stream_hook(FILE *fp, const char *mode);

#endif  // _QZIP_COOKIE_H
