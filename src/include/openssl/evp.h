#ifndef DUMMY_OPENSSL_EVP_H
#define DUMMY_OPENSSL_EVP_H

#include <stddef.h>

typedef void EVP_MD;
inline const EVP_MD* EVP_sha256() { return (const EVP_MD*)0; }

inline void EVP_cleanup() {}

#endif
