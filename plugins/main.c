#include <stdio.h>

int _pinit(){
    printf("main plugin init\n");
    return 0;
}

void test_func()
{
    printf("test\n");
}

int _pclose(){
    printf("main plugin close\n");
    return 0;
}