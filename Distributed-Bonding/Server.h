#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

class Server
{
public:
	Server();
	~Server();

	void init(std::string host, int port);
	void start();
private:
	SOCKET m_socket;
	sockaddr_in m_addr;

	// Threads
	std::thread m_listenThread;
	std::thread m_sendThread;

	// Queues
	std::queue<std::string> hydrogen;
	std::queue<std::string> oxygen;

	bool isRunning = false;
	void listener();
};

