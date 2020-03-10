#include "main.h"
#include "serial.h"
#include "server.h"

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
	if (strcmp(TobeConfig.mode, "client") == 0)
	{
		//获取主中心与副中心的端口号、地址
		//get_client_opt();
		//获取登录所需要的配置
		//get_login_opt();
		//获取心跳所需要的配置
		//get_heartbeat_opt();
		//获取应答时间
		//get_acktime_opt();
		//获取重连时间
		//get_reconntime_opt();

		//创建主副客户端
		/*
        while(1)
		{
			create_main_client();

			RELINK_IN(reconn.time);

			create_assist_client();

			RELINK_IN(reconn.time);
		}
        */
	}
	//如果是服务端模式
	else if (strcmp(TobeConfig.mode, "server") == 0)
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
