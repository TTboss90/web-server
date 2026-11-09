#define header_funcs
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"

SSL_CTX* ctx;

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void init_ssl()
{
	SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();


    ctx = SSL_CTX_new(TLS_server_method());

    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        return 1;
    }


    if (SSL_CTX_use_certificate_file(
            ctx,
            "server.crt",
            SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return 1;
    }


    if (SSL_CTX_use_PrivateKey_file(
            ctx,
            "server.key",
            SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return 1;
    }
}

int init_socket(int port)
{
	init_ssl();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0){
		perror("socket");
		exit(1);
	}

	int opt = 1;
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		perror("setsockopt");
		exit(1);	
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if(bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))<0){
		perror("bind");
		exit(1);
	}

	if(listen(server_fd, SOMAXCONN) < 0){
		perror("listen");
		exit(1);
	}

	set_nonblocking(server_fd);

    return server_fd;
}