#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

extern SSL_CTX* ctx;

int set_nonblocking(int fd);
int init_socket(int port);