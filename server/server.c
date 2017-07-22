#include "server.h"

int main(int argc,char *argv[])
{
    int sock_listen,sock_control,port,pid;
    
    //检测命令行参数是否正确
    if(argc != 2)
    {
        printf("usage:./server port\n");
        exit(0);
    }
    
    //端口号参数转化为int型
    port = atoi(argv[1]);
    
    //创建监听套接字
    if((sock_listen = socket_create(port)) < 0)
    {
        perror("Socket create error.\n");
        exit(1);
    }
    
    //监听客户端控制连接请求
    while(1)
    {
        if((sock_control = socket_accept(sock_listen)) < 0)
            break;
        
        //收到连接请求创建新进程处理请求
        if((pid = fork()) < 0)
        {
            perror("Fork child process error.\n");
        }
        else if(pid == 0)
        {
            close(sock_listen);//保证一对一连接，先关闭监听套接字
            server_process(sock_control);
            close(sock_control);
            exit(2);
        } 
        close(sock_control);
    }
    close(sock_listen);
    return 0;
}

//用户登录接口
int server_login(int sock_control)
{
    char buf[MAXSIZE];
    char user[MAXSIZE];
    char pass[MAXSIZE];
    memset(user,0,MAXSIZE);
    memset(pass,0,MAXSIZE);
    memset(buf,0,MAXSIZE);
    
    //接收用户名
    if((recv_data(sock_control,buf,sizeof(buf))) == -1)
    {
        perror("recv error.\n");
        exit(1);
    }

    int i = 5;
    int n = 0;
    while(buf[i] != 0)
    {
        user[n++] = buf[i++];
    }

    send_response(sock_control,331);//用户名接收完成，通知客户端输入密码

    memset(buf,0,MAXSIZE);
    
    //接收密码
    if((recv_data(sock_control,buf,sizeof(buf))) == -1)
    {
        perror("recv error.\n");
        exit(1);
    }

    while(buf[i] != 0)
    {
        pass[n++] = buf[i++];
    }

    //检测用户名密码是否匹配
    return server_check_user(user,pass);
}

//用户名密码检查接口
int server_check_user(char *user,char *pass)
{
    char username[MAXSIZE];
    char password[MAXSIZE];
    char *pch;
    char buf[MAXSIZE];
    char *line = NULL;
    size_t num_read;
    size_t len = 0;
    FILE* fd;
    int auth = 0;

    fd = fopen(".auth","r");
    if(fd == NULL)
    {
        perror("file not found.\n");
        exit(1);
    }

    //获取.auth文件中的用户名和密码，验证其合法性
    /*
     * getline函数
     *返回值：成功返回读取的字节数，失败返回-1
     *
     *参数，第一个参数为指向存放该行字符的指针，如果是空则由系统帮助mallc，
     *第二个参数为读取长度，如果是系统帮助mallc的则为0，
     *第三个参数是所读取文件的文件描述符
     */
    while((num_read = getline(&line,&len,fd)) != -1)
    {
        memset(buf,0,MAXSIZE);
        strcpy(buf,line);

        pch = strtok(buf," ");//切分用户输入的用户名和密码
        strcpy(username,pch);

        if(pch != NULL)
        {
            pch = strtok(NULL," ");
            strcpy(password,pch);
        }
        //去除空格和换行
        trimstr(password,strlen(password));
        
        //判断密码用户名是否匹配
        if((strcmp(user,username) == 0)&&(strcmp(pass,password)))
        {
            auth = 1;
            break;
        }
    }

    free(line);
    fclose(fd);
    return auth;

}

//用户指令接收接口
int server_recv_cmd(int sock_control,char *cmd,char *arg)
{
    int rc = 200;
    char buffer[MAXSIZE];
    
    memset(buffer,0,MAXSIZE);
    memset(cmd,0,5);
    memset(arg,0,MAXSIZE);
    
    //接收控制套接字中的数据到buffer中
    if((recv_data(sock_control,buffer,sizeof(buffer))) == -1)
    {
        perror("recv error./n");
        exit(1);
    }
    
    //分离出cmd指令和arg
    strncpy(cmd,buffer,4);
    char *tmp = buffer+5;
    strcpy(arg,tmp);
    
    if(strcmp(cmd,"QUIT") == 0)
    {
        rc = 221;//退出指令
    }
    else if((strcmp(cmd,"USER") == 0) || (strcmp(cmd,"PASS") == 0) || (strcmp(cmd,"LIST") == 0)|| (strcmp(cmd,"RETR") == 0))
    {
        rc = 200;//指令有效
    }
    else
    {
        rc = 502;//无效指令
    }
    
    send_response(sock_control,rc);//发送应答码到控制套接字
    return rc;
}


