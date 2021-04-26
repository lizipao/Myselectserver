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
		}//���� ��Ϣ---���� ����   ������ ���ݻ�����  ������ 
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
		}
		break;
		default:
		{
			printf("<socket=%d>�յ�δ������Ϣ,���ݳ��ȣ�%d\n", pClient->_sock, header->dataLength);
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
