// TODO如何实现异步dns请求
// 1.建立tcp连接（可选）
// 2.建立udp
// 3.发送dns请求，接收dns请求
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<time.h>
#include<arpa/inet.h>
#define DNS_SERVER_PORT 53
#define DNS_SERVER_IP "114.114.114.114"
#define DNS_CNAME 0x05
#define DNS_HOST  0x01
struct dns_header{
    // 每一项对应16位，4个16进制数，也就是2个字节
    unsigned short id; // 请求和返回的id一致 
    unsigned short flags;
    unsigned short num_questions;
    unsigned short answer_RRs;
    unsigned short authority_RRs;
    unsigned short additional_RRs;
};
struct dns_queries{
    int length;
    unsigned char *qname; // 查询名，长度不固定，且不使用填充字节
    unsigned short qtype;
    unsigned short qclass;
};
struct dns_item{
    char *domain;
    char *ip;
};

int dns_create_header(struct dns_header *header){
    if(header ==NULL) return -1;
    memset(header,0,sizeof(struct dns_header));

    //random 非线程安全
    srandom(time(NULL)); // 设置随机数种子
    header->id = random(); // 随机生成id

    header->flags = htons(0x0100); // 主机字节序 --> 网络字节序 表示期望递归
    header->num_questions=htons(1); // 请求数量为1
    return 0;
}
int dns_create_queries(struct dns_queries *queries,const char *hostname){
    if(queries==NULL || hostname==NULL) return -1;
    memset(queries,0,sizeof(struct dns_queries));
    queries->qname = (char *)malloc(strlen(hostname)+2); // 结尾补0+总长度
    if(queries->qname==NULL) return -2;
    queries->length = strlen(hostname)+2;
    queries->qtype =htons(1); // 助记符A,表示查询
    queries->qclass =htons(1); // 表示Internet

    // hostname:www.baidu.com --> qname 3w5baidu3com
    const char delim[2]=".";
    char *name = queries->qname; // 为了方便操作，定义一个指针
    char *hostname_dup = strdup(hostname); //strdup --> malloc 复制一份获得可修改版本
    char *token = strtok(hostname_dup,delim); // 'www'
    while (token!=NULL){
        size_t len = strlen(token); //3
        *name = len;
        name++;
        // strcpy() 遇到'\0'结束
        strncpy(name,token,len+1);  // 指定长度 name = '3w\0'
        name +=len; // 指向'\0'
        token = strtok(NULL,delim); // 'baidu'，非线程安全，依赖上一次的结果 
    }
    free(hostname_dup);
}
// 合并header和require,得到一个完整的请求
int dns_build_request(struct dns_header *header,struct dns_queries *queries,char *request,int rlen){
    if (header==NULL||queries==NULL||request==NULL) return -1;
    memset(request,0,rlen);
    // header --> request
    memcpy(request,header,sizeof(struct dns_header));
    int offset = sizeof(struct dns_header);
    // queries --> request
    memcpy(request+offset,queries->qname,queries->length);
    offset += queries->length;
    memcpy(request+offset,&queries->qtype,sizeof(queries->qtype));
    offset += sizeof(queries->qtype);
    memcpy(request+offset,&queries->qclass,sizeof(queries->qclass));
    offset +=sizeof(queries->qclass);
    return offset;
}

