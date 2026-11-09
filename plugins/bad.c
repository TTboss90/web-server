#include <stdio.h>

int _pinit(){
    printf("bad plugin init\n");
    return 0;
}

void bad_func()
{
    exit(1);
}

int _pclose(){
    printf("bad plugin close\n");
    return 0;
}