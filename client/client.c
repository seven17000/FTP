#include "client.h"

int sock_control;

int main(int argc,char* argv[])
{
    int s,recode,sock_data;
    char buffer[MAXSIZE];
    struct command cmd;
    struct addrinfo hints,*res, *rp;

    //命令行参数检测
    if(argc != 3)
    {
        printf("usage:./client hostname port\n");
        exit(0);
    }

    char *host = argv[1];
    char *port = argv[2];


    memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;//设置为协议无关
    hints.ai_socktype = SOCK_STREAM;//设置为面向流传输
    s = getaddrinfo(host,port,&hints,&res);//把addrinfo信息与post,host存入res

    if(s != 0)
    {
        printf("getaddrinfo() error%s/n",gai_strerror(s));
        exit(1);
    }

    //getaddrinfo函数返回一个addrinfo结构的链表
    //这些addrinfo结构可以供后面套接字函数使用
    //这里遍历这个链表查找符合要求的addrinfo
    for(rp = res;rp != NULL;rp = rp->ai_next)
    {
        sock_control = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
        
        if(sock_control < 0)
            continue;
        
        //找到合适的addrinfo连接到服务器端
        if(connect(sock_control,res->ai_addr,res->ai_addrlen) == 0)
            break;
        else
        {
            perror("connect stream socket.\n");
            exit(2);
        }
        close(sock_control);
    }
    freeaddrinfo(rp);//addrinfo链表使用完需要free

    printf("connect to %s\n",host);
    print_reply(read_reply());

    //用户登录
    client_login();

    while(1)
    {//循环接收用户输入的命令直到用户输入"quit"

        //读取用户命令到cmd
        if(client_read_cmd(buffer,sizeof(buffer),&cmd) < 0)
        {
            close(sock_control);
            exit(3);
        }
        
        if(client_send_cmd(&cmd) < 0)
        {
            close(sock_control);
            perror("send cmd error.\n");
            exit(1);
        }
        recode = read_reply();

        //退出响应码
        if(recode == 221)
        {
            print_reply(221);
            break;
        }
        //非法输入响应码
        if(recode == 502)
        {
            printf("%d Invalid command.\n",recode);
        }
        //合法命令，处理命令
        else
        {
            if((sock_data = client_open_conn(sock_control)) < 0)
            {
                perror("Socket open error.\n");
                exit(4);
            }

            if(strcmp(cmd.code,"LIST") == 0)
            {
                client_list(sock_data,sock_control);
            }
            else if(strcmp(cmd.code,"RETR") == 0)
            {
                if(read_reply() == 550)
                {
                    print_reply(550);
                    close(sock_data);
                    continue;
                }
                
                client_get(sock_data,sock_control,cmd.arg);
                print_reply(read_reply());
            }
            
            close(sock_data);

        }
    }
    close(sock_control);
    return 0;
}

//接收服务器响应
//错误返回-1，正确返回响应码
int read_reply()
{
    int recode = 0;
    if(recv(sock_control,&recode,sizeof(recode),0) < 0)
    {
        perror("client read massage error.\n");
        return -1;
    }
    
    return ntohl(recode);
}

//打印响应信息
void print_reply(int recode)
{
    switch(recode)
    {
        case 220:
            printf("220:Welcome!\n");
            break;
        case 221:
            printf("221:Bye!\n");
            break;
        case 226:
            printf("226:Close data connection,Requested file action successful.\n");
            break;
        case 500:
            printf("Requested action not token,File unavailab.\n");
            break;
    }
}

