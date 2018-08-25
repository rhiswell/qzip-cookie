#ifndef _QZIP_COOKIE_H
#define _QZIP_COOKIE_H

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>

#define QC_MAXDATA  (512*1024*1024)

#ifdef QZ_COOKIE_DEBUG
#define QC_DEBUG(fmt, args...) { fprintf(stderr, "DEBUG: " fmt, ##args); }
#else
#define QC_DEBUG(fmt, args...) { do {} while(0); }
#endif

#define QC_PRINT(fmt, args...) { fprintf(stderr, "INFO: " fmt, ##args); }
#define QC_ERROR(fmt, args...) { fprintf(stderr, "ERROR: " fmt, ##args); }

typedef struct {
    struct timeval time_s;
    struct timeval time_e;
} run_time_t;

typedef struct run_time_list_node_ {
    run_time_t                 rtime;
    struct run_time_list_node_ *next;
} run_time_list_node_t;

#define LIST_NEW() \
    (run_time_list_node_t *)calloc(1, sizeof(run_time_list_node_t));

#define LIST_ADD(list_head, list_node)                          \
{                                                               \
    list_node->next = (list_head == NULL) ? NULL : list_head;   \
    list_head = list_node;                                      \
}

#define LIST_FOR(list_head, list_node)  \
    for (list_node = list_head; list_node != NULL; list_node = list_node->next)

FILE * gzip_fopen(const char *fname, const char *mode);

FILE * qzip_fopen(const char *fname, const char *mode);
FILE * qzip_hook(FILE *fp, const char *mode);
FILE * my_qzip_hook(FILE *fp, const char *mode);

FILE * qzip_stream_fopen(const char *fname, const char *mode);
FILE * qzip_stream_hook(FILE *fp, const char *mode);

#endif  // _QZIP_COOKIE_H
