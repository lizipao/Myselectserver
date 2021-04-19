#ifndef _ClientSocket_hpp_
#define _ClientSocket_hpp_
#include"head.h"
#define RECV_BUFF_SZIE 10240*5

class ClientSocket
{
public:
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SZIE];
	//消息缓冲区的数据尾部位置
	int _lastPos;
	SOCKET _sock;
	ClientSocket(SOCKET sock) {
		_sock = sock;
		memset(_szMsgBuf, 0, RECV_BUFF_SZIE);
		_lastPos = 0;
	};
	~ClientSocket() {
	};

	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

private:


};

class CellSendMsg2ClientTask :public CellTask
{
	ClientSocket* _pClient;
	DataHeader* _pHeader;
public:
	CellSendMsg2ClientTask(ClientSocket* pClient, DataHeader* header)
	{
		_pClient = pClient;
		_pHeader = header;
	}

	//执行任务
	void doTask()
	{
		_pClient->SendData(_pHeader);
		delete _pHeader;
	}
};
#endif