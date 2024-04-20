#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "threadpool.h"

int  Number = 2;

struct Task
{
    void (*function)(void* arg);
    void* arg;
};

struct Threadpool
{
    struct Task* taskQ;
    int queueCapacity;
    int queueSize;
    int queueFront;
    int queueRear;

    pthread_t managerID;
    pthread_t* threadIDs;
    int minNum;
    int maxNum;
    int liveNum;
    int busyNum;
    int exitNum;

    pthread_mutex_t mutexPool;
    pthread_cond_t notEmpty;
    pthread_cond_t notFull;
    pthread_mutex_t mutexBusy;
    
    int shutdown;
};

struct Threadpool* threadpoolCreate(int min, int max, int queueCapacity)
{
    printf("create threadpool.\n");
    struct Threadpool* pool = (struct Threadpool*)malloc(sizeof(struct Threadpool));
    do
    {
        if(pool == NULL)
        {
            printf("malloc threadpool fail.\n");
            break;
        }
        
        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if(pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail.\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pool->threadIDs));

        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;
        
        if( pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0)
        {
            break;
        }

        pool->taskQ = (struct Task*)malloc(sizeof(struct Task) * queueCapacity);
        if(pool->taskQ == NULL)
        {
            printf("malloc task fail.\n");
            break;
        }
        pool->queueCapacity = queueCapacity;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutdown = 0;

        pthread_create(&pool->managerID, NULL, manager, pool);
        for(int i = 0; i < pool->liveNum; i++)
        {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
        }
        
        return pool;
    } while (0);
    
    if(pool && pool->threadIDs)
    {
        free(pool->threadIDs);
    }
    if(pool && pool->taskQ)
    {
        free(pool->taskQ);
    }
    if(pool)
    {
        free(pool);
    }
    return NULL;
}

int threadpoolDestroy(struct Threadpool* pool)
{
    if(pool == NULL)
    {
        return 0;
    }

    pool->shutdown = 1;
    pthread_join(pool->managerID, NULL);
    for(int i = 0; i < pool->liveNum; i++)
    {
        pthread_cond_signal(&pool->notEmpty);
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    pthread_mutex_destroy(&pool->mutexBusy);

    if(pool->threadIDs)
    {
        free(pool->threadIDs);
    }
    if(pool->taskQ)
    {
        free(pool->taskQ);
    }
    if(pool)
    {
        free(pool);
    }

    return 0;
}

void threadpoolAdd(struct Threadpool* pool, void (*function)(void* arg), void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);

    while(pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }

    if(pool->shutdown)
    {
        printf("shutdown manger thread.\n");
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    pool->taskQ[pool->queueRear].function = function;
    pool->taskQ[pool->queueRear].arg = arg;

    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    pthread_mutex_unlock(&pool->mutexPool);

    pthread_cond_signal(&pool->notEmpty);
}

int threadpoolbusyNum(struct Threadpool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return busyNum;
}

int threadpoolliveNum(struct Threadpool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

void* worker(void* arg)
{
    printf("start worker thread.\n");
    struct Threadpool* pool = (struct Threadpool*)arg;
    
    while(1)
    {
        pthread_mutex_lock(&pool->mutexPool);

        while(pool->queueSize == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
        }

        if(pool->shutdown)
        {
            printf("shutdown worker thread.\n");
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }

        struct Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;

        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;

        pthread_mutex_unlock(&pool->mutexPool);

        pthread_cond_signal(&pool->notFull);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        (*task.function)(task.arg);
        free(task.arg);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
}

void* manager(void* arg)
{
    printf("start manager thread.\n");
    struct Threadpool* pool = (struct Threadpool*)arg;

    while(!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //创建线程
        //任务个数 > 存活线程数 且 存活线程数 < 最大线程数
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for(int i = 0; i < pool->maxNum && counter < Number && pool->liveNum < pool->maxNum; i++)
            {
                if(pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, &worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //销毁线程
        //任务个数*2 < 存活线程数 且 存活线程数 > 最小线程数
        if(queueSize * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = Number;
            pthread_mutex_unlock(&pool->mutexPool);
            for(int i = 0; i < Number; i++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }

    printf("shutdown manager thread.\n");
    pthread_exit(NULL);
}

void threadExit(struct Threadpool* pool)
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < pool->maxNum; i++)
    {
        if(pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}