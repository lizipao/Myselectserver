#include"TcpServer.hpp"
#include<thread>

bool Run = true;

void cmdThread()
{
	while (true)
	{
		char cmdbuff[256];
		scanf("%s", cmdbuff);
		if (0 == strcmp(cmdbuff, "exit")) {
			printf("退出\n");
			Run = false;
			break;
		}
		else {
			printf("不支持命令。\n");
		}
	}
}


int main() {
	TcpServer myserver;
	myserver.Initsocket();
	myserver.Bind(nullptr, 4567);
	myserver.Listen(5);
	myserver.Start(4);
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();
	while (Run)
	{
		myserver.OnRun();
		//printf("空闲时间处理其它业务..\n");
	}
	myserver.Close();
	return 0;
}
