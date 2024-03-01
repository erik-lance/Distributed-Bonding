#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include <vector>

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

class Client {
public:
	Client(int m_type);
	~Client();
private:
	SOCKET m_socket;
	sockaddr_in m_addr;

	// Timestamps
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;
	int molecules = 0;

	bool isRunning = false;
	void init();
	void prepareMolecules(int type);
};
