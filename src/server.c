#include "server.h"

//存放此C文件所需要的变量，在入口处已经被赋值
int serialfd;
int packetlength;
int packettime;

//socket设置需要的变量
typedef struct sockaddr SA;
int socketfd;

int size=20;
int fds[20];

void send_full(char data_buff[packetlength])
{
    int i;
    for (i = 0;i < size; i++){
        if (fds[i] != 0 && fds[i] != -1){
            //printf("sendto%d\n",fds[i]);
            if(send(fds[i],data_buff, packetlength, MSG_WAITALL) == -1)
            {
                perror("Serial2telnet Error.\n");
                printf("terminating current client_connection...\n");
                //close(fds[i]);          
            }
        }
    }
}

void send_half(char data_buff[packetlength], int len)
{
    int i;
    for (i = 0;i < size; i++){
        if (fds[i] != 0 && fds[i] != -1){
            //printf("sendto%d\n",fds[i]);
            if(send(fds[i],data_buff, len, MSG_WAITALL) == -1)
            {
                perror("Serial2telnet Error.\n");
                printf("terminating current client_connection...\n");
                //close(fds[i]);          
            }
        }
    }
}

void send_outtime(char data_buff[packetlength], int total_len)
{
    int i;
    for (i = 0;i < size; i++){
        if (fds[i] != 0 && fds[i] != -1){
            //printf("sendto%d\n",fds[i]);
            if(send(fds[i],data_buff,total_len, MSG_WAITALL) == -1)
            {
                perror("Serial2telnet Error.\n");
                printf("terminating current client_connection...\n");
                //close(fds[i]);          
            }
        }
    }
}

void* service_thread(void* p)
{
    //分离线程，退出后自动释放资源
    pthread_detach(pthread_self());

    int fd = *(int*)p;

    printf("pthread = %d\n",fd);
    int i_recvBytes;
    int j;
    int count = 0;
    while(1)
    {
        char buf[packetlength];
        memset(&buf, 0, sizeof(buf));
        if ((i_recvBytes = recv(fd,buf,sizeof(buf),0)) <= 0)
        {
            int i;
            for (i = 0;i < size; i++)
            {
                if (fd == fds[i])
                {
                    fds[i] = 0;
                    break;
                }
            }
            for (j = 0; j < size; ++j)
            {
                if (fds[j] == 0)
                {
                    ++count;
                }
            }
            
            printf("退出：fd = %d 主动 quit\n",fd);
            printf("还有[%d]个位置可用\n", count);
            pthread_exit((void*)i);
        }
        else
        {
          int len = 0;
          //len = write(p_fd,data_recv,strlen(data_recv));
          len = write(serialfd, buf, i_recvBytes);

          if (len == i_recvBytes )
              {
                //打印发往串口的消息
                printf("Send to Serial:%s\n", buf);
              }     
          else   
              {                  
                tcflush(serialfd, TCOFLUSH);
              }
        }

        printf("buf:%s from:%d\n", buf, fd);
    }
}

void* my_accept(void* p)
{
  //分离线程，退出后自动释放资源
  pthread_detach(pthread_self());

    while(1)
    {
        struct sockaddr_in fromaddr;
        socklen_t len = sizeof(fromaddr);
        //阻塞在这里等待ACCEPT
        int fd = accept(socketfd, (SA*)&fromaddr,&len);
        if (fd == -1)
        {
            printf("accept error ...\n");
            continue;
        }

        //把这个新加入的客户端，放入描述符的池子中
        int i = 0,j = 0,count = 0;
        for (i = 0; i < size; i++)
        {
            if (fds[i] == 0)
            {
                //记录客户端的socket
                fds[i] = fd;
                printf("new fd = %d\n",fd);
                pthread_t tid;
                pthread_create(&tid, 0, service_thread,&fd);
                break;
            }
        }

        //轮询查询当前可用客户端
        for (j = 0; j < size; ++j)
        {
            if (fds[j] == 0)
            {
                ++count;
            }
        }
        printf("还有[%d]个位置可用\n", count);
    }
}


//创建一个TCP的服务端
int tcp_server(struct TobeConfig *TobeConfig)
{
	//创建一个socket
	socketfd = socket(PF_INET,SOCK_STREAM,0);
	if (socketfd == -1)
	{
		perror("create socket error");
		return FALSE;
	}

    //设置以下的参数主要是为了解决网线不小心断开所引起的超时问题
	//开启心跳机制
	int keep_alive = 1;
	// 如该连接在xx秒内没有任何数据往来,则进行探测
	int keep_idle = 2;
	// 探测时发包的时间间隔为x秒
	int keep_interval = 2;
	// 探测尝试的次数.如果第1次探测包就收到响应了,则后x次的不再发.
	int keep_count = 2;
	int ret = 0;

	//长连接开启        
	if (-1 == (ret = setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive,
	  sizeof(keep_alive)))) {
	    fprintf(stderr, "[%s %d] set socket to keep alive error", __FILE__,
	      __LINE__);
	}
	if (-1 == (ret = setsockopt(socketfd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle,
	  sizeof(keep_idle)))) {
	    fprintf(stderr, "[%s %d] set socket keep alive idle error", __FILE__,
	      __LINE__);
	}
	if (-1 == (ret = setsockopt(socketfd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval,
	  sizeof(keep_interval)))) {
	    fprintf(stderr, "[%s %d] set socket keep alive interval error", __FILE__,
	      __LINE__);
	}
	if (-1 == (ret = setsockopt(socketfd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count,
	  sizeof(keep_count)))) {
	    fprintf(stderr, "[%s %d] set socket keep alive count error", __FILE__,
	      __LINE__);
	}

	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
    int port = atoi(TobeConfig->port);
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//端口复用
	int opt = 1; 
	setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(socketfd,(SA*)&addr,sizeof(addr)) == -1)
	{
		perror("bind error");
		return FALSE;
	}
	if (listen(socketfd,5) == -1)
	{
		perror("listen error");
		return FALSE;
	}

	printf(">>> Waiting for new connection ! <<<\n"); 

	//创建接收的线程
	pthread_t p_accept;
	pthread_create(&p_accept, 0, my_accept, NULL);

	while(1)
	{
		//组包的数据长度
		char tmp_buff[packetlength];
		char data_buff[packetlength];

		struct timeval tv;
		fd_set rfds;

		//组包等待时间在这里设定
		tv.tv_sec=0;
		tv.tv_usec=packettime*1000;

		//清除上一次传往串口的数据,
		//这一句可以解决一下子收到许多之前残留的信息的问题
		tcflush(serialfd,TCIOFLUSH);

		int tmp_buff_len = 0;
		int total_len = 0;

		while(1)
        {
          FD_ZERO(&rfds);
          FD_SET(serialfd, &rfds);

            //利用select函数完成串口数据的读取和超时设定
            if (select(1+serialfd, &rfds, NULL, NULL, &tv)>0)
            {

              tmp_buff_len = read(serialfd, tmp_buff, sizeof(tmp_buff));

              printf("%s\n", tmp_buff);

              total_len += tmp_buff_len;

              //先判断包的剩余空间是否能容纳当前接收到的数据
                //1.如果是的，就组包
                if ((sizeof(data_buff)-(total_len-tmp_buff_len)) >= tmp_buff_len)
                {
                  my_strncat(data_buff, tmp_buff, tmp_buff_len, total_len-(tmp_buff_len));
                    //1.1如果组包刚刚好达到容量上限就立即发送
                    if (sizeof(data_buff) == total_len)
                    {
                      //修改为一个函数
                      send_full(data_buff);

                      memset(&data_buff, 0, sizeof(data_buff));
                      total_len = 0;
                    }
                  //1.2重新开始计时
                  tv.tv_usec = packettime*1000;

                }

                //2.没有足够的空间,就把能容纳的数据先组合，然后剩余的数据放入下一次发送的包
                else
                {
                  //2.1先计算还剩下多少空间
                  int space = sizeof(data_buff)-(total_len-tmp_buff_len);
                  //2.2然后相应的数据放入buff中
                  my_strncat(data_buff, tmp_buff, space, (total_len-tmp_buff_len));

                  //修改为一个函数
                  send_full(data_buff);

                  //2.4清空这个数据包，并把剩下的数据放入这个空数据包中
                  memset(&data_buff, 0, sizeof(data_buff));
                  printf("tmp_buff_len-space:[%d]\n", tmp_buff_len-space);

                  //修改为一个函数
                  send_half(tmp_buff+space, tmp_buff_len-space);

                  //2.5重新开始计时
                  tv.tv_usec = packettime*1000;
                  total_len = 0;
                } 

             memset(&tmp_buff, 0, sizeof(tmp_buff));

            }

            //3.组包超时
            else
            {
              //修改为一个函数
              send_outtime(data_buff, total_len);

              memset(&data_buff, 0, sizeof(data_buff));
              memset(&tmp_buff, 0, sizeof(tmp_buff));
              //3.2重新开始计时
              tv.tv_usec = packettime*1000;
              total_len = 0;
            }
        }
	}
}

