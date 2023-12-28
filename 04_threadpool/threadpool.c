#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<string.h>
#define THREADPOOL_INIT_COUNT 20
#define TASK_INIT_SIZE        1000
#define LIST_INSERT(item, list) do{   \
    item->prev = NULL;                \
    item->next = list;                \
    if(list) (list)->prev = item;     \
    list = item;                      \
} while (0)
#define LIST_REMOVE(item,list) do{                       \
    if(item->prev !=NULL) item->prev->next = item->next; \
    if(item->next !=NULL) item->next->prev = item->prev; \
    if(list == item) list = item -> next;                \
    item->prev = item->next = NULL;                      \
}while(0)
// 任务队列
struct nTask
{
    void (*tack_func)(struct nTask *task);
    void *user_data;
    struct nTask *prev;
    struct nTask *next;
};

// 工作队列
struct nWorker{
    pthread_t threadid;
    int terminate;
    struct nManager *manager;
    struct nWorker *prev;
    struct nWorker *next;
};

// 管理队列（线程池）
typedef struct nManager{
    struct nTask *tasks;
    struct nWorker *workers;

    pthread_mutex_t mutex;
    pthread_cond_t cond; //条件变量
}ThreadPool;

// 回调函数每个线程做的事情，找任务
static void *nThreadPoolCallback(void *arg){
    struct nWorker *worker = (struct nWorker*)arg;
    while (1)
    {
        pthread_mutex_lock(&worker->manager->mutex);
        while(worker->manager->tasks==NULL){
            if(worker->terminate) break;
            // 条件等待
            pthread_cond_wait(&worker->manager->cond,&worker->manager->mutex);
        }
        if(worker->terminate){
            pthread_mutex_unlock(&worker->manager->mutex);
            break;
        }
        // 取出任务队列中的首任务
        struct nTask *task = worker->manager->tasks;
        LIST_REMOVE(task,worker->manager->tasks);
        pthread_mutex_unlock(&worker->manager->mutex);
        task->tack_func(task);
    }
    // 线程退出
    free(worker); 
}

// API
int nThreadPoolCreate(ThreadPool *pool,int numWorkers){
    // 初始化除了task以外的所有资源
    if(pool ==NULL) return -1;
    if(numWorkers<1) numWorkers = 1;
    memset(pool,0,sizeof(ThreadPool));
    // 初始化条件变量
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER; // 创建空白条件变量
    memcpy(&pool->cond,&blank_cond,sizeof(pthread_cond_t));
    // 初始化互斥锁 
    // pthread_mutex_init(&pool->mutex,NULL);
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->mutex,&blank_mutex,sizeof(pthread_mutex_t));
    // 初始化工作队列
    int i=0;
    for(i=0;i<numWorkers;i++){
        struct nWorker *worker = (struct nWorker*)malloc(sizeof(struct nWorker));
        if(worker==NULL){
            perror ("malloc");
            return -2;
        }
        // 初始化worker
        memset(worker,0,sizeof(struct nWorker));
        // 分配线程池
        worker->manager =pool;
        // 创建线程
        int ret = pthread_create(&worker->threadid,NULL,nThreadPoolCallback,worker);
        if(ret){ // 创建成功返回0，失败返回非0
            perror("pthread_create");
            free(worker);
            return -3;
        }
        LIST_INSERT(worker,pool->workers);
    }
    // success
    return 0;
}
int nTthreadPoolDestory(ThreadPool *pool,int numwokers){
    struct nWorker *worker = NULL;
    for(worker=pool->workers;worker!=NULL;worker=worker->next){
        worker->terminate=1;
    }
    // 唤醒所有等待条件的线程(同一把锁)
    pthread_mutex_lock(&pool->mutex);
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    pool->workers=NULL;
    pool->tasks=NULL;
    return 0;
}
// API
int nThreadPoolPushTask(ThreadPool *pool,struct nTask *task){
    pthread_mutex_lock(&pool->mutex);
    LIST_INSERT(task,pool->tasks);
    // 唤醒等待条件中的一个线程
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}
void task_entry(struct nTask *task){
    // struct nTask *task = (struct nTask*)task;
    int idx = *(int *)task->user_data;
    printf("idx: %d\n",idx);
    free(task->user_data);
    free(task);
}
int main(){
    ThreadPool pool={0};
    nThreadPoolCreate(&pool,THREADPOOL_INIT_COUNT);
    int i=0;
    for(i=0;i<TASK_INIT_SIZE;i++){
        struct nTask *task = (struct nTask*)malloc(sizeof(struct nTask));
        if(task==NULL){
            perror("malloc");
            exit(1);
        }
        memset(task,0,sizeof(struct nTask));
        task->tack_func =task_entry;
        task->user_data = malloc(sizeof(int));
        *(int*)task->user_data = i;

        nThreadPoolPushTask(&pool,task);
    }
    getchar();
    return 0;
}