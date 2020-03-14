#include "main.h"

void* my_shell(const char *fmt, ...)  
{  
	//存储需要执行的shell命令的缓存
    char shell_buf[SHELL_BUFF_LEN];
    char tmp_buf[SHELL_BUFF_LEN];
    memset(&shell_buf, 0, sizeof(shell_buf));
    memset(&tmp_buf, 0, sizeof(tmp_buf));

    //将传进来的参数放到shell_buf中
    va_list args;   
    va_start(args, fmt);  
    vsnprintf(shell_buf, SHELL_BUFF_LEN, fmt, args);

    //必须调用的一步  
    va_end(args);

	//定义读取shell命令的文件指针
	FILE * fp;
	fp = popen(shell_buf, "r");
	fread(tmp_buf, sizeof(tmp_buf), 1, fp);

	//动态分配一块内存给shell命令的返回值
	char* ret_str = (char *) malloc(strlen(tmp_buf)+1);
	//char* ret_str;
	//ret_str = (char *) malloc(strlen(tmp_buf)+1);
	if (!ret_str)
	{
		perror("malloc");
		return NULL;
	}
	memset(ret_str, 0, strlen(tmp_buf)+1);

	//将读到的返回值放入分配的内存中
	strncpy(ret_str, tmp_buf, strlen(tmp_buf));

	pclose(fp);
    return ret_str;  
}

//通过文件锁防止程序多次运行
void AvoidMultiRun(int LOCK_FILE)
{
    //获得当前有多少个LDS_Uart1在启动
    char* var = NULL;
    var = my_shell("echo -n `ps -A | grep 'NetSerial' | grep -v grep |wc -l`");
    int Uart1_num = atoi(var);
    free(var);
    var = NULL;
    //如果这个数量大于1
    if (Uart1_num > 1)
    {
        printf(">>> Detect too many NetSerial, exiting ...\n");
        exit(0);
    }
    //否则，证明程序是第一次启动
    else
    {
        printf(">>> There is only one NetSerial, lauching ...\n");
    }
}

//启动时进行配置初始化
void Init(struct TobeConfig *TobeConfig)
{
	//收集串口配置
    GetProfileString(CONFIG_PATH, "SerialPort", "baudrate", TobeConfig->baud);
    GetProfileString(CONFIG_PATH, "SerialPort", "data", TobeConfig->data);
    GetProfileString(CONFIG_PATH, "SerialPort", "parity", TobeConfig->parity);
    GetProfileString(CONFIG_PATH, "SerialPort", "stop", TobeConfig->stop);
    GetProfileString(CONFIG_PATH, "SerialPort", "serial_path", TobeConfig->serial_path);

    printf("SerialConf:%s %s %s %s %s\n", TobeConfig->baud, TobeConfig->data, TobeConfig->parity, TobeConfig->stop, TobeConfig->serial_path);

	//收集网络配置
	GetProfileString(CONFIG_PATH, "NetWork", "mode", TobeConfig->mode);
	GetProfileString(CONFIG_PATH, "NetWork", "port", TobeConfig->port);
	GetProfileString(CONFIG_PATH, "NetWork", "protocol", TobeConfig->protocol);
	GetProfileString(CONFIG_PATH, "NetWork", "IP", TobeConfig->serverIP);

	GetProfileString(CONFIG_PATH, "NetWork", "bk_port", TobeConfig->bk_port);
	GetProfileString(CONFIG_PATH, "NetWork", "bk_protocol", TobeConfig->bk_protocol);
	GetProfileString(CONFIG_PATH, "NetWork", "bk_IP", TobeConfig->bk_IP);

	GetProfileString(CONFIG_PATH, "NetWork", "timeout", TobeConfig->timeout);
	GetProfileString(CONFIG_PATH, "NetWork", "reconnecttime", TobeConfig->reconnecttime);

	GetProfileString(CONFIG_PATH, "NetWork", "heartbeattime", TobeConfig->heartbeattime);
	GetProfileString(CONFIG_PATH, "NetWork", "heartbeatbuf", TobeConfig->heartbeatbuf);	

	printf("NetworkConf:%s %s %s %s\n", TobeConfig->mode, TobeConfig->port, TobeConfig->protocol, TobeConfig->serverIP);
	printf("NetworkConf:%s %s %s\n", TobeConfig->bk_port, TobeConfig->bk_protocol, TobeConfig->bk_IP);


	//收集数据配置
	GetProfileString(CONFIG_PATH, "Data", "packetlength", TobeConfig->packetlength);
	GetProfileString(CONFIG_PATH, "Data", "packettime", TobeConfig->packettime);

	printf("DataConf:%s %s \n",TobeConfig->packetlength, TobeConfig->packettime);
}

