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

class MyServer : public TcpServer
{
public:

	virtual void OnNetJoin(ClientSocket* pClient)
	{
		TcpServer::OnNetJoin(pClient);
	}
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		TcpServer::OnNetLeave(pClient);
	}
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header)
	{
		TcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			LoginResult* ret = new LoginResult();
			pCellServer->addSendTask(pClient, ret);
		}//接收 消息---处理 发送   生产者 数据缓冲区  消费者 
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
		}
		break;
		default:
		{
			printf("<socket=%d>收到未定义消息,数据长度：%d\n", pClient->_sock, header->dataLength);
		}
		break;
		}
	}
private:

};

int main() {
	MyServer myserver;
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
