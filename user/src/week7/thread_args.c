#include "ulib.h"

int thread_func(void *args){
    char **argv = (char **)args;
    for(int i = 0; argv[i]; i++){
        printf("%s\n", argv[i], argv[i]);
    }
    return 0;
}

int main(){
    char *argv[] = {
        "Hello, world!",
        "This is a test for thread with args.",
        "OK, thread_args is passed.",
        NULL
    };

    void *stack = malloc(4096);
    // printf("%p-%x\n", &argv[1], argv[0]);
    // printf("func %p\n", thread_func);
    clone(thread_func, stack + 4096, (void *)argv);

    thread_exit(0);
}