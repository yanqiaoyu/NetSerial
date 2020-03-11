#include "main.h"
#include "serial.h"
#include "server.h"
#include "client.h"

int main(int argc, char const *argv[])
{
    //文件锁，防止被重复启动
    
    //防止程序重复启动
    AvoidMultiRun();

    //存放配置的结构体
    struct TobeConfig TobeConfig;
    memset(&TobeConfig, 0, sizeof(TobeConfig));

    //获取配置文件
    Init(&TobeConfig);

    //打开串口
    Serial(&TobeConfig);

	//如果是客户端模式
	if (strncmp(TobeConfig.mode, "client", 6) == 0
		||	strncmp(TobeConfig.mode, "CLIENT", 6) == 0 )
	{
		//创建主副客户端
		
        while(1)
		{
			//标志位置1,说明是主连接
			TobeConfig.clientflag = 1;
			CreateMainClient(&TobeConfig);
			
			//RELINK_IN(reconn.time);

			//标志位置0,说明书副连接
			TobeConfig.clientflag = 0;
			CreateBackupClient(&TobeConfig);
			//RELINK_IN(reconn.time);
		}
        
	}
	//如果是服务端模式
	else if (strncmp(TobeConfig.mode, "server", 6) == 0
		||	strncmp(TobeConfig.mode, "SERVER", 6) == 0)
	{
		while(1)
		{
            CreateServer(&TobeConfig);
		}
	}
	//出错
	else
	{
		printf("*** Something wrong when choosing mode\n");
	}

    return 0;
}
