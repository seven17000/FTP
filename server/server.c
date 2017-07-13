#include "server.h"

int main(int argc,char *argv[])
{
    int sock_listen,sock_control,port,pid;

    if(argc != 2)
    {
        printf("usage:./server port\n");
        exit(0);
    }

    port = atoi(argv[1]);
    
    if((sock_listen = socket_create(port)) < 0)
    {
        perror("Socket create error.\n");
        exit(1);
    }
    
    while(1)
    {
        if((sock_control = sock_accept(sock_listen)) < 0)
            break;

        if((pid = fork()) < 0)
        {
            perror("Fork child process error.\n");
        }
        else if(pid == 0)
        {
            close(sock_listen);
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
    char *buf[MAXSIZE];
    char *user[MAXSIZE];
    char *pass[MAXSIZE];
    memset(buf,0,MAXSIZE);
    memset(user,0,MAXSIZE);
    memset(pass,0,MAXSIZE);
    
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

    msmset(buf,0,MAXSIZE);
    
    //接收密码
    if((recv_data(sock_control,buf,sizeof(buf))) == -1)
    {
        perror("recv error.\n");
        exit(1)
    }

    int i = 5;
    int n = 0;
    while(buf[i] != 0)
    {
        pass[n++] = buf[i++];
    }

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

    fd = fopen(".auth",r);
    if(fd == NULL)
    {
        perror("file not found");
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

        trimstr(password,strlen(password));
        
        if((strcmp(user,username) == 0)&&(strcmp(pass,password)))
            break;
    }

    free(line);
    fclose(fd);
    return auth;

}

//处理客户端请求接口
int server_process(int sock_control)
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
        send_response(sock_control,430);//认证失败
        exit(0);
    }

    //处理用户请求
    while(1)
    {
    
        int rc = server_recv_cmd(sock_control,cmd,arg);//接受并解析客户端命令，获取其参数
        
        if((rc < 0) || (rc == 221))
            break;

        if(rc == 200)
        {
            //创建与客户端连接
            if((sock_data = server_start_data_conn(sock_control)) < 0)
            {
                close(sock_control);
                exit(1);
            }
            
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
