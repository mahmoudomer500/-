#ifndef DUMMY_OPENSSL_SSL_H
#define DUMMY_OPENSSL_SSL_H

#include <stdint.h>
#include "openssl/evp.h"

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

#define SSL_OP_NO_SSLv2 0x01000000L
#define SSL_OP_NO_SSLv3 0x02000000L
#define SSL_OP_NO_TLSv1 0x04000000L
#define SSL_OP_NO_TLSv1_1 0x10000000L
#define SSL_FILETYPE_PEM 1

inline const SSL_METHOD* TLS_server_method() { return 0; }
inline SSL_CTX* SSL_CTX_new(const SSL_METHOD* meth) { return 0; }
inline void SSL_CTX_free(SSL_CTX* ctx) {}
inline long SSL_CTX_set_options(SSL_CTX* ctx, long options) { return 0; }
inline int SSL_CTX_set_ecdh_auto(SSL_CTX* ctx, int onoff) { return 0; }
inline int SSL_CTX_use_certificate_file(SSL_CTX* ctx, const char* file, int type) { return 0; }
inline int SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file, int type) { return 0; }
inline int SSL_CTX_check_private_key(SSL_CTX* ctx) { return 0; }
inline int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* CAfile, const char* CApath) { return 0; }

inline SSL* SSL_new(SSL_CTX* ctx) { return 0; }
inline void SSL_free(SSL* ssl) {}
inline int SSL_set_fd(SSL* ssl, int fd) { return 0; }
inline int SSL_accept(SSL* ssl) { return 0; }
inline int SSL_shutdown(SSL* ssl) { return 0; }
inline int SSL_read(SSL* ssl, void* buf, int num) { return 0; }
inline int SSL_write(SSL* ssl, const void* buf, int num) { return 0; }

inline void SSL_load_error_strings() {}
inline void OpenSSL_add_ssl_algorithms() {}

#endif
