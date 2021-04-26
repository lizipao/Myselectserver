#include"head.h"

class ClientSocket
{
public:
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SZIE];
	//消息缓冲区的数据尾部位置
	int _lastPos;
	//第二缓冲区 发送缓冲区
	char _szSendBuf[SEND_BUFF_SZIE];
	//发送缓冲区的数据尾部位置
	int _lastSendPos;
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
	//发送数据
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SZIE)
			{
				//计算可拷贝的数据长度
				int nCopyLen = SEND_BUFF_SZIE - _lastSendPos;
				//拷贝数据
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//计算剩余数据位置
				pSendData += nCopyLen;
				//计算剩余数据长度
				nSendLen -= nSendLen;
				//发送数据
				ret = send(_sock, _szSendBuf, SEND_BUFF_SZIE, 0);
				//数据尾部位置清零
				_lastSendPos = 0;
				//发送错误
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else {
				//将要发送的数据 拷贝到发送缓冲区尾部
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//计算数据尾部位置
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}
private:


};
class CellServer;
//网络事件接口
class INetEvent
{
public:
	//纯虚函数
	//客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	//客户端离开事件
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	//客户端消息事件
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header) = 0;
	//recv事件
	virtual void OnNetRecv(ClientSocket* pClient) = 0;
private:

};
//网络消息发送任务
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
class CellServer
{
public:
	CellServer(SOCKET sock) {
		_sock = sock;
	};
	~CellServer() {

	};
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}
	void addSendTask(ClientSocket* pClient, DataHeader* header)
	{
		CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
		_taskServer.addTask(task);
	}
	int RecvData(ClientSocket* pClient) {

		//接收客户端数据
		char* szRecv = pClient->_szMsgBuf + pClient->_lastPos;
		int nLen = (int)recv(pClient->_sock, szRecv, (RECV_BUFF_SZIE)-pClient->_lastPos, 0);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
			return -1;
		}
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->_lastPos + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->_lastPos >= sizeof(DataHeader))
		{
			//这时就可以知道当前消息的长度
			DataHeader* header = (DataHeader*)pClient->_szMsgBuf;
			//判断消息缓冲区的数据长度大于消息长度
			if (pClient->_lastPos >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->_lastPos - header->dataLength;
				_pNetEvent->OnNetMsg(this, pClient, header);
				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->_szMsgBuf, pClient->_szMsgBuf + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else {
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.first);
				delete iter.second;
			}
			//关闭套节字closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//关闭套节字closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}

	void OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (!_clientsBuff.empty())
			{//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->_sock] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符（socket） 集合
			//清理集合
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//将描述符（socket）加入集合
				_maxSock = _clients.rbegin()->first;
				for (auto iter : _clients)
				{
					FD_SET(iter.first, &fdRead);
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return;
			}
			else if (ret == 0)
			{
				continue;
			}

#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						_clients_change = true;
						_clients.erase(iter);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}

			}
#else
			for (auto iter = _clients.begin(); iter != _clients.end();)
			{
				if (FD_ISSET(iter.first, &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						_clients_change = true;
						_clients.erase(iter++);
					}
					else {
						iter++;
					}
				}
			}

			/*for (auto iter : _clients)
			{
				if (FD_ISSET(iter.first, &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						_clients_change = true;
						_clients.erase(iter);
					}
				}
			}*/

#endif
		}
	}
	int getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void ThreadStart()
	{
		_thread = std::thread(&CellServer::OnRun, this);
		_thread.detach();
		_taskServer.Start();

	}
private:
	std::map<SOCKET, ClientSocket*> _clients;
	std::vector<ClientSocket*> _clientsBuff;
	mutex _mutex;
	thread _thread;
	bool _clients_change;
	SOCKET _sock;
	SOCKET _maxSock;
	fd_set _fdRead_bak;
	CellTaskServer _taskServer;
	//网络事件对象
	INetEvent* _pNetEvent;
};

class TcpServer : public INetEvent
{
public:
	TcpServer() {
		_sock = INVALID_SOCKET;
	};
	~TcpServer() {
		printf("完事");
	};

	SOCKET Initsocket() {
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sock == INVALID_SOCKET) {
			cout << "创建socket失败" << endl;
		}
		else {
			cout << "创建socket" << _sock << "成功" << endl;
		}
		return _sock;
	}

	int Bind(const char* ip, int port) {
		sockaddr_in _sin = {};
		_sin.sin_port = htons(port);
		_sin.sin_family = AF_INET;
#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin,sizeof(_sin));
		if (ret== SOCKET_ERROR ) {
			printf("错误,绑定网络端口<%d>失败...\n", port);
		}
		else {
			printf("绑定网络端口<%d>成功...\n", port);
		}
		return ret;
	}

	int Listen(int n) {
		int ret = listen(_sock, n);//n表示最大需要等待多少人来连接
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>错误,监听网络端口失败...\n", _sock);
		}
		else {
			
			printf("socket=<%d>监听网络端口成功...\n", _sock);
		}
		return ret;
	}

	void addClientToCellServer(ClientSocket* pClient)
	{
		//查找客户数量最少的CellServer消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		OnNetJoin(pClient);
	}

	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>错误,接受到无效客户端SOCKET...\n", (int)_sock);
		}
		else {
			//将新客户端分配给客户数量最少的cellServer
			addClientToCellServer(new ClientSocket(cSock));
		}
		return cSock;
	}

	void Start(int nCellServer)
	{
		for (int n = 0; n < nCellServer; n++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			ser->setEventObj(this);
			ser->ThreadStart();
		}
	}
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			//关闭套节字closesocket
			closesocket(_sock);
			//------------
			//清除Windows socket环境
			WSACleanup();
#else
			//关闭套节字closesocket
			close(_sock);
#endif
		}
	}
		bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			//伯克利套接字 BSD socket
			
			//清理集合
			FD_ZERO(&fdRead);
			//将描述符（socket）加入集合
			FD_SET(_sock, &fdRead);
			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			timeval t = { 0,10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
			if (ret < 0)
			{
				printf("Accept Select任务结束。\n");
				Close();
				return false;
			}
			//判断描述符（socket）是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//计算并输出每秒收到的网络消息
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n", _cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

	//只会被一个线程触发 安全
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		//printf("client<%d> join\n", pClient->sockfd());
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;
	}

	virtual void OnNetRecv(ClientSocket* pClient)
	{
		_recvCount++;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
private:
	SOCKET _sock;
	vector<CellServer*> _cellServers;
	fd_set fdRead;
	//SOCKET recv计数
	std::atomic_int _recvCount;
	//收到消息计数
	std::atomic_int _msgCount;
	//客户端计数
	std::atomic_int _clientCount;
	CELLTimestamp _tTime;
};