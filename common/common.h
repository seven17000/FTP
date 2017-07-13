#ifndef COMMON_H
#define COMMON_H

#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<dirent.h>
#include<errmo.h>
#include<fcntl.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>

#define DEBUG 1
#define MAXSIZE 512
#define CLIENT_PORT_ID 30020

struct command
{
    char arg[255];
    char code[5];
}

int socket_create(int port);
int socket_accept(int sock_listen);
int socket_connect(int port,char *host);
int recv_data(int sockfd,char *buf,int bufsize);
int send_response(int sockfd,int rc);
void trimstr(char *str,int n);
void read_input(char *buffer,int size);

#endif
