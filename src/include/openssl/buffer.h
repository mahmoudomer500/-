#ifndef DUMMY_OPENSSL_BUFFER_H
#define DUMMY_OPENSSL_BUFFER_H

#include <stddef.h>

typedef struct buf_mem_st {
    size_t length;
    char* data;
    size_t max;
} BUF_MEM;

#endif
