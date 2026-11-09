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
#include "settings.h"

struct settings_t settings = {0};

void init_settings()
{
    FILE* file = fopen("settings.txt", "r");
    if (file == NULL) {
        perror("settings.txt: ");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* filebuff = malloc(file_size + 1);
    if (!filebuff) {
        fclose(file);
        exit(1);
    }

    size_t bytes_read = fread(filebuff, 1, (size_t)file_size, file);
    filebuff[file_size] = '\0';

    struct item items[100];
    int count = parse(filebuff,&items,';',100);

    for (int i = 0; i < count; i++) {
        if (strcmp(items[i].name, "port") == 0) {
            settings.port = atoi(items[i].value);
        }else if (strcmp(items[i].name, "url") == 0) {
            strcpy(settings.url, items[i].value);
        }
    }

    free(filebuff);
    fclose(file);
}