#include<iostream>
#include<vector>
#include<map>
#include<mutex>
#include"CELLTimestamp.hpp"
#include"Message.hpp"
#include"CellTaskServer.hpp"
using namespace std;
#ifdef _WIN32
#define FD_SETSIZE      2506
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<unistd.h> //uni std
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif