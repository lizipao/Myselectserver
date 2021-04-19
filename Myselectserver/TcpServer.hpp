#include"head.h"
#include"CellServer.hpp"
class TcpServer
{
public:
	TcpServer() {
		_sock = INVALID_SOCKET;
	};
	~TcpServer() {
		printf("����");
	};

	SOCKET Initsocket() {
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sock == INVALID_SOCKET) {
			cout << "����socketʧ��" << endl;
		}
		else {
			cout << "����socket" << _sock << "�ɹ�" << endl;
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
			printf("����,������˿�<%d>ʧ��...\n", port);
		}
		else {
			printf("������˿�<%d>�ɹ�...\n", port);
		}
		return ret;
	}

	int Listen(int n) {
		int ret = listen(_sock, n);//n��ʾ�����Ҫ�ȴ�������������
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>����,��������˿�ʧ��...\n", _sock);
		}
		else {
			
			printf("socket=<%d>��������˿ڳɹ�...\n", _sock);
		}
		return ret;
	}

	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		//printf("client<%d> join\n", pClient->sockfd());
	}


	void addClientToCellServer(ClientSocket* pClient)
	{
		//���ҿͻ��������ٵ�CellServer��Ϣ�������
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
			printf("socket=<%d>����,���ܵ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			//���¿ͻ��˷�����ͻ��������ٵ�cellServer
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
			ser->ThreadStart();
		}
	}
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			//�ر��׽���closesocket
			closesocket(_sock);
			//------------
			//���Windows socket����
			WSACleanup();
#else
			//�ر��׽���closesocket
			close(_sock);
#endif
		}
	}
		bool OnRun()
	{
		if (isRun())
		{
			//�������׽��� BSD socket
			
			//������
			FD_ZERO(&fdRead);
			//����������socket�����뼯��
			FD_SET(_sock, &fdRead);
			///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
			///���������ļ����������ֵ+1 ��Windows�������������д0
			timeval t = { 0,10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
			if (ret < 0)
			{
				printf("Accept Select���������\n");
				Close();
				return false;
			}
			//�ж���������socket���Ƿ��ڼ�����
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
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
private:
	SOCKET _sock;
	vector<CellServer*> _cellServers;
	fd_set fdRead;
	int _clientCount;
};


