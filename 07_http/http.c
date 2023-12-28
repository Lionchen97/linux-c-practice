// 0.通过DNS得到ip
// 1.建立tcp连接 （3次握手）
// 2.在tcp连接，socket的基础上，发送http协议request
// 3.服务器在tcp连接socket，返回http协议response
// www.baidu.com --> 翻译为ip地址 DNS
// tcp 连接这个ip地址:端口
// 发送http协议
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<time.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<netdb.h>
#define HTTP_VERSION    "HTTP/1.1"
#define CONNECTION_TYPE "Connection: close\r\n"
#define BUFFER_SIZE     4096
// DNS
// 系统API  : baidu --> struct hostent --> ip
char *host_to_ip(const char *hostname){
    // 0x3FD384F09 --> "14.215.177.39"
    struct hostent *host_entry = gethostbyname(hostname); // dns
    if(host_entry){
        // unsigned int --> char *
        char *ip = inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
        return ip;
    }
    return NULL;
}

// 创建连接服务器的tcp
int http_create_socket(const char *ip){
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    // 定义服务器
    struct sockaddr_in sin ={0}; 
    sin.sin_family=AF_INET;
    sin.sin_port = htons(80);
    // char* --> unsigned int
    sin.sin_addr.s_addr = inet_addr(ip);

    // 连接
    if(0!=connect(sockfd,(struct sockaddr*)&sin,sizeof(struct sockaddr_in))) return -1;
    // 阻塞I/O 会一直read()直到有数据
    // 非阻塞I/O
    // 对参数sockfd指定的socket进行操作;
    // 操作类型是设置文件状态标志;
    // 要设置的文件状态标志是非阻塞模式。
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
    return sockfd;
}

// 发送请求
char * http_send_request(const char *hostname,const char* resource){
    char *ip = host_to_ip(hostname); // DNS获得ip地址
    int sockfd = http_create_socket(ip); // 建立连接
    printf("%s,%d\n",ip,sockfd);
    char buffer[BUFFER_SIZE] = {0};
    sprintf(buffer,
"GET %s %s\r\n\
Host: %s\r\n\
%s\r\n\
\r\n",resource,HTTP_VERSION,hostname,CONNECTION_TYPE);
    int res = send(sockfd,buffer,strlen(buffer),0);
    printf("send: %d\n",res);

    // recv 非阻塞 可能会收到空
    // select 监听网络I/O 是否有可读数据
    fd_set fdread;
    FD_ZERO(&fdread); // 滞空
    FD_SET(sockfd,&fdread); // 监听状态
    struct timeval tv;
    tv.tv_sec = 5; // 5秒轮询一次
    tv.tv_usec = 0; 
    char *result = malloc(sizeof(int)); 
    memset(result,0,sizeof(int));
    while(1){ // 可能多次recv
        // 第一个参数：最大fd
        // 判断可读fd
        // 判断可写fd
        // 判断出错fd
        // 时间
        int selection = select(sockfd+1,&fdread,NULL,NULL,&tv);
        printf("selection :%d\n",selection);
        if(!selection || !FD_ISSET(sockfd,&fdread)){
            break;
        }else{
            memset(buffer,0,BUFFER_SIZE);
            int len = recv(sockfd,buffer,BUFFER_SIZE,0);
            printf("len: %d\n",len);
            if(len ==0){ // disconnect
                break;
            }
            result = realloc(result,(strlen(result)+len+1)*sizeof(char));
            strncat(result,buffer,len);
        }
    }
    return result;
}
int main(int argc,char *argv[]){
    // ./http www.baidu.com / 请求首页
    if(argc <3) return -1;
     char *response = http_send_request(argv[1],argv[2]);
     printf("response: %s\n",response);

     free(response);
     return 0;
}