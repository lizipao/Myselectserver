#ifndef _CellServer_hpp_
#define _CellServer_hpp_
#include"head.h"
#include"ClientSocket.hpp"

class CellServer
{
public:
	CellServer(SOCKET sock) {
		_sock = sock;
	};
	~CellServer() {

	};
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
		//将收取到的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
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
			for (auto iter= _clients.begin();iter!= _clients.end();)
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
};



#endif