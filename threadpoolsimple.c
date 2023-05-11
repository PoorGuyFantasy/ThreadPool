#include"threadpoolsimple.h"

ThreadPool *thrPool=NULL;

int biginnum=1000;  //任务的编号

//线程工作函数
void *thrRun(void * arg)
{
    ThreadPool *pool=(ThreadPool*)arg;  //传进来的是线程
    int taskpos=0;  //任务位置
    PoolTask* task=(PoolTask* )malloc(sizeof(PoolTask));    //创建一个任务，以一会从任务队列上面取任务

    while(1)
    {
        //获取任务，首先需要尝试加锁
        pthread_mutex_lock(&thrPool->pool_lock);

        //无任务，并且线程池仍然在使用，则阻塞在此
        while(thrPool->job_num<=0 && !thrPool->shutdown)
            pthread_cond_wait(&thrPool->not_empty_task,&thrPool->pool_lock);
        
        if(thrPool->job_num)
        {
            //有任务需要处理
            taskpos=(thrPool->job_pop++)%(thrPool->max_job_num);
            
            //printf("task out %d...tasknum==%d  tid=%lu\n",taskpos,
                    //thrPool->tasks[taskpos].tasknum,pthread_self());

            //避免任务呗修改，因为生产者会添加任务，所以需要将需要处理的任务拷贝过来
            memcpy(task,&thrPool->tasks[taskpos],sizeof(PoolTask));
            task->arg=task;
            thrPool->job_num--;
            pthread_cond_signal(&thrPool->empty_task);  //任务取出一个后，通知生产者可以开始生产任务
            
        }
        if(thrPool->shutdown)
        {
            //线程池被摧毁，则应该退出线程
            pthread_mutex_unlock(&thrPool->pool_lock);
            free(task);
            pthread_exit(NULL);
        }

        //释放锁
        pthread_mutex_unlock(&thrPool->pool_lock);
        task->task_func(task->arg); //执行回调函数
    }

}


//创建线程池
void create_threadpool(int thrnum,int maxtasknum)
{
    printf("begin call -------\n");
    
    thrPool=(_ThreadPool*)malloc(sizeof(ThreadPool));

    thrPool->thr_num=thrnum;
    thrPool->max_job_num=maxtasknum;
    thrPool->shutdown=0;    //0代表线程池在使用，1代表关闭
    thrPool->job_push=0;    //任务队列添加的位置
    thrPool->job_pop=0;     //任务队列出队的位置
    thrPool->job_num=0;     //初始化的任务个数为0

    thrPool->tasks=(PoolTask*)malloc(sizeof(PoolTask)*maxtasknum);   
    //申请一个大小为maxtasknum的任务队列数组

    //初始化锁和条件变量
    pthread_mutex_init(&thrPool->pool_lock,NULL);
    pthread_cond_init(&thrPool->empty_task,NULL);
    pthread_cond_init(&thrPool->not_empty_task,NULL);

    int i=0;
    //申请thrnum个线程id的空间
    thrPool->threads=(pthread_t*)malloc(sizeof(pthread_t)*thrnum);

    //分离线程属性
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    for(i=0;i<thrnum;i++)
        pthread_create(&thrPool->threads[i],&attr,thrRun,(void *)thrPool);  //创建多个线程
    printf("end call ------\n");


}

void addTask(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->pool_lock);

    while(pool->max_job_num<=pool->job_num)
        pthread_cond_wait(&pool->empty_task,&pool->pool_lock);
    int taskpos=(pool->job_push++)%(pool->max_job_num);

    pool->tasks[taskpos].tasknum=biginnum++;
    pool->tasks[taskpos].arg=(void*)&pool->tasks[taskpos];  //拷贝队列上的改任务的地址给任务
    pool->tasks[taskpos].task_func=taskRun;
    pool->job_num++;


    pthread_mutex_unlock(&pool->pool_lock);
    pthread_cond_signal(&pool->not_empty_task); //任务添加结束，发送信号通知进程可以取任务
}

void taskRun(void* arg)
{
    PoolTask *task=(PoolTask*)arg;
    int num=task->tasknum;
    printf("task %d is running,pthread number is %lu\n",num,pthread_self());
    sleep(1);
    printf("task %d is done,pthread number is %lu\n",num,pthread_self());
}

void destroy_threadpool(ThreadPool *pool)
{
    pool->shutdown=1;
    pthread_cond_broadcast(&pool->not_empty_task);

    int i=0;
    for(i=0;i<pool->thr_num;i++)
        pthread_join(pool->threads[i],NULL);
    pthread_cond_destroy(&pool->not_empty_task);
    pthread_cond_destroy(&pool->empty_task);
    pthread_mutex_destroy(&pool->pool_lock);

    free(pool->tasks);
    free(pool->threads);
    free(pool);


}





int main(void)
{
    create_threadpool(3,20);
    int i=0;
    for(i=0;i<50;i++)
        addTask(thrPool);   //往线程池添加任务
    sleep(20);
    destroy_threadpool(thrPool);
    return 0;
}
