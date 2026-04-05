#ifndef DUMMY_OPENSSL_SHA_H
#define DUMMY_OPENSSL_SHA_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_DIGEST_LENGTH 32

typedef struct SHA256state_st {
    uint32_t h[8];
    uint32_t Nl, Nh;
    uint32_t data[16];
    unsigned int num, md_len;
} SHA256_CTX;

inline int SHA256_Init(SHA256_CTX *c) { return 1; }
inline int SHA256_Update(SHA256_CTX *c, const void *data, size_t len) { return 1; }
inline int SHA256_Final(unsigned char *md, SHA256_CTX *c) { return 1; }
inline unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md) { return md; }

#endif
