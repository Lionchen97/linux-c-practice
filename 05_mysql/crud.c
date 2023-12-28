// Server-Ubuntu
// sudo apt-get install mysql-server-5.7 安装MySQL
// sudo netstat -anp|grep mysql 查看服务器端口号
// ifconfig 查看服务器IP
// sudo vim /etc/mysql/mysql.conf/mysqld.cnf 修改bind_address 0.0.0.0
// mysql -u root -p 登录
// sudo /etc/init.d/mysql restart 重启
// sudo service mysql restart 重启
// sudo apt-get install libmysqlclient-dev
// gcc -o a a.c -I /usr/include/mysql/ -lmysqlclient

// NodeServer-Windows 
// C- R - U - D 工程师
// MYSQL *handle 类似于ns和s之间的管道
// MYSQL_STMT *stmt 声明占位符
#include<stdio.h>
#include<string.h>
#include<mysql/mysql.h>
#define csj_DB_SERVER_IP  "192.168.77.133"
#define csj_DB_SERVER_PORT 3306

#define csj_DB_USERNAME "admin"
#define csj_DB_PASSWORD "123456"

#define csj_DB_DEFAULTDB "csj_DB"

#define SQL_INSERT_TBL_USER "INSERT TBL_USER(U_NAME,U_GENGDER) VALUES('chensijie','male');"
#define SQL_SELECT_TBL_USER "SELECT * FROM TBL_USER;"
#define SQL_DELETE_TBL_USER "CALL PROC_DELETE_USER('chensijie');" //过程存储操作，存储于csj_DB数据库中
#define SQL_INSERT_IMG_USER "INSERT TBL_USER(U_NAME,U_GENGDER,U_IMG) VALUES('chensijie','male',?);" // ? 占位符
#define SQL_SELECT_IMG_USER "SELECT U_IMG FROM TBL_USER WHERE U_NAME='chensijie';"