//数据连接创建接口
//成功返回数据连接套接字
//失败返回-1
int server_start_data_conn(int sock_control)
{
    char buffer[MAXSIZE];
    int wait,sock_data;
    
    //确定连接成功
    if(recv(sock_control,&wait,sizeof(wait),0) < 0)
    {
        perror("recv error.\n");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    getpeername(sock_control,(struct sockaddr*)&client_addr,&len);//从控制套接字获取远程地址（获取客户端IP地址）
    inet_ntop(AF_INET,&client_addr.sin_addr,buffer,sizeof(buffer));//把IP地址转换为主机字节序放入client_addr

    //连接客户端
    if((sock_data = socket_connect(CLIENT_PORT_ID,buffer)) == 0)
    {
        return -1;
    }

    return sock_data;
}

//文件发送接口
//通过数据套接字发送指定文件
//通过控制套接字控制信息交互
//处理无效或者不存在的文件
void server_retr(int sock_control,int sock_data,char *filename)
{
    FILE *fd = NULL;
    char data[MAXSIZE];
    size_t num_read;

    fd = fopen(filename,"r");

    if(!fd)
    {
        send_response(sock_control,550);//返回响应码，打开文件失败
    }
    else
    {
        printf("start write data.\n");
        send_response(sock_control,150);//返回响应吗，打开文件成功
        
        //从数据连接上读数据直到数据读完
        do
        {
            num_read = fread(data,1,MAXSIZE,fd);
            if(num_read < 0)
            {
                printf("fread error.\n");
            }
            
            if((send(sock_data,data,num_read,0)) < 0)
            {
                printf("send error.\n");
            }
            
        }while(num_read > 0);
        printf("data write over.\n");
        send_response(sock_control,226);//返回响应码，数据读完关闭文件
        fclose(fd);
    }

}
//响应请求：发送当前所在目录的目录项列表
//关闭数据连接
//错误返回-1，正确返回0
int server_list(int sock_data,int sock_control)
{
    char data[MAXSIZE];
    size_t num_read;
    FILE *fd;

    int rc = system("ls -l | tail -n+2 > tmp.txt");//系统调用ls -l 并重定向到文件
    if(rc<0)
    {
        exit(1);
    }

    fd = fopen("tmp.txt","r");//读方式打开文件
    if(!fd)
    {
        exit(1);
    }

    fseek(fd,SEEK_SET,0);//定位到文件开始位置

    send_response(sock_control,1);

    memset(data,0,MAXSIZE);

    //从fd一次读取MAXSIZE个字节到data
    //并发送到数据连接套接字
    //直到读完
    while((num_read = fread(data,1,MAXSIZE,fd)) > 0)
    {
        if(send(sock_data,data,num_read,0) < 0)
        {
            perror("send error.\n");
        }
        memset(data,0,MAXSIZE);
    }
    
    fclose(fd);
    send_response(sock_control,226);//发送应答码，数据传输完毕，关闭数据连接端口
    return 0;
}

//处理客户端请求接口
void server_process(int sock_control)
{
    int sock_data;
    char cmd[5];
    char arg[MAXSIZE];
    
    send_response(sock_control,220);//发送应答码

    if(server_login(sock_control) == 1)
    {
        send_response(sock_control,230);//认证成功
    }
    else
    {
        send_response(sock_control,530);//认证失败
        exit(0);
    }

    //处理用户请求
    while(1)
    {
    
        int rc = server_recv_cmd(sock_control,cmd,arg);//接受并解析客户端命令，获取其参数
        printf("server recveice cmd:%s\n",cmd);

        if((rc < 0) || (rc == 221))
        {
            printf("client quit!\n");
            break;
        }

        if(rc == 200)
        {
            //创建与客户端连接
            if((sock_data = server_start_data_conn(sock_control)) < 0)
            {
                close(sock_control);
                exit(1);
            }
            printf("data connect start.\n");

            //执行指令
            if(strcmp(cmd,"LIST") == 0)
            {
                server_list(sock_data,sock_control);
            }
            else if(strcmp(cmd,"RETR") == 0)
            {
                server_retr(sock_control,sock_data,arg);
            }
            close(sock_data);
        }
    }
}
