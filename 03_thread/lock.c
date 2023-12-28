#include<stdio.h>
#include<pthread.h>
// 自旋锁适用于锁的内容较少，线程切换的代价远大于等待的代价的情况下。
// 互斥锁适用于锁的内容比较多，如线程安全的红黑树插入
// 原子操作 将多个汇编指令的操作改写为一个原子操作函数，一个线程必须全部执行完毕，第二个线程才能继续执行
// CAS 问题
#define THREAD_COUNT 10
pthread_mutex_t mutex; //互斥锁（引起线程切换，等待）
pthread_spinlock_t spinlock; //自旋锁(相当于while(1)循环)

int inc(int *value,int add){
    int old;
    __asm__ volatile(                 
// 1.防止编译器优化汇编代码
// 编译器可能会对汇编代码进行优化,改变执行顺序等。加volatile可以避免这种优化,确保汇编顺序和结果不变。
// 2.强制每次读取内存
// volatile告诉编译器内存操作数每次都需要读/写内存,不使用寄存器缓存的值。

// 3.强制禁止并行执行
// volatile通知编译器对应的汇编代码不能并行执行,需要串行化。

// 4.定义内存类型的操作数
// volatile可以把操作数定义为内存类型,比如 volatile int,然后就可以用于内存操作数。
        "lock; xaddl %2, %1;"          //原子执行 第二个值+第一个值 赋值给 第一个值
        : "=a" (old)                  //选择eax寄存器
        : "m" (*value), "a" (add)   //目标操作数 源操作数
        : "cc", "memory"              //"cc" 表示这个汇编代码可能会修改状态标志寄存器FLAGS。"memory" 表示可能会修改内存中的内容。
    );
    return old;
}

void *thread_callback(void *arg){
    int *pcount = (int *)arg;
    int i=0;
    while (i++<1000)
    {
#if 0
        (*pcount)++;
#else
        // pthread_mutex_lock(&mutex);
        // (*pcount)++; //临界资源
        // pthread_mutex_unlock(&mutex);

        // pthread_spin_lock(&spinlock);
        // (*pcount)++;
        // pthread_spin_unlock(&spinlock);
        inc(pcount,1);                  // mov [*pcount],eax;
                                        // inc eax;               ---->  xaddl 1,[*pcount]
                                        // mov eax,[*pcount];


#endif
        usleep(1);
    }
    
}

int main(){
    pthread_t threadid[THREAD_COUNT] = {0};
    pthread_mutex_init(&mutex,NULL);
    pthread_spin_init(&spinlock,PTHREAD_PROCESS_SHARED);
    int i=0;
    int count = 0;
    for(i=0;i<THREAD_COUNT;i++){
        pthread_create(&threadid[i],NULL,thread_callback, &count);
    }
    for(i=0;i<100;i++){
        printf("count: %d\n",count);
        sleep(1);
    }
    return 0;
}
