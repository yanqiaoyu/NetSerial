#include "client.h"

//以下变量仅用于当前C文件
int clientflag;
int port;
char protocol[8];
char serverIP[64];
int socketfd;
int packetlength;
int packettime;
int serialfd;


//存放UDP所需要的变量的结构体
typedef struct udpinfo
{
	struct sockaddr_in server_addr;
	socklen_t server_addr_length;
}UDP;

UDP udpinfo;

//判断域名是否合法，并解析
int is_domain_legal(char domain[64], int clientflag)
{
    //判断IP地址是否合法
    int a,b,c,d;
    char s[512];
    strcpy(s,domain);
    if (4==sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)) 
    {
        if (0<=a && a<=255
         && 0<=b && b<=255
         && 0<=c && c<=255
         && 0<=d && d<=255) 
        {
            printf("[%s] is valid IPv4\n",s);
        } 
        else 
        {
            printf("[%s] is invalid IPv4\n",s);
            return FALSE;
        }
    }
    //域名解析 
    else 
    {
        do
           {
                printf("[%s] is invalid IPv4\n",s);
                FILE * fp;  
                char shell_buf[SHELL_BUFF_LEN];
                memset(&shell_buf, 0, sizeof(shell_buf));

                if (snprintf(shell_buf, SHELL_BUFF_LEN, "echo -n `ping -c1 %s |awk -F'[(|)]' 'NR==1{print $2}'`", s) == -1)
                {
                perror("translate ip error");
                return FALSE;
                }
                fp = popen(shell_buf, "r");

                memset(&serverIP, 0, sizeof(serverIP));
                fread(serverIP, sizeof(serverIP), 1, fp);
                memset(&shell_buf, 0, sizeof(shell_buf));  
                printf("translate to IP:[%s]\n",serverIP);
                sleep(1);

           } while (strlen(serverIP) == 0); 
    }
	return TRUE;
}

void *UDP_T2S()
{
	char data_recv[packetlength];
	
	//在内核中创建事件表
	//
	int epfd = epoll_create(EPOLL_SIZE);
	if(epfd < 0)
	{
		perror("epfd error"); 
		pthread_exit(NULL);
	}
	//printf("epoll created, epollfd = %d\n", epfd);

	//定义一个epoll事件
	static struct epoll_event events[EPOLL_SIZE];

    //往内核事件表里添加事件
    struct epoll_event ev;
    //ev.data.fd=socketfd=4
    ev.data.fd = socketfd;

    ev.events = EPOLLIN;

    epoll_ctl(epfd, EPOLL_CTL_ADD, socketfd, &ev);

    printf("[--UDP Socket Added to Epoll!--]\n");

	int i;
	//从socket接收的字节数
	int socket_recvBytes=0;
	//成功写入串口缓冲区的字节数
	int serial_sendBytes=0;
	//总字节数
	int Bytes_all=0;

	//配置应答时间
	int ack;
	//如果配置为0,则不设置超时时间，永久阻塞
	if ( 0 == acktime.time )
	{
		//在UDP协议下，并没有连接，所以300秒没收到数据自动断开连接
		ack = 300*1000;
	}
	//否则，配置超时时间(毫秒)
	else
	{
		ack = acktime.time*1000;
	}

    while(1)
    {
    	memset(&data_recv, 0, sizeof(data_recv));

	    int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, ack);
	    //printf("epoll_events_count:%d\n", epoll_events_count);
	    //出错
    	if (epoll_events_count < 0)
	    {
	        perror("epoll failure");
	        break;
	    }
	    //超时,在ack时间之内，没有收到服务器消息就退出
    	else if(epoll_events_count == 0) 
	    {
	       	printf("NO DATA in %d Second, disconnect..\n", acktime.time);
	        break;
	    }

	    //从socket中读取数据
        socket_recvBytes = recvfrom(ev.data.fd, data_recv, packetlength, 0, (struct sockaddr *)&udpinfo.server_addr, &udpinfo.server_addr_length);
        Bytes_all += socket_recvBytes;

    	//计数        
    	//printf("ALL BYTES:%d\n", Bytes_all);
    	printf("Recv:%s>>>", data_recv);

        serial_sendBytes = write(serialfd, data_recv, socket_recvBytes);

        if (socket_recvBytes == serial_sendBytes)
            {
              //打印发往串口的消息
              printf("Send to Serial OK!\n");
              //show_hex(data_recv, i_recvBytes);
              //printf("\n");

              //闪烁
              shining();
            }     
        else   
            {                  
              tcflush(serialfd, TCOFLUSH);
            }
    }
    close(epfd);
    close(socketfd); 
	pthread_exit(NULL); 
}

void *UDP_S2T()
{
	//异步取消，收到取消命令立刻退出线程
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	char tmp_buff[packetlength];
	char data_buff[packetlength];
	memset(&tmp_buff, 0, sizeof(tmp_buff));
	memset(&data_buff, 0, sizeof(data_buff));

	//组包的时间,单位是毫秒
	struct timeval tv;
	if (packettime == 0)
	{
		packettime=1;
		tv.tv_sec=0;
		//1ms
		tv.tv_usec=10;		
	}
	else
	{
		tv.tv_sec=0;
		tv.tv_usec=packettime*1000;
	}


	//清除上一次传往串口的数据,
	//这一句可以解决一下子收到许多之前残留的信息的问题
	tcflush(serialfd, TCIOFLUSH);

	//初始化某些计数变量
	int tmp_buff_len = 0;
	int total_len = 0;

	//select需要的描述符合集
	fd_set rfds;

	//获取心跳时间
	long int uTime = heartbeat.time*1000*1000;

    printf("[--Serialfd Added to Select!--]\n");	
//------------------------------------------------//
    while(1)
	{
		//初始化，每次select都需要
		FD_ZERO(&rfds);
		FD_SET(serialfd, &rfds);

        //利用select函数完成串口数据的读取和超时设定
        if (select(1+serialfd, &rfds, NULL, NULL, &tv)>0)
        {
        	//读取串口缓存中的数据，记录下读取的字节数
			tmp_buff_len = read(serialfd, tmp_buff, sizeof(tmp_buff));
			//把本次读取的字节数，添加到总字节数中
			total_len += tmp_buff_len;

			//先判断包的剩余空间是否能容纳当前接收到的数据
            //1.如果是的，就组包
            if ((sizeof(data_buff)-(total_len-tmp_buff_len)) >= tmp_buff_len)
            {				
				my_strncat(data_buff, tmp_buff, tmp_buff_len, total_len-(tmp_buff_len));
				printf(">>> Filling the Buffer with Data![%d]in[%d]\n", total_len, packetlength);

                //1.1如果组包刚刚好达到容量上限就立即发送		
                if (sizeof(data_buff) == total_len)
                {
					printf(">>> Buffer is Full!\n");

					if(sendto(socketfd, data_buff, packetlength, 0, (struct sockaddr*)&udpinfo.server_addr, udpinfo.server_addr_length) == -1)
					{
					  close(socketfd);          
					  pthread_exit(NULL);
					}
					//清空发送包，重新计数
					memset(&data_buff, 0, sizeof(data_buff));
					total_len = 0;						

					//闪烁 
					shining(); 

                }//send full   
				//1.2重新开始计时
				tv.tv_usec=packettime*1000;
            }//not full
            //2.没有足够的空间,就把能容纳的数据先组合，然后剩余的数据放入下一次发送的包
            else
            {
				printf("Buff isn't BIG ENOUGH!\n");
				//2.1先计算还剩下多少空间
				int space = sizeof(data_buff)-(total_len-tmp_buff_len);
				printf("[%d] Left, but [%d] to put\n", space, tmp_buff_len);	
				//2.2然后相应的数据放入buff中
				my_strncat(data_buff, tmp_buff, space, (total_len-tmp_buff_len));

				//不需要发送ID

				//2.3发送这个已经满载的数据包
				if(sendto(socketfd, data_buff, packetlength, 0, (struct sockaddr*)&udpinfo.server_addr, udpinfo.server_addr_length) == -1)
				{
					close(socketfd);
					pthread_exit(NULL);					          
				}					

				//闪烁 
				shining(); 		
				
				//2.4清空这个数据包，并把剩下的数据放入这个空数据包中
				memset(&data_buff, 0, sizeof(data_buff));

				if(sendto(socketfd, data_buff, packetlength, 0, (struct sockaddr*)&udpinfo.server_addr, udpinfo.server_addr_length) == -1)
				{
					close(socketfd);
					pthread_exit(NULL);	          
				}
	
				//2.5重新开始计时
				tv.tv_usec=packettime*1000;
				total_len = 0;																									
            }//not enough space
			memset(&tmp_buff, 0, sizeof(tmp_buff));
        }//select if
        //3.组包超时
        else
        {				
			
			if (total_len > 0)
            {

	            //3.1直接发送当前没有组满的数据包
				if(sendto(socketfd, data_buff, total_len, 0, (struct sockaddr*)&udpinfo.server_addr, udpinfo.server_addr_length)  < 0)
				{
					close(socketfd);
					pthread_exit(NULL);	          
				}
            	
              //闪烁 
              shining(); 
            }
           
			memset(&data_buff, 0, sizeof(data_buff));
			memset(&tmp_buff, 0, sizeof(tmp_buff));
			//3.2重新开始计时
			tv.tv_usec=packettime*1000;
			total_len = 0;
        }

		if( heartbeat.time != 0 )
		{
	        //每隔一个select周期去检测
	        uTime -= packettime*1000;
	        if (uTime <= 0)
	        {
				//send_Heart_Beat(heart_beat_enable);
	        	sendto(socketfd, data_buff, total_len, 0, (struct sockaddr*)&udpinfo.server_addr, udpinfo.server_addr_length);
				uTime = heartbeat.time*1000*1000;
	        }
		} 
	}//while
}