//读取用户命令
int client_read_cmd(char *buf,int size,struct command *cmd)
{
    memset(cmd->code,0,sizeof(cmd->code));
    memset(cmd->arg,0,sizeof(cmd->arg));

    printf("client> ");//客户端提示信息
    fflush(stdout);
    read_input(buf,size);//从输入流中读取
    char *arg = NULL;
    arg = strtok(buf," ");
    arg = strtok(NULL," ");//拆分用户命令
    
    if(arg != NULL)//arg此时存放用户要获取的文件名
    {
        strncpy(cmd->arg,arg,strlen(arg));
    }

    //判断用户命令
    if(strcmp(buf,"list") == 0)
    {
        strcpy(cmd->code,"LIST");
    }
    else if(strcmp(buf,"get") == 0)
    {
        strcpy(cmd->code,"RETR");    
    }
    else if(strcmp(cmd->code,"quit") == 0)
    {
        strcpy(cmd->code,"QUIT");
    }
    else
    {
        return -1;
    }

    //存放用户命令到buf开始处
    memset(buf,0,MAXSIZE);
    strcpy(buf,cmd->code);

    //把剩下的参数追加到buf中
    if(arg != NULL)
    {
        strcat(buf," ");
        strncat(buf,arg,(int)strlen(cmd->arg));
    }

    return 0;
}

//用户登录
void client_login()
{
    struct command cmd;
    char user[256];
    memset(user,0,256);

    printf("Name: ");
    fflush(stdout);
    read_input(user,256);

    strcpy(cmd.code,"USER");
    strcpy(cmd.arg,user);
    client_send_cmd(&cmd);

    int wait;
    recv(sock_control,&wait,sizeof(wait),0);

    fflush(stdout);
    char* pass = getpass("Password: ");//关闭回显的密码输入函数返回值为输入字符串地址

    strcpy(cmd.code,"PASS");
    strcpy(cmd.arg,pass);
    client_send_cmd(&cmd);

    int recode = read_reply();
    switch(recode)
    {
        case 430:
            printf("Ivalid username or password.\n");
            exit(0);
        case 230:
            printf("Successful login.\n");
            break;
        default:
            perror("error reading massage from server.\n");
            exit(1);
    }
}

//客户端发送命令
int client_send_cmd(struct command *cmd)
{
    char buffer[MAXSIZE];
    int ret;

    sprintf(buffer,"%s %s",cmd->code,cmd->arg);//把cmd里的内容写到buffer中
    
    ret = send(sock_control,buffer,(int)strlen(buffer),0);
    if(ret < 0)
    {
        perror("Sending command error. /n");
        return -1;
    }
    
    return 0;
}

//打开数据连接获得服务端数据连接端口
int client_open_conn(int sock_control)
{
    int sock_listen = socket_create(CLIENT_PORT_ID);

    int ack = 1;
    if(send(sock_control,(char*)&ack,sizeof(ack),0) < 0)
    {
        printf("client:ack write error %d\n",errno);
        exit(1);
    }
    
    int sock_data = socket_accept(sock_listen);//返回服务端数据连接端口给sock_conn;
    close(sock_listen);
    return sock_data;
}

//处理用户get命令
int client_get(int sock_data,int sock_control,char *arg)
{
    char data[MAXSIZE];
    int size;
    FILE *fd = fopen(arg,"w");

    while((size = recv(sock_data,data,MAXSIZE,0)) > 0)
    {
        fwrite(data,1,size,fd);
    }
    printf("data recived.\n");

    if(size < 0)
    {
        perror("write error.\n");
    }

    fclose(fd);
    return 0;
}

//处理用户list命令
int client_list(int sock_data,int sock_control)
{
    char buffer[MAXSIZE];
    int temp = 0;
    size_t num_recv;

    if(recv(sock_control,&temp,sizeof(temp),0) < 0)
    {
        perror("client reads server'massage error.\n");
        return -1;
    }

    memset(buffer,0,sizeof(buffer));

    while((num_recv = recv(sock_data,buffer,MAXSIZE,0)) > 0)
    {
        printf("%s",buffer);
        memset(buffer,0,MAXSIZE);
    }
    
    if(num_recv < 0)
        perror("error.\n");

    if(recv(sock_control,&temp,sizeof(temp),0) < 0)
    {
        perror("client reads server'massage error.\n");
        return -1;
    }
    return 0;
}
