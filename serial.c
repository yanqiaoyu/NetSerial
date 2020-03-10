#include "serial.h"

int serial_open(int fd, char* SERIAL_PATH)
{
  //打开串口，错误则返回
  if ((fd = open(SERIAL_PATH, O_RDWR|O_NOCTTY|O_NDELAY )) < 0)
  {
    perror("*** can't open serial! ");
    return -1;
  }
  else 
  {
  	printf(">>> open dev successful !\n");
  }

  //判断串口是否阻塞
  if (fcntl(fd ,  F_SETFL , 0) < 0)
  {
    printf("*** fcntl failed! ---\n");
    return -1;
  }
  else
  {
    //printf("--- dev is not dump ! ---\n");
  }

  //测试是否为终端设备
  if ( 0 == isatty(STDIN_FILENO))
  {
    printf("*** this is not a terminal device ! ---\n");
  }
  else
  {
    //printf("--- this is a terminal device ! ---\n");
  }

  return fd;
}

int serial_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{	
  /*
  	tcgetattr(fd,&options)得到与fd指向对象的相关参数，
  	并将它们保存于options,该函数,还可以测试配置是否正确，该串口是否可用等。
  	若调用成功，函数返回值为0，若调用失败，函数返回值为1.
  */

	struct termios options; 
    if( tcgetattr( fd,&options)  !=  0)
    	{
        	perror("SetupSerial 1");    
            return -1; 
    	}

    //选择波特率 
    switch(speed){
        case 1200:  
            cfsetispeed(&options, B1200);  
            cfsetospeed(&options, B1200);
            printf(">>> you choose baudrate:1200\n");
            break;   
        case 2400:  
            cfsetispeed(&options, B2400);  
            cfsetospeed(&options, B2400);
            printf(">>> you choose baudrate:2400\n");
            break;      
        case 4800:  
            cfsetispeed(&options, B4800);  
            cfsetospeed(&options, B4800);
            printf(">>> you choose baudrate:4800\n");
            break;        
        case 9600:  
            cfsetispeed(&options, B9600);  
            cfsetospeed(&options, B9600);
            printf(">>> you choose baudrate:9600\n");
            break;       
        case 19200:  
            cfsetispeed(&options, B19200);  
            cfsetospeed(&options, B19200);
            printf(">>> you choose baudrate:19200\n");
            break;   
        case 38400:  
            cfsetispeed(&options, B38400);  
            cfsetospeed(&options, B38400);
            printf(">>> you choose baudrate:38400\n");
            break;   
        case 57600:  
            cfsetispeed(&options, B57600);  
            cfsetospeed(&options, B57600);
            printf(">>> you choose baudrate:57600\n");
            break;   
        case 115200:  
            cfsetispeed(&options, B115200);  
            cfsetospeed(&options, B115200);
            printf(">>> you choose baudrate:115200\n");
            break; 
        case 230400:  
            cfsetispeed(&options, B230400);  
            cfsetospeed(&options, B230400);
            printf(">>> you choose baudrate:230400\n");
            break;   
        default:  
            cfsetispeed(&options, B57600);  
            cfsetospeed(&options, B57600);
            printf(">>> you choose baudrate:57600\n");
            break;   
    } 
     

    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;
    //设置数据流控制
    switch(flow_ctrl)
    {
    	case 0 ://不使用流控制
              options.c_cflag &= ~CRTSCTS;
              break;        
        case 1 ://使用硬件流控制
              options.c_cflag |= CRTSCTS;
              break;
        case 2 ://使用软件流控制
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
    }

    //设置数据位
    options.c_cflag &= ~CSIZE; //屏蔽其他标志位  
    switch (databits)
    {  
        case 5:
            options.c_cflag |= CS5;
        break;

        case 6:
			      options.c_cflag |= CS6;
        break;

        case 7    :    
            options.c_cflag |= CS7;
        break;

        case 8:    
            options.c_cflag |= CS8;
		    break;  

        default:   
            fprintf(stderr,"Unsupported data size\n");
        return (-1); 
    }
    //设置校验位
    switch (parity)
    {  
        case 'n':
        case 'N': //无奇偶校验位。
                 options.c_cflag &= ~PARENB; 
                 options.c_iflag &= ~INPCK;    
        break; 

        case 'o':  
        case 'O'://设置为奇校验    
                 options.c_cflag |= (PARODD | PARENB); 
                 options.c_iflag |= INPCK;             
                 break; 

        case 'e': 
        case 'E'://设置为偶校验  
                 options.c_cflag |= PARENB;       
                 options.c_cflag &= ~PARODD;       
                 options.c_iflag |= INPCK;       
                 break;

       case 's':
       case 'S': //设置为空格 
                 options.c_cflag &= ~PARENB;
                 options.c_cflag &= ~CSTOPB;
                 break; 

        default:  
                 fprintf(stderr,"Unsupported parity\n");   
                 return (-1); 
    } 

    // 设置停止位 
    switch (stopbits)
    {  
       case 1:   
                options.c_cflag &= ~CSTOPB; 
                 break; 
       case 2:   
                options.c_cflag |= CSTOPB; 
                         break;
       default:   
                fprintf(stderr,"Unsupported stop bits/n"); 
                return (-1);

    }
  
    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 0; /* 读取一个字符等待1*(1/10)s */  
    options.c_cc[VMIN] = 0; /* 读取字符的最少个数为1 */

    //处理Linux系统会舍弃0x03 0x13的问题
    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //如果发生数据溢出，接收数据，但是不再读取
    tcflush(fd,TCIFLUSH);

    //**********************************
        /* !!! 串口的关键设定 !!! */

        //这是串口的原始输入模式，原始输入模式是没有处理过的,当接收数据时，输入的字符在它们被接收后立即被传送
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        //这是串口的规范模式，输入字符被放入用于和用户交互可以编辑的缓冲区内，直接到读入回车或者换行符号时才结束
        //options.c_lflag &= (ICANON | ECHO | ECHOE);

    //**********************************


    //激活配置(将修改后的termios数据设置到串口中)
    if (tcsetattr(fd,TCSANOW,&options) != 0)  
    {
        perror("com set error!/n");  
		return (-1); 
    }
    return (0);  
}

int Serial(struct TobeConfig *TobeConfig)
{
    //定义串口描述符并打开串口
    TobeConfig->serial_fd = serial_open(TobeConfig->serial_fd, TobeConfig->serial_path);

    int baud = atoi(TobeConfig->baud);
    int data = atoi(TobeConfig->data);
    int stop = atoi(TobeConfig->stop);
    char parity = TobeConfig->parity[0];

    //设置串口参数
    printf("%c\n", parity);
    int ret = serial_set(TobeConfig->serial_fd, baud, 0, data, stop, parity);
    if (ret == 0)
    {
        printf(">>> serial set successful!...\n");
    }
    else
    {
        printf("*** fail to set serial! ***\n");
        return -1;
    }
    return 0;
}

