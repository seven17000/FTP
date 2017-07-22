/*
 * 内容： FTP传输工具的通讯部分函数实现
 * 创建日期：2017.7.12
 * 创建者：Seven17000
 *
 */
#include "common.h"

//创建监听套接字
int socket_create(int port)
{
    int sockfd;
    int yes = 1;
    struct sockaddr_in sock_addr;
    if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket() error.\n");
        return -1;
    }
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY表示所有IP不确定
    //设置套接字选项
    //第一个参数为需要设置的套接字
    //第二个参数是套接字所在的层级SOL_SOCKET是为通用套接字
    //第三个套接字是套接字的的选项SO_REUSEADDR是允许重用本地地址和端口
    //第四个参数是一个选项类型的缓冲，根据选项的类型决定
    //第五个参数是选项的长度
    //成功返回0，失败返回-1
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
    {
        close(sockfd);
        perror("setsockopt() error.\n");
        return -1;
    }

    //设置选项成功后则绑定套接字到地址
    if(bind(sockfd,(struct sockaddr *) &sock_addr,sizeof(sock_addr)) < 0)
    {
        close(sockfd);
        perror("bind() error.\n");
        return -1;
    }

    //设置sockfd为监听套接字，最大监听数为5
    if(listen(sockfd,5) < 0)
    {
        close(sockfd);
        perror("listen() error.\n");
        return -1;
    }
    return sockfd;
}

//请求接收接口
//监听套接字接收请求
int socket_accept(int sock_listen)
{
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    sockfd = accept(sock_listen,(struct sockaddr *) &client_addr,&len);
    if(sockfd < 0)
    {
        perror("accept() error.\n");
        return -1;
    }
    return sockfd;
}

//创建连接接口
//成功返回连接套接字
//失败返回0
int socket_connect(int port,char *host)
{
    int sockfd;
    struct sockaddr_in dest_addr;
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket connect  error.\n");
        return -1;
    }
    memset(&dest_addr,0,sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host);
    if(connect(sockfd,(struct sockaddr*) &dest_addr,sizeof(dest_addr)) < 0)
    {
        close(sockfd);
        perror("connect to server error.\n");
        return -1;
    }
    return sockfd;
}

//接收数据
//从sockfd中接收大小为bufsize的数据到buf里
int recv_data(int sockfd,char *buf,int bufsize)
{
    size_t num_bytes;
    memset(buf,0,bufsize);
    num_bytes = recv(sockfd,buf,bufsize,0);
    if(num_bytes < 0)
    {
        return -1;
    }
    return num_bytes;
}

//发送应答码
//发送到sockfd
//发送成功返回0，失败返回-1
int send_response(int sockfd,int rc)
{
    int conv = htonl(rc);
    if(send(sockfd,&conv,sizeof(conv),0) < 0)
    {
        perror("send error.\n");
        return -1;
    }
    return 0;
}

//去除空格和换行
void trimstr(char *str,int n)
{
    for(int i = 0;i < n;i++)
    {
        if(isspace(str[i]))
            str[i] = 0;
        if(str[i] == '\n')
            str[i] = 0;
    }
}

//从命令行中读数据
void read_input(char *buffer,int size)
{
    char *nl = NULL;
    memset(buffer,0,size);
    if(fgets(buffer,size,stdin) != NULL)
    {
        nl = strchr(buffer,'\n');
        if(nl)
            *nl = '\0';//如果读到换行符则替换为字符串结束符
    }
}
