#include "uthread/uruntime.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct tagTestArgs
{
    uruntime * runtime;
    int id;
    int seq;
} testargs_t;

void tfunc(void * args) 
{
    testargs_t * testarg = (testargs_t *)args;

    printf("[p:%zd], %d-%d start!\n", pthread_self(), testarg->id, testarg->seq);
    for (int i = 0; i < 10; i++) {
        printf("[p:%zd], %d-%d-%d running..\n", pthread_self(), testarg->id, testarg->seq, i);
        testarg->runtime->yield();
    }
    
    printf("[p:%zd], %d-%d finish!\n", pthread_self(), testarg->id, testarg->seq);
}

void test(uruntime & runtime, int thread_count)
{
    testargs_t * args = (testargs_t *)malloc(thread_count * sizeof(testargs_t));
    
    for (int i = 0; i < thread_count; i++) {
        testargs_t * arg = args + i;

        arg->seq      = i;
        arg->runtime = &runtime;
        arg->id       = runtime.new_thread(tfunc, arg);
        printf("[p:%zd], create id:%d\n", pthread_self(), arg->id);
    }

    while (!runtime.finished()) {
        int seq = random() % thread_count;
        runtime.resume( args[seq].id );
    }
    
    free(args);
}

void * pfun(void * args)
{
    printf("pthread pid:%zd start...\n", pthread_self());
    #if 1
    uruntime runtime;
    test(runtime, 2);
    #else
    for (int i = 0; i < 1000; i++) 
        printf("[p:%d], --%d\n", pthread_self(), i);
    #endif
    printf("pthread pid:%zd finish...\n", pthread_self());

    return NULL;
}

int main(int argc, char * argv[])
{
    uruntime runtime;

    pthread_t p1;
    pthread_t p2;

    pthread_create(&p1, NULL, pfun, NULL);
    pthread_create(&p2, NULL, pfun, NULL);
    
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);

    return 0;
}