int udp()
{
	printf(">>> UDP->%s:%d\n", domain, port);
	if (FALSE == is_domain_legal(domain, clientflag) )
	{
		printf("!!! Error in domain\n");
		return FALSE;
	}

	udpinfo.server_addr_length = sizeof(udpinfo.server_addr);

	//创建两个线程ID
	pthread_t UDP_T2S_id, UDP_S2T_id;

	//服务端地址  
	bzero(&udpinfo.server_addr, sizeof(udpinfo.server_addr)); 
	udpinfo.server_addr.sin_family = AF_INET; 
	udpinfo.server_addr.sin_addr.s_addr = inet_addr(domain); 
	udpinfo.server_addr.sin_port = htons(port); 

	//创建UDP socket
	//IPV4  SOCK_DGRAM 数据报套接字（UDP协议） 
	if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{ 
		perror("*** Create Socket Failed:"); 
		return FALSE;
	}
	printf(">>> Entering UDP Client Working Mode \n");

	//处理 UDP服务器发往串口函数 的线程函数
	if (pthread_create(&UDP_T2S_id, NULL, &UDP_T2S, NULL) == -1)
	{
		perror("!!! UDP_T2S pthread create error. !!!\n");
		return FALSE;
	}

	//处理 UDP串口发往服务器 的线程函数
	if (pthread_create(&UDP_S2T_id, NULL, &UDP_S2T, NULL) == -1)
	{
		perror("!!! UDP_S2T pthread create error. !!!\n");
		return FALSE;
	}

	//阻塞式回收线程资源
	if(pthread_join(UDP_T2S_id, NULL) == 0)
	{
		printf(">>> Recycle pthread [-UDP_T2S-]\n");
		pthread_cancel(UDP_S2T_id);
	}

	if(pthread_join(UDP_S2T_id, NULL) == 0)
	{
		printf(">>> Recycle pthread [-UDP_S2T-]\n");
	}
	return TRUE;
}

int create_socket()
{
	//先判断使用的是什么协议
	//如果使用的是TCP
	if (	strncmp(protocol, "tcp", 3) == 0 
		||	strncmp(protocol, "TCP", 3) == 0 )
	{
		if( FALSE == tcp(domain, port, clientflag) )
		{
			printf("!!! Something Wrong in current client Socket !!!\n");
			return FALSE;
		}		
	}
	//如果使用的是UDP
	else if (strncmp(protocol, "udp", 3) == 0 
		||	strncmp(protocol, "UDP", 3) == 0 )
	{
		if( FALSE == udp() )
		{
			printf("!!! Something Wrong in current client Socket !!!\n");
			return FALSE;
		}
	}
	return TRUE;	
}

//创建主socket
int CreateMainClient(struct TobeConfig *TobeConfig)
{
	clientflag = TobeConfig->clientflag;
    port = atoi(TobeConfig->port);
    memset(&protocol, 0 , sizeof(protocol));
    memset(&serverIP, 0 , sizeof(serverIP));
    strncpy(protocol, TobeConfig->protocol, strlen(TobeConfig->protocol));
    strncpy(serverIP, TobeConfig->serverIP, strlen(TobeConfig->serverIP));
    packetlength = TobeConfig->packetlength;
    packettime = TobeConfig->packettime;
    

    //printf("%d %d %s %s\n", clientflag, port, protocol, serverIP);
	return create_socket();
    exit(0);
}

//创建副socket
int CreateBackupClient(struct TobeConfig *TobeConfig)
{
    clientflag = TobeConfig->clientflag;
    port = atoi(TobeConfig->bk_port);
    memset(&protocol, 0 , sizeof(protocol));
    memset(&serverIP, 0 , sizeof(serverIP));
    strncpy(protocol, TobeConfig->bk_protocol, strlen(TobeConfig->bk_protocol));
    strncpy(serverIP, TobeConfig->bk_IP, strlen(TobeConfig->bk_IP));
    packetlength = TobeConfig->packetlength;
    packettime = TobeConfig->packettime;
	
	return create_socket();
}