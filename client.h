#ifndef _CLIENT_H
#define _CLIENT_H

#include "main.h"

#define EPOLL_SIZE 20

//创建主socket
int CreateMainClient(struct TobeConfig *TobeConfig);
//创建副socket
int CreateBackupClient(struct TobeConfig *TobeConfig);

#endif