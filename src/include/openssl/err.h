#ifndef OPENSSL_ERR_H
#define OPENSSL_ERR_H

#include <openssl/ssl.h>

void ERR_free_strings(void);
void ERR_load_BIO_strings(void);
void ERR_print_errors_fp(FILE *fp);
char *ERR_error_string(unsigned long e, char *buf);
unsigned long ERR_get_error(void);

#endif
