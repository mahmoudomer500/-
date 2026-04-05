#ifndef DUMMY_OPENSSL_BIO_H
#define DUMMY_OPENSSL_BIO_H

#include <stddef.h>
#include "openssl/buffer.h"

typedef struct bio_st BIO;
typedef struct bio_method_st BIO_METHOD;

#define BIO_FLAGS_BASE64_NO_NL 0x100

inline BIO* BIO_new(const BIO_METHOD* type) { return 0; }
inline const BIO_METHOD* BIO_s_mem() { return 0; }
inline const BIO_METHOD* BIO_f_base64() { return 0; }
inline BIO* BIO_push(BIO* b, BIO* append) { return b; }
inline void BIO_set_flags(BIO* b, int flags) {}
inline int BIO_write(BIO* b, const void* data, int dlen) { return 0; }
inline int BIO_read(BIO* b, void* data, int dlen) { return 0; }
inline int BIO_flush(BIO* b) { return 0; }
inline void BIO_free_all(BIO* a) {}

inline long BIO_get_mem_ptr(BIO* b, BUF_MEM** pp) { return 0; }

#endif