#define FILE_IMAGE_LENGTH (3*207*240)
int mysql_select(MYSQL *handle){
    //mysql_real_query --> sql
     if(mysql_real_query(handle,SQL_SELECT_TBL_USER,strlen(SQL_SELECT_TBL_USER))){
        // 返回0 成功
        printf("mysql_real_query: %s\n",mysql_error(handle));
        return -1;
     }
    //store -->
    MYSQL_RES *res = mysql_store_result(handle);
    if(res==NULL){
        printf("mysql_restore_result: %s\n",mysql_error(handle));
        return -2;
    }
    //rows /cols
    int rows = mysql_num_rows(res);
    printf("rows: %d\n",rows);
    int cols = mysql_num_fields(res);
    printf("cols: %d\n",cols);
    //fetch
    MYSQL_ROW row; // 游标
    while (row = mysql_fetch_row(res))
    {
        int i=0;
        for(i=0;i<cols;i++){
             printf("%s\t",row[i]);
        }
        printf("\n");
    }
    mysql_free_result(res);
    return 0;
}
int read_image(char *filename,char *buffer){
    // 读取图片，并返回尺寸（字节）
    if(filename==NULL || buffer ==NULL) return -1;
    FILE *fp = fopen(filename,"rb"); // 读取二进制文件
    if(fp==NULL){
        printf("fopen failed\n");
        return -2;
    }
    //file size
    fseek(fp,0,SEEK_END); // 指向文件末尾
    int length = ftell(fp); // fifle size
    fseek(fp,0,SEEK_SET); // 指向文件开头
    int size = fread(buffer,1,length,fp); // 向buffer中1个字节大小读，共写length个字节
    if(size != length){
        printf("fread failed: %d\n",size);
        return -3;
    }
    fclose(fp);

    return size;
}
int write_image(char *filename,char *buffer,int length){
    if(filename==NULL||buffer==NULL||length<=0) return -1;
    FILE *fp = fopen(filename,"wb+"); // 没有就创建 
    if(fp==NULL){
        printf("fopen failed\n");
        return -2;
    }
    int size = fwrite(buffer,1,length,fp);
    if(size!=length){
        printf("fwrite failed: %d\n",size);
        return -3;
    }
    fclose(fp);
    return size;
}
int mysql_write(MYSQL *handle,char *buffer,int length){
    if(handle==NULL || buffer==NULL ||length<=0) return -1;
    // 准备数据
    MYSQL_STMT *stmt = mysql_stmt_init(handle); // ？占位符的声明
    if(mysql_stmt_prepare(stmt,SQL_INSERT_IMG_USER,strlen(SQL_INSERT_IMG_USER))){
        // 返回0 成功
        printf("mysql_stmt_prepare: %s\n",mysql_error(handle));
        return -2;
    }
    // 绑定占位符
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG_BLOB;
    param.buffer = NULL;
    param.is_null = 0;
    param.length = NULL;
    if(mysql_stmt_bind_param(stmt,&param)){
        printf("mysql_stmt_bind_param: %s\n",mysql_error(handle));
        return -3;
    }
    // 发送数据到占位符
    if(mysql_stmt_send_long_data(stmt,0,buffer,length)){
        printf("mysql_stmt_send_long_data: %s",mysql_error(handle));
        return -4;
    }
    // 发送数据到服务器
    if(mysql_stmt_execute(stmt)){
        printf("mysql_stmt_execute: %s",mysql_error(handle));
        return -5;
    }
    // 关闭占位符
    if(mysql_stmt_close(stmt)){
        printf("mysql_stmt_close: %s",mysql_error(handle));
        return -6;
    }
    return 0; 
}
int mysql_read(MYSQL *handle,char *buffer,int length){
    if(handle==NULL || buffer==NULL ||length<=0) return -1;
    // 准备数据
    MYSQL_STMT *stmt = mysql_stmt_init(handle); // ？占位符的声明
    if(mysql_stmt_prepare(stmt,SQL_SELECT_IMG_USER,strlen(SQL_SELECT_IMG_USER))){
        // 返回0 成功
        printf("mysql_stmt_prepare: %s\n",mysql_error(handle));
        return -2;
    }
    // 绑定结果
    MYSQL_BIND result = {0};
    result.buffer_type = MYSQL_TYPE_LONG_BLOB;
    unsigned long total_length = 0;
    result.length = &total_length;
    if(mysql_stmt_bind_result(stmt,&result)){
        printf("mysql_stmt_bind_result: %s\n",mysql_error(handle));
        return -3;
    }
    if (mysql_stmt_execute(stmt)){
        printf("mysql_stmt_execute: %s",mysql_error(handle));
        return -4;
    }
    if(mysql_stmt_store_result(stmt)){
        printf("mysql_stmt_store_result: %s",mysql_error(handle));
        return -5;
    }
    // 将结果的数据慢慢输送到buffer
    while (1)
    {
        int ret = mysql_stmt_fetch(stmt);
        if(ret!=0 &&  ret !=MYSQL_DATA_TRUNCATED) break; // 失败或者数据截短停止
        int start = 0;
        while (start<(int)total_length)
        {
            result.buffer = buffer+start; // 指向buffer的当前位置（首地址+偏移）
            result.buffer_length = 1; //每次输送1字节
            mysql_stmt_fetch_column(stmt,&result,0,start); //取出查询的第0列的下一部分数据,长度为1个字节,填充到result.buffer指定的缓冲区中。
            start += result.buffer_length;
        }
    }
    mysql_stmt_close(stmt);
    return total_length; 
}
int main(){
    // 创建数据库句柄
    MYSQL mysql;
    if(mysql_init(&mysql)==NULL){
        printf("mysql_init: %s\n",mysql_error(&mysql));
        return -1;
    }
    // 连接数据库
    if(!mysql_real_connect(&mysql,csj_DB_SERVER_IP,csj_DB_USERNAME,csj_DB_PASSWORD,csj_DB_DEFAULTDB,csj_DB_SERVER_PORT,NULL,0)){
        // 返回非0 成功
        printf("mysql_real_connect: %s\n",mysql_error(&mysql));
        return -2;
    }
#if 1
    // mysql --> insert
    if(mysql_real_query(&mysql,SQL_INSERT_TBL_USER,strlen(SQL_INSERT_TBL_USER))){  
        // 返回0 成功
        printf("mysql_real_query: %s\n",mysql_error(&mysql));
        return -3;
    }
    // mysql --> select
    printf("mysql --> select\n");
    mysql_select(&mysql);
#endif
    // mysql --> delete
    printf("mysql --> delete\n");
    if(mysql_real_query(&mysql,SQL_DELETE_TBL_USER,strlen(SQL_DELETE_TBL_USER))){  
        // 返回0 成功
        printf("mysql_real_query: %s\n",mysql_error(&mysql));
        return -4;
    }
    // 在执行存储过程 SQL_DELETE_TBL_USER 时，你没有明确处理存储过程的结果集。存储过程可能包含多个查询，
    // 每个查询都会生成一个结果集，但你没有为它们添加适当的处理逻辑。
    // 所以会出现Commands out of sync; you can't run this command now
     do {
        MYSQL_RES *result = mysql_store_result(&mysql);
        if (result) {
            mysql_free_result(result);
        }
    } while (!mysql_next_result(&mysql));
    // mysql --> select
    mysql_select(&mysql);

    // mysql --> insert img
    printf("mysql --> read image and write mysql\n");
    char buffer[FILE_IMAGE_LENGTH]={0};
    int length = read_image("./picture,pic1.jpg",buffer);
    printf("read img ok : %d!\n",length);
    if(length<0) goto Exit;
    mysql_write(&mysql,buffer,length);
    printf("write sql ok!\n");
    
    // mysql --> select img
    printf("mysql --> read mysql and write image\n");
    memset(buffer,0,FILE_IMAGE_LENGTH);
    length = mysql_read(&mysql,buffer,length);
    printf("read sql ok : %d!\n",length);
    int size = write_image("./picture,pic2.jpg", buffer, length);
    printf("write img ok : %d!\n",size);
Exit:
    mysql_close(&mysql);
    return 0;
}