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
	Server(std::string hostname, int portNum);
	~Server();

	void start();
private:
	SOCKET m_socket;
	sockaddr_in m_addr;
	std::string host;
	int port;
	int H20_bonded;

	std::vector<SOCKET> connected_clients;
	std::vector<bool> socket_done = std::vector<bool>(2, false);

	// Threads
	std::thread m_listenThread;
	std::thread m_sendThread;
	std::thread m_processorThread;

	// Queues
	std::queue<std::string> message_queue;
	std::queue<std::string> send_queue;
	std::queue<std::string> hydrogen;
	std::queue<std::string> oxygen;

	bool isRunning = false;
	void listener();
	void processor();
	void bonding();
	void send();
};

