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
			printf("�˳�\n");
			Run = false;
			break;
		}
		else {
			printf("��֧�����\n");
		}
	}
}


int main() {
	TcpServer myserver;
	myserver.Initsocket();
	myserver.Bind(nullptr, 4567);
	myserver.Listen(5);
	myserver.Start(4);
	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();
	while (Run)
	{
		myserver.OnRun();
		//printf("����ʱ�䴦������ҵ��..\n");
	}
	myserver.Close();
	return 0;
}
