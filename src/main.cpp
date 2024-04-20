#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "threadpool.h"

void taskFunc(void * arg)
{
    int* num = (int*)arg;
    printf("thread:%ld,num:%d.\n", pthread_self(), *num);
}

int main()
{
    struct Threadpool* pool = threadpoolCreate(2, 8, 10);
    for(int i = 0; i < 100; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        threadpoolAdd(pool, &taskFunc, num);
    }
    sleep(20);
    threadpoolDestroy(pool);
    return 0;
}