//----------------------下面是 udp server---------------------//

//UDP client 全局变量
struct sockaddr_in client;
socklen_t len = sizeof(client);

//处理接收服务器消息函数
void * udp_telnet2serial_s()
{
  char data_recv[packetlength];
  int i_recvBytes;

  while(1)
  {
    //初始化缓存空间
    memset(data_recv,0,packetlength);

    i_recvBytes = recvfrom(socketfd, data_recv, packetlength, 0, (struct sockaddr *)&client, &len);
    
    int len = 0;
    len = write(serialfd, data_recv, i_recvBytes);

    if (len == i_recvBytes )
        {
          //打印发往串口的消息
          printf("Send to Serial:%s:%d", data_recv, i_recvBytes);
          printf("\n");

          
        }     
    else   
        {                  
          tcflush(serialfd, TCOFLUSH);
        }
  }//while

  //清理工作
  //printf("terminating current server_connection...\n");
  close(socketfd);          
  pthread_exit(NULL); 
}

//处理发送串口消息
void * udp_serial2telnet_s()
{
  //组包的数据长度
  char tmp_buff[packetlength];
  char data_buff[packetlength];
  memset(&tmp_buff, 0, sizeof(tmp_buff));
  memset(&data_buff, 0, sizeof(data_buff));

  struct timeval tv;
  fd_set rfds;

  //组包等待时间在这里设定
  tv.tv_sec = 0;
  tv.tv_usec = packetlength * 1000;

  int p_fd = serialfd;
  int s_fd = socketfd;

  //清除上一次传往串口的数据,
  //这一句可以解决一下子收到许多之前残留的信息的问题
  tcflush(p_fd,TCIOFLUSH);
  int tmp_buff_len = 0;
  int total_len = 0;

  while(1)
    {
      FD_ZERO(&rfds);
      FD_SET(p_fd, &rfds);

        //利用select函数完成串口数据的读取和超时设定
        if (select(1+p_fd, &rfds, NULL, NULL, &tv)>0)
        {

          tmp_buff_len = read(p_fd, tmp_buff, sizeof(tmp_buff));

          total_len += tmp_buff_len;

          //先判断包的剩余空间是否能容纳当前接收到的数据
            //1.如果是的，就组包
            if ((sizeof(data_buff)-(total_len-tmp_buff_len)) >= tmp_buff_len)
            {
              printf("filling the buffer with data!\n");
              my_strncat(data_buff, tmp_buff, tmp_buff_len, total_len-(tmp_buff_len));
              printf("strlen:%d sizeof:%ld\n",total_len, sizeof(data_buff) );

                //1.1如果组包刚刚好达到容量上限就立即发送
                if (sizeof(data_buff) == total_len)
                {
                  printf("the data_buff is full!\n");

                  if(sendto(s_fd, data_buff, packetlength, 0, (struct sockaddr*)&client, len) == -1)
                  {
                      //perror("Serial2telnet Error.\n");
                      //printf("terminating current client_connection...\n");
                      close(s_fd);          
                      //pthread_exit(NULL);
                      //return FALSE; 
                  }

                  

                  memset(&data_buff, 0, sizeof(data_buff));
                  total_len = 0;
                }
              //1.2重新开始计时
              tv.tv_usec=packettime*1000;

            }

            //2.没有足够的空间,就把能容纳的数据先组合，然后剩余的数据放入下一次发送的包
            else
            {
              printf("the data_buff is not big enough!\n");

              //2.1先计算还剩下多少空间
              int space = sizeof(data_buff)-(total_len-tmp_buff_len);
              printf("there is [%d], but [%d] to put\n", space, tmp_buff_len);
              //2.2然后相应的数据放入buff中
              my_strncat(data_buff, tmp_buff, space, (total_len-tmp_buff_len));

              //2.3发送这个已经满载的数据包
                  if(sendto(s_fd , data_buff , packetlength, 0, (struct sockaddr*)&client, len) == -1)
                  {
                      //perror("Serial2telnet Error.\n");
                      //printf("terminating current client_connection...\n");
                      close(s_fd);          
                      //pthread_exit(NULL);
                      //return FALSE;
                  }
                  

                  //2.4清空这个数据包，并把剩下的数据放入这个空数据包中
                  memset(&data_buff, 0, sizeof(data_buff));
                  printf("tmp_buff_len-space:[%d]\n", tmp_buff_len-space);

                  if(sendto(s_fd , tmp_buff+space, tmp_buff_len-space, 0, (struct sockaddr*)&client, len) == -1)
                  {
                      //perror("Serial2telnet Error.\n");
                      //printf("terminating current client_connection...\n");
                      close(s_fd);          
                      //pthread_exit(NULL);
                      //return FALSE;
                  }
                  //memset(&tmp_buff, 0, sizeof(tmp_buff));
                  //memset(&data_buff, 0, sizeof(data_buff));
              //2.5重新开始计时
              tv.tv_usec=packettime*1000;
              total_len = 0;
            } 

         memset(&tmp_buff, 0, sizeof(tmp_buff));

        }

        //3.组包超时
        else
        {
          //printf("Arrive at timeout:%ds\n",_uart1_timeout);
          //3.1直接发送当前没有组满的数据包        
            if (total_len > 0)
            {
              if(sendto(s_fd , data_buff , total_len, 0, (struct sockaddr*)&client, len) < 0)
              {
                  close(s_fd);          
              }  
         
            }

          memset(&data_buff, 0, sizeof(data_buff));
          memset(&tmp_buff, 0, sizeof(tmp_buff));
          //3.2重新开始计时
          tv.tv_usec=packettime*1000;
          total_len = 0;
        }
    }
}

int udp_server(struct TobeConfig *TobeConfig)
{
    //创建两个线程ID
    pthread_t udp_telnet2serial_s_id,udp_serial2tlnet_s_id;

    //local的变量
    struct sockaddr_in local;

    //IPV4  SOCK_DGRAM 数据报套接字（UDP协议） 
    if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    { 
    perror("*** Create Socket Failed:"); 
    return FALSE;
    }

    //本地服务端设置  
    bzero(&local, sizeof(local)); 
    local.sin_family = AF_INET; 
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    int port = atoi(TobeConfig->port);
    local.sin_port = htons(port);

    //绑定
    if(bind(socketfd, (struct sockaddr*)&local,sizeof(local)) < 0)
    {
        perror("bind");
        return FALSE;
    }

    printf(">>> Entering Udp Server Working Mode ...\n");

    //处理 串口发往UDP客户端函数 的线程函数
    if (pthread_create(&udp_serial2tlnet_s_id,NULL,&udp_serial2telnet_s,NULL) == -1)
    {
        perror("serial2telnet pthread create error.\n");
        return FALSE;
    }

    //处理 UDP服务器发往串口函数 的线程函数
    if (pthread_create(&udp_telnet2serial_s_id,NULL,&udp_telnet2serial_s,NULL) == -1)
    {
        perror("serial2telnet pthread create error.\n");
        return FALSE;
    }


    //阻塞式回收线程资源
    if(pthread_join(udp_serial2tlnet_s_id, NULL) == 0)
    {
        printf(">>> Recycle pthread [-Serial2telnet-]\n");
    }
    if(pthread_join(udp_telnet2serial_s_id, NULL) == 0)
    {
        printf(">>> Recycle pthread [-Serial2telnet-]\n");
    }
}


//接口
int CreateServer(struct TobeConfig *TobeConfig)
{
    packetlength = atoi(TobeConfig->packetlength);
    serialfd = TobeConfig->serial_fd;
    packettime = atoi(TobeConfig->packettime);

	//TCP的服务端
	if (strcmp(TobeConfig->protocol, "tcp") == 0)
	{
		if (tcp_server(TobeConfig) == FALSE)
		{
			printf("*** Something Wrong in Creating TCP Server\n");
            return FALSE;
		} 
	}
	else if (strcmp(TobeConfig->protocol, "udp") == 0)
	{
        
		if (udp_server(TobeConfig) == FALSE)
		{
			printf("*** Something Wrong in Creating UDP Server\n");
            return FALSE;
		} 
        
	}
}