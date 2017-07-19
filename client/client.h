/*
 *内容：客户端接口实现
 *创建时间：2017.7.15
 *创建者：Seven17000
 *
 */


#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../common/common.h"

int read_reply();//读取响应码

void print_reply(int recode);//回显状态信息

int client_read_cmd(char *buf,int size,struct command *cmd);//用户命令读取接口

int client_get(int sock_data,int sock_control,char *arg);

int client_open_conn(int sock_control);

int client_list(int sock_data,int sock_control);

int client_send_cmd(struct command *cmd);//命令发送接口

void client_login();//用户登录接口

#endif

