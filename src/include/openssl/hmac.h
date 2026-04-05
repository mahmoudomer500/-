#ifndef DUMMY_OPENSSL_HMAC_H
#define DUMMY_OPENSSL_HMAC_H

#include <stdint.h>
#include <stddef.h>
#include "openssl/sha.h"
#include "openssl/evp.h"

inline unsigned char *HMAC(const void *evp_md, const void *key, int key_len,
                    const unsigned char *d, size_t n, unsigned char *md,
                    unsigned int *md_len) { return md; }

#endif
