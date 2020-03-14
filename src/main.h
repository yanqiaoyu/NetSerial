#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <regex.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>    
#include <termios.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>

//关于串口的一些设置
struct TobeConfig
{
	//波特率
	char baud[8];
	//数据位
	char data[2];
	//校验位
	char parity[2];
	//停止位
	char stop[2];
	//串口描述符
	int serial_fd;
	//串口路径
	char serial_path[64];

	//网络模式
	char mode[8];
	//端口
	char port[8];
	//协议
	char protocol[8];
	//server的IP地址,或者域名
	char serverIP[64];
	//备份端口号
	char bk_port[8];
	//备份IP
	char bk_IP[64];
	//备份协议
	char bk_protocol[8];
	//主副连接标志位
	int clientflag;
	//超时时间
	char timeout[8];
	//心跳间隔
	char heartbeattime[8];
	//心跳包内容
	char heartbeatbuf[64];
	//重连时间
	char reconnecttime[8];

	//数据长度
	char packetlength[16];
	//组包延时
	char packettime[16];

};

//最长的shell指令长度
#define SHELL_BUFF_LEN 256

//获取配置最长的长度
#define KEYVALLEN 100
//配置文件名
#define CONFIG_PATH "./NetSerial.conf"

#define TRUE 0
#define FALSE -1

//通过文件锁防止程序多次运行
void AvoidMultiRun();

//启动时，获取配置文件
void Init(struct TobeConfig *TobeConfig);

//提供调用的获取配置文件的接口
int GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal);

//自己重新复写了strncat，主要原因是既有的函数无法无差别复制串口过来的特殊字符
char * my_strncat(char *dst,const char *src,size_t count, int dst_len);

//重连等待函数
void RELINK_IN(int reconn_time);

#endif
