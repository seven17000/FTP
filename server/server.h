/*
 *内容：实现服务端接口部分
 *创建时间：2017.7.13
 *创建者：Seven17000
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include "../common/common.h"

void server_retr(int sock_control,int sock_data,char *filename);

int server_list(int sock_data,int sock_control);

int server_start_data_conn(int sock_control);//数据连接创建接口

int server_check_user(char *user,char *pass);//用户名密码检测接口

int server_login(int sock_control);//用户登录接口

int server_recv_cmd(int sock_control,char *cmd,char *arg);//用户指令接收接口

int server_process(int sock_control);//客户端请求处理接口

#endif 

