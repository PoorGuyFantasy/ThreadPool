#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<pthread.h>

typedef struct _PoolTask    //任务结构体
{
    int tasknum;    //任务的编号
    void *arg;      //回调函数的参数
    void (*task_func)(void* arg);   //任务的回调函数
}PoolTask;

typedef struct _ThreadPool  //线程池结构体
{
    int max_job_num;       //最大任务个数
    int job_num;           //实际任务个数
    PoolTask *tasks;         //任务队列数组
    int job_push;           //入队位置
    int job_pop;            //出队位置

    int thr_num;            //线程池内线程个数
    pthread_t *threads;     //线程池内线程数组
    int shutdown;           //标记是否关闭线程池
    pthread_mutex_t pool_lock;   //线程池的锁
    pthread_cond_t empty_task; //任务队列为空的条件变量
    pthread_cond_t not_empty_task;     //任务队列不为空的条件变量
    
}ThreadPool;

void create_threadpool(int thrnum,int maxtasknum);  //创建线程池----thrnum:代表线程池中线程个数，maxtasknum:代表最大的任务数目
void destroy_threadpool(ThreadPool *pool);          //销毁线程池
void addTask(ThreadPool *pool);                     //添加任务到线程池
void taskRun(void *arg);                            //任务回调函数

#endif
