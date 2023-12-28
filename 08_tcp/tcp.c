// TCP服务器
// 并发服务器，一请求一线程，IO多路复用，epoll
// send/recv sockfd --> 五元组（远程IP，远程端口，本机IP，本机端口，协议）
// 一个端口1s可以加入1500个连接
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<netinet/in.h>
#include<pthread.h>
#include<errno.h>
#include<unistd.h>
#include<time.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<netdb.h>
#include<sys/epoll.h>
#define BUFFER_SIZE 1024
#define EPOLL_SIZE  1024
#define MAX_PORT    100
// 多线程的回调函数
void *client_routine(void *arg){
    int clientfd = *(int *)arg;
    while(1){
        char buffer[BUFFER_SIZE]={0};
        // 多线程可以实现阻塞IO，阻塞IO的话，会recv会挂起等待直到len>0
        int len = recv(clientfd,buffer,BUFFER_SIZE,0);
        if(len<0){
            // if(errno == EAGIN || errno == EWOULDBLOCK ){
            //     // 阻塞IO不可能发生以上情况。只有非阻塞IO才可能发生。
            // }
            close(clientfd);
            break;
        }else if(len ==0){ // 客户端断开了链接
            close(clientfd);
            break;
        }else{
            printf("recv: %s, %d byte(s)\n",buffer,len);
        }
    }
}

// 判断是不是监听sockfd
int islistenfd(int fd,int *fds){
    int i=0;
    for(i=0;i<MAX_PORT;i++){
        if(fd == *(fds+i)) return fd;
    }
    return 0;
}
int main(int argc,char *argv[]){
    // tcp 8888
    // 启动netassist
    if (argc<2){
        printf("Param Error\n");
        return -1;
    }
    int port = atoi(argv[1]); // 将字符串转为整数
    int i=0;
    int sockfds[MAX_PORT]={0}; // 添加监听sockfd到集合中
    int epfd = epoll_create(1); // 创建epoll参数大于0即可
    for(i=0;i<MAX_PORT;i++){ // 创建100个监听端口
        int sockfd = socket(AF_INET,SOCK_STREAM,0);//迎宾sockfd
        struct sockaddr_in addr ={0};
        //memset(&addr,0,sizeof(struct sock_addr in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port+i); // 8888 8889 8890 ... 8987
        addr.sin_addr.s_addr = INADDR_ANY; // 绑定在所有网卡上
        if(bind(sockfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))<0){
            perror("bind");
            return -2;
        }
        if(listen(sockfd,5)<0){
            perror("listen");
            return -3;
        } // 最多带5个人
        printf("tcp server listen on port : %d\n",port+i);
        struct epoll_event ev; // 当前事件
        ev.events = EPOLLIN; // 监听读事件
        ev.data.fd = sockfd;
        epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev); // 添加listen socket 到epoll
        sockfds[i]=sockfd;
    }
#if 0  // 一请求一线程 
       // 问题1；如果客户端太多，不能用一请求一线程。假设一个线程8m 1G内存 --> 128 个线程。解决：epoll
       // 问题2：如何区分客户端 无法通过tcp 只能通过应用层 
    while (1){
        struct sockaddr_in client_addr={0};
        socklen_t client_len = sizeof(struct sockaddr_in);
        // 连接一个客户端
        int clientfd = accept(sockfd,(struct sockaddr *)&client_addr,&client_len);
        // 对每一个客户端使用一个线程
        pthread_t thread_id;
        pthread_create(&thread_id,NULL,client_routine,&clientfd);
       
      
    }
#endif
    // epoll 管理监听所有clientfd 相当于小区快递员,必定有一个主事件循环
    // 问题0： 面对大量连接断开如何解决CPU增高的问题
    // 问题1：connection failed 一个进程最多文件fd个数1024 修改 sudo vim /etc/security/limits.conf 
                                                      // 在end of file 上加入 
                                                      //*        hard nofile 1048576
                                                      //*        soft nofile 1048576
                                                      //sudo reboot                       
                                                // ulimit -a 查看个数
                                                // ulimit -n 1048576 修改个数（临时）
    // 问题2:cannot assign requested address 端口一共65535个客户端send的时候端口号被耗尽  使服务器的开放多个端口进行监听
    // 问题3：connect:Connection timed out  64999 --> 65535
    // fd的最大值查看        cat /proc/sys/fs/file-max 1048576
    // nf_conntrack_max 查看 cat /proc/sys/net/netfilter/nf_conntrack_max 65536
    // 客户端修改 vim /etc/svsctl.conf 
    //           fs.file-max = 1048576
    //           net.nf_conntrack_max =1048576
    // 生效：sudo sysctl -p
    // 问题5：too many open files in system  服务端修改同上
    // 问题6：no such file or directory 加载：sudo modprobe ip_conntrack
    // 问题7：内存回收，cpu爆炸 cpu、内存都不能超过80% 
    // 调优 tcp的协议栈
    // sockfd --> 2k *100w -> 2G
    // net.ipv4.tcp_mem 252144 524288 786432 tcp 协议栈总大小 单位页，每一页4k 随意分配1G随意分配2G内存优化3G禁止分配
    // net.ipv4.tcp_wmem 1024 1024 2048 每一个socket发送空间 中间的是缺省值
    // net.ipv4.tcp_rmem 1024 1024 2049 每一个socket接收空间
    // 问题8：单台千万级 用户态协议栈
    struct epoll_event events[EPOLL_SIZE] = {0}; // 事件列表
    
    while(1){
        int nready = epoll_wait(epfd,events,EPOLL_SIZE,-1); //-1表示阻塞，如果是>0的数就是非阻塞io，直到带回事件 多久去一次小区,返回事件数
        if(nready==-1)continue;
        int i =0;
        for(i =0;i<nready;i++){
            int sockfd = islistenfd(events[i].data.fd,sockfds);
            if(sockfd){// 是listen监听socket 表示有新连接
                struct sockaddr_in client_addr={0};
                socklen_t client_len = sizeof(struct sockaddr_in);
                // 连接一个客户端，并添加到epoll(长连接)
                int clientfd = accept(sockfd,(struct sockaddr *)&client_addr,&client_len);
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET; // 有数据，水平触发（多次，持续）| 检测sockfd状态从无到有，边沿触发（一次性，高性能）
                ev.data.fd = clientfd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ev);
            }else{ // 已连接的clientfd有事件
                int clientfd = events[i].data.fd;
                char buffer[BUFFER_SIZE]={0};
                // 多线程可以实现阻塞IO，阻塞IO的话，会recv会挂起等待直到len>0
                int len = recv(clientfd,buffer,BUFFER_SIZE,0);
                if(len<0){
                    // if(errno == EAGIN || errno == EWOULDBLOCK ){
                    //     // 阻塞IO不可能发生以上情况。只有非阻塞IO才可能发生。
                    // }
                    close(clientfd);
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,clientfd,&ev);
                }else if(len ==0){ // 客户端断开了链接
                    close(clientfd);
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,clientfd,&ev);
                }else{
                    printf("recv: %s, %d byte(s),clientfd: %d\n",buffer,len,clientfd);
                }
            }
        }
    }
    return 0;
}