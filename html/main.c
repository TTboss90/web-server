#include <stdio.h>
#include <stdlib.h>
#include <header.h>
#include <string.h>
#include "main.h"
#include <dlfcn.h>

char* _handle(struct header* data)
{
    void (*func)(void);

    dlerror(); // clear old error

    func = (void (*)(void))dlsym(RTLD_DEFAULT, "test_func");

    char *err = dlerror();
    if (err) {
        printf("dlsym error: %s\n", err);
    } else {
        func();
    }

    printf("handle called\n");
    printf("%d\n",data->n_post);
    for(int i = 0; i < data->n_post;i++){
        printf("%s : %s\n",data->Post_Data[i].name, data->Post_Data[i].value);
    }

    char* m = "HTTP/1.1 302 Found\r\n"
            "Location: index.html\r\n"
            "\r\n";
    char* r = malloc(strlen(m)+1);
    if (r)
        strcpy(r, m);
    return r;
}