//删除左边的空格
static char * l_trim(char * szOutput, const char *szInput)
{
	assert(szInput != NULL);
	assert(szOutput != NULL);
	assert(szOutput != szInput);
 
	for(; *szInput != '\0' && isspace(*szInput); ++szInput)
	{
		;
	}
 
	return strcpy(szOutput, szInput);
}

//删除右边的空格
static char *r_trim(char *szOutput, const char *szInput)
{
	char *p = NULL;
 
	assert(szInput != NULL);
	assert(szOutput != NULL);
	assert(szOutput != szInput);
	strcpy(szOutput, szInput);
 
	for(p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p)
	{
		;
	}
 
	*(++p) = '\0';
 
	return szOutput;
}

//删除两边的空格
static char * a_trim(char * szOutput, const char * szInput)
{
	char *p = NULL;
 
	assert(szInput != NULL);
	assert(szOutput != NULL);
 
	l_trim(szOutput, szInput);
 
	for(p = szOutput + strlen(szOutput) - 1;p >= szOutput && isspace(*p); --p)
	{
		;
	}
 
	*(++p) = '\0';
 
	return szOutput;
}

//提供调用的获取配置文件的接口
int GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal)
{
	char appname[32],keyname[32];
	char *buf,*c;
	char buf_i[KEYVALLEN], buf_o[KEYVALLEN];
 
	FILE *fp;
	int found=0; /* 1 AppName 2 KeyName */
 
	if( (fp=fopen( profile,"r" ))==NULL )
	{
		printf( "openfile [%s] error [%s]\n",profile,strerror(errno) );
		return(-1);
	}
 
	fseek( fp, 0, SEEK_SET );
	memset( appname, 0, sizeof(appname) );
	sprintf( appname,"[%s]", AppName );
 
	while( !feof(fp) && fgets( buf_i, KEYVALLEN, fp )!=NULL )
	{
		l_trim(buf_o, buf_i);
 
		if( strlen(buf_o) <= 0 )
			continue;
 
		buf = NULL;
		buf = buf_o;
 
		if( found == 0 )
		{
			if( buf[0] != '[' ) 
			{
				continue;
			} 
			else if ( strncmp(buf,appname,strlen(appname))==0 )
			{
				found = 1;
				continue;
			}
		} 
		else if( found == 1 )
		{
			if( buf[0] == '#' )
			{
				continue;
			} 
			else if ( buf[0] == '[' ) 
			{
				break;
			} 
			else 
			{
				if( (c = (char*)strchr(buf, '=')) == NULL )
					continue;
 
				memset( keyname, 0, sizeof(keyname) );
				sscanf( buf, "%[^=|^ |^\t]", keyname );
 
				if( strcmp(keyname, KeyName) == 0 )
				{
					sscanf( ++c, "%[^\n]", KeyVal );
					char *KeyVal_o = (char *)malloc(strlen(KeyVal) + 1);
 
					if(KeyVal_o != NULL)
					{
						memset(KeyVal_o, 0, sizeof(KeyVal_o));
						a_trim(KeyVal_o, KeyVal);
						if(KeyVal_o && strlen(KeyVal_o) > 0)
							strcpy(KeyVal, KeyVal_o);
						free(KeyVal_o);
						KeyVal_o = NULL;
					}
 
					found = 2;
					break;
				} 
				else 
				{
					continue;
				}
			}
		}
	}
 
	fclose( fp );
 
	if( found == 2 )
		return(0);
 
	return(-1);
}

//自己重新复写了strncat，主要原因是既有的函数无法无差别复制串口过来的特殊字符
char * my_strncat(char *dst,const char *src,size_t count, int dst_len)  
{
    //判断传参是否合法
    if (NULL == dst || NULL == src)
    {
        return dst;
    }

    //定义一个指针指向目标地址
    char *cp = dst;

    //将指针指向地址末尾
    int i;
    for (i = 0; i < dst_len; ++i)
    {
      cp++;
    }

    //复制内容
    for (i = 0; i <= count; ++i)
    {
      *cp++ = *src++;
    }

    return ( dst );
}

void RELINK_IN(int reconn_time)
{
    while(reconn_time)
    {
        printf("Trying to reconnect in %d's\r", reconn_time);
        reconn_time--;
        fflush(stdout);
        printf("                            \r");
        sleep(1);
    }
}

