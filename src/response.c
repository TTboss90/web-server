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

static int is_inside_webroot(const char* target)
{
    char root[PATH_MAX];

    if (!realpath("./html", root))
        return 0;

    char requested[PATH_MAX];
    snprintf(requested, sizeof(requested), "%s%s", root, target);

    char normalized[PATH_MAX];
    char *parts[PATH_MAX / 2];
    int count = 0;

    char temp[PATH_MAX];
    snprintf(temp, sizeof(temp), "%s", requested);

    char *token = strtok(temp, "/");

    while (token) {
        if (strcmp(token, ".") == 0) {
        }
        else if (strcmp(token, "..") == 0) {
            if (count > 0)
                count--;
        }
        else {
            parts[count++] = token;
        }

        token = strtok(NULL, "/");
    }

    normalized[0] = '\0';
    strcat(normalized, "/");

    for (int i = 0; i < count; i++) {
        strcat(normalized, parts[i]);

        if (i != count - 1)
            strcat(normalized, "/");
    }

    size_t len = strlen(root);

    return strncmp(normalized, root, len) == 0 &&
           (normalized[len] == '/' || normalized[len] == '\0');
}

char* http_err(const int code, const char* text)
{
    FILE *fp = fopen("error.html", "rb");
    if (!fp){
        char* h = "HTTP/1.1 %d %s\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "\r\n";

        char* b = "<!DOCTYPE html>"
                    "<html>"
                    "<head><title>%d %s</title></head>"
                    "<body>"
                    "<h1>%d %s</h1>"
                    "<p>The requested resource could not be found.</p>"
                    "</body>"
                    "</html>";
        
        int b_len = snprintf(
            NULL,
            0,
            b,
            code, text,
            code, text
        );

        int h_len = snprintf(
            NULL,
            0,
            h,
            code, text,
            b_len
        );

        char* r = malloc(b_len+h_len+1);
        int w = snprintf(
            r,
            h_len + 1,
            h,
            code, text,
            b_len
        );
        snprintf(
            r + w,
            b_len + 1,
            b,
            code, text,
            code, text
        );
        return r;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    const char *script =
        "<script>"
        "error = %d;"
        "error_text = \"%s\";"
        "</script>";

    int script_len = snprintf(
        NULL,
        0,
        script,
        code,
        text
    );

    const char *header =
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n";

    long body_len = script_len + file_size;

    int header_len = snprintf(
        NULL,
        0,
        header,
        code,
        text,
        body_len
    );

    char *response = malloc(header_len + body_len + 1);
    if (!response) {
        fclose(fp);
        return NULL;
    }

    int written = snprintf(
        response,
        header_len + 1,
        header,
        code,
        text,
        body_len
    );

    int offset = written;

    offset += snprintf(
        response + offset,
        script_len + 1,
        script,
        code,
        text
    );

    fread(
        response + offset,
        1,
        file_size,
        fp
    );

    fclose(fp);

    response[header_len + body_len] = '\0';

    return response;
}

void* build_response(char* buffer, struct header* header)
{ 
    char file_name[PATH_MAX];
    snprintf(file_name, sizeof(file_name), "./html%s", header->Target);
    if (!is_inside_webroot(header->Target)) {
        return http_err(403, "Forbidden");
    }
    size_t len = strlen(file_name);
    
    if(file_name[len-1] == '/'){
        char tmp[PATH_MAX];

        snprintf(tmp, sizeof(tmp), "%sindex.html", file_name);
        snprintf(file_name, sizeof(file_name), "%s", tmp);

        len = strlen(file_name);
    }

    if(len >= 5 && strcmp(file_name + len - 5, ".html") == 0){
        FILE* file = fopen(file_name, "r");
        if (file == NULL) {
            char* m = http_err(404, "No such file or directory");
            return m;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        char* filebuff = malloc(file_size + 1);
        if (!filebuff) {
            fclose(file);
            return NULL;
        }

        fread(filebuff, 1, file_size, file);
        filebuff[file_size] = '\0';

        size_t response_size = file_size + 256;

        char* response = malloc(response_size);

        if (!response) {
            free(filebuff);
            fclose(file);
            return NULL;
        }

        snprintf(
            response,
            response_size,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            file_size,
            filebuff
        );

        free(filebuff);
        fclose(file);

        return response;
    }else if(len >= 3 && (strcmp(file_name + len - 3, ".so") == 0 || strcmp(file_name + len - 5, ".hide") == 0)){
        char* m = http_err(404, "No such file or directory");
        return m;
    }else{
        strncat(file_name, ".so", sizeof(file_name) - strlen(file_name) - 1);

        void *handle = dlopen(file_name, RTLD_LAZY);
        if (!handle) {
            char* m = http_err(404, "No such file or directory");
            return m;
        }
        dlerror();

        char* (*func)(struct header *) = dlsym(handle, "_handle");
        char *err = dlerror();
        if (err) {
            fprintf(stderr, "%s\n", err);
            dlclose(handle);
            return NULL;
        }
        char *response = func(header);

        dlclose(handle);
        return response;
    }
}