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

		//���տͻ�������
		char* szRecv = pClient->_szMsgBuf + pClient->_lastPos;
		int nLen = (int)recv(pClient->_sock, szRecv, (RECV_BUFF_SZIE)-pClient->_lastPos, 0);
		//printf("nLen=%d\n", nLen);
		if (nLen <= 0)
		{
			//printf("�ͻ���<Socket=%d>���˳������������\n", pClient->sockfd());
			return -1;
		}
		//����ȡ�������ݿ�������Ϣ������
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//��Ϣ������������β��λ�ú���
		pClient->setLastPos(pClient->_lastPos + nLen);

		//�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
		while (pClient->_lastPos >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
			DataHeader* header = (DataHeader*)pClient->_szMsgBuf;
			//�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
			if (pClient->_lastPos >= header->dataLength)
			{
				//��Ϣ������ʣ��δ�������ݵĳ���
				int nSize = pClient->_lastPos - header->dataLength;

				//����Ϣ������ʣ��δ��������ǰ��
				memcpy(pClient->_szMsgBuf, pClient->_szMsgBuf + header->dataLength, nSize);
				//��Ϣ������������β��λ��ǰ��
				pClient->setLastPos(nSize);
			}
			else {
				//��Ϣ������ʣ�����ݲ���һ��������Ϣ
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
			//�ر��׽���closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//�ر��׽���closesocket
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
			{//�ӻ��������ȡ���ͻ�����
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->_sock] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//�������׽��� BSD socket
			fd_set fdRead;//��������socket�� ����
			//������
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//����������socket�����뼯��
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

			///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
			///���������ļ����������ֵ+1 ��Windows�������������д0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select���������\n");
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