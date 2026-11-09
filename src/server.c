#define header_funcs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"
#include "server.h"
#include "response.h"
#include "header.h"
#include <dlfcn.h>
#include <dirent.h>
#include <limits.h> 
#include <signal.h>
#include "plugins.h"
#include "settings.h"

#define MAX_EVENTS 64

typedef struct
{
    int fd;
    SSL *ssl;
    int handshake_done;
    int response_sent;
} client_t;

struct epoll_event events[MAX_EVENTS];
struct epoll_event ev;
int server_fd;
int epfd;
int shutting_down = 0;

void init_server()
{
	init_settings();
    server_fd = init_socket(settings.port);

	epfd = epoll_create1(0);
	if(epfd < 0){
		perror("epoll_create1");
		exit(1);
	}

	ev.events = EPOLLIN;
	ev.data.fd = NULL;

	epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

	atexit(close_server);
	signal(SIGINT, close_server);
    signal(SIGTERM, close_server);

	load_plugins();
	
	printf("serving on: %d\n", settings.port);
}

static inline void close_client(int epfd, client_t *client)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, client->fd, NULL);

    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);

    close(client->fd);

    free(client);
}

void handle_client(struct epoll_event* ev)
{
	int fd = ev->data.fd;
	client_t* client = ev->data.ptr;

	if (!client->handshake_done){
		int r = SSL_accept(client->ssl);


		if (r == 1){
			client->handshake_done = 1;
		}
		else
		{
			int err = SSL_get_error(client->ssl, r);


			if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE){
				char response[2048];
				
				int len = snprintf(response, sizeof(response),
					"HTTP/1.1 302 Found\r\n"
					"Location: https://%s\r\n"
					"Content-Length: 0\r\n"
					"Connection: close\r\n"
					"\r\n",
					settings.url
				);

				send(client->fd, response, len, 0);
				close_client(epfd, client);
			}
		}

		return;
	}

	size_t buffer_size = 4096;
	char* buffer = malloc(buffer_size);
	int total = 0;
	if (!buffer) {
		perror("malloc");
		close_client(epfd, client);
		return;
	}

	while (1) {
		if(buffer_size > 16384){
			char* m = http_err(413, "Payload Too Large");
			SSL_write(client->ssl, m, sizeof(m));
			free(buffer);
			free(m);
			close_client(epfd, client);
			break;
		}
		if (buffer_size - total <= 1) {
			buffer_size *= 2;
			char *tmp = realloc(buffer, buffer_size);
			if (!tmp) {
				free(buffer);
				close_client(epfd, client);
				break;
			}
			buffer = tmp;
		}

		ssize_t bytes = SSL_read(client->ssl, buffer + total, buffer_size - total - 1);

		if (bytes > 0) {
			total += bytes;
			buffer[total] = '\0';

			if (strstr(buffer, "\r\n\r\n")) {
				break;
			}
		}
		else if (bytes == 0) {
			close(fd);
			return;
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				free(buffer);
				close_client(epfd, client);
				return;
			}

			perror("read");
			free(buffer);
			close_client(epfd, client);
			return;
		}
	}

	struct header header_data = get_header_data(buffer, total);
	if(header_data.Method == 0)
	{
		char* m = http_err(400, "Bad Request");
		free(buffer);
		SSL_write(client->ssl, m, strlen(m));
		free(m);
		close_client(epfd, client);
		return;
	}
	//print_header(&header_data);

	const char* response = (char*)build_response(buffer, &header_data);
	if(response == NULL)
	{
		char* m = http_err(500, "Internal Server Error");
		free(buffer);
		SSL_write(client->ssl, m, strlen(m));
		free(m);
		close_client(epfd, client);
		return;
	}
	//printf("response:\n%s", response);

	size_t len = strlen(response);
	size_t sent = 0;

	while (sent < len) {
		ssize_t n = SSL_write(client->ssl, response + sent, len - sent);

		if (n > 0)
			sent += n;
		else{
			free(buffer);
			free((void*)response);
			close_client(epfd, client);
			continue;
		}
	}
	
	free(header_data.Cookies);
	free(header_data.Post_Data);
	free(header_data.Get_Data);
	free(buffer);
	free((void*)response);
	close_client(epfd, client);
}

void run_server()
{
    while(1){
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

		for(int i = 0; i < n; i++){
			if(events[i].data.fd == NULL){
				while (1) {
					struct sockaddr_in addr;
					socklen_t len = sizeof(addr);

					int client_fd = accept(server_fd,(struct sockaddr *)&addr,&len);

					if (client_fd < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK)
							break;
						perror("accept");
						break;
					}

					set_nonblocking(client_fd);

					client_t* client = calloc(1,sizeof(client_t));

					client->fd = client_fd;
					client->ssl = SSL_new(ctx);

					SSL_set_fd(client->ssl, client_fd);

                    SSL_set_accept_state(client->ssl);
					
					struct epoll_event cev;
					cev.events = EPOLLIN;
					cev.data.ptr = client;

					epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
	        	}	
			}else{
				handle_client(&events[i]);
			}
		}
	}
}

void close_server()
{
	if (shutting_down)
        return;

    shutting_down = 1;

	printf("closing server\n");

	close_plugins();
	
    close(server_fd);
	close(epfd);

	exit(0);
}