// dns response解析
static int is_pointer(int in){
    return ((in & 0xC0)==0xC0);
}
static void dns_parse_name(unsigned char *chunk, unsigned char *ptr, char *out, int *len) {
    int flag = 0, n = 0, alen = 0;
    char *pos = out + (*len);
    while (1) {
        flag = (int)ptr[0];
        if (flag == 0) break;
        if (is_pointer(flag)) {
            n = (int)ptr[1];
            ptr = chunk + n;
            dns_parse_name(chunk, ptr, out, len);
            break;
        } else {
            ptr ++;
            memcpy(pos, ptr, flag);
            pos += flag;
            ptr += flag;
            *len += flag;
            if ((int)ptr[0] != 0) {
                memcpy(pos, ".", 1);
                pos += 1;
                (*len) += 1;
            }
        }
    }
}
static int dns_parse_response(char *buffer, struct dns_item **domains){
    int i=0;
    unsigned char *ptr = buffer;
    ptr += 4;
    int querys = ntohs(*(unsigned short*)ptr);
    ptr += 2;
    int answers = ntohs(*(unsigned short*)ptr);
    ptr += 6;
    for (i = 0;i < querys;i ++) {
        while (1) {
            int flag = (int)ptr[0];
            ptr += (flag + 1);
            if (flag == 0) break;
        }
        ptr += 4;
    }
    char cname[128], aname[128], ip[20], netip[4];
    int len, type, ttl, datalen;
    int cnt = 0;
    struct dns_item *list = (struct dns_item*)calloc(answers, sizeof(struct dns_item));
    if (list == NULL) {
        return -1;
    }
    for (i = 0;i < answers;i ++) {
        bzero(aname, sizeof(aname));
        len = 0;
        dns_parse_name(buffer, ptr, aname, &len);
        ptr += 2;
        type = htons(*(unsigned short*)ptr);
        ptr += 4;
        ttl = htons(*(unsigned short*)ptr);
        ptr += 4;

        datalen = ntohs(*(unsigned short*)ptr);
        ptr += 2;
        if (type == DNS_CNAME) {
            bzero(cname, sizeof(cname));
            len = 0;
            dns_parse_name(buffer, ptr, cname, &len);
            ptr += datalen;
        } else if (type == DNS_HOST) {
            bzero(ip, sizeof(ip));
            if (datalen == 4) {
                memcpy(netip, ptr, datalen);
                inet_ntop(AF_INET , netip , ip , sizeof(struct sockaddr));

            printf("%s has address %s\n" , aname, ip);
            printf("\tTime to live: %d minutes , %d seconds\n", ttl / 60, ttl % 60);
            list[cnt].domain = (char *)calloc(strlen(aname) + 1, 1);
            memcpy(list[cnt].domain, aname, strlen(aname));
            list[cnt].ip = (char *)calloc(strlen(ip) + 1, 1);
            memcpy(list[cnt].ip, ip, strlen(ip));
            cnt ++;
            }
            ptr += datalen;
        }
    }
    *domains = list;
    ptr += 2;
    return cnt;
}
// udp编程,提交请求，并且接收回复
int dns_client_commit(const char *domain)
{
    // AF_INET表示该socket用于IPv4网络通信。
    // SOCK_DGRAM表示这个是一个UDP socket,而不是TCP socket。
    // 第三个参数一般设置为0,让系统选择合适的网络协议。
    int sockfd = socket(AF_INET,SOCK_DGRAM,0); 
    if(sockfd <0) return -1;
    struct sockaddr_in serveraddr ={0};
    serveraddr.sin_family = AF_INET; // IPV4地址簇
    serveraddr.sin_port = htons(DNS_SERVER_PORT); // 端口号
    serveraddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP); // DNS服务器IP地址
  
    // 可选connect （两次握手）
    // udp中connect()的作用:为sendto开辟一条网路，防止sendto失败，保证sendto的成功。
    int ret = connect(sockfd,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));
    printf("connect: %d\n",ret);
    struct dns_header header = {0};
    dns_create_header(&header);
    struct dns_queries require={0};
    dns_create_queries(&require,domain);

    char request[1024]={0}; // 请求
    int length = dns_build_request(&header,&require,request,sizeof(request));
    
    //request
    int slen = sendto(sockfd,request,length,0,(struct sockaddr*)&serveraddr,(socklen_t)sizeof(struct sockaddr_in));
    printf("sendto: %d\n",slen);

    //receive 同步的过程，等待request
    char response[2048]={0}; // 回复
    struct sockaddr_in addr;
    size_t addr_len = sizeof(struct sockaddr_in);
    int rcvlen =recvfrom(sockfd,response,sizeof(response),0,(struct sockaddr*)&addr,(socklen_t*)&addr_len);
    
    printf("recvfrom : %d\n",rcvlen);
    struct dns_item *dns_domain = NULL;
    dns_parse_response(response,&dns_domain);
    free(dns_domain);
    return rcvlen;
}

// udp传输速度快（下载），响应速度快（游戏）
int main(int argc,char *argv[]){
    if(argc<2) return -1;
    dns_client_commit(argv[1]);
    return 1;
}