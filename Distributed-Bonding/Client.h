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

class Client
{
public:
	Client(int m_type);
	~Client();

	void init(std::string host, int port);
	void run();

private:
	SOCKET m_socket;
	sockaddr_in m_addr;

	// Timestamps
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::time_point end;

	// Atoms
	int atoms = 0;
	bool isHydrogen = false;

	// Sanity check
	std::vector<bool> bonded_atoms;

	bool isRunning = false;
	std::thread m_thread;

	std::mutex mtx;
	std::condition_variable cv;
	bool isReady = false;

	void prepareAtoms(int type);
	void listener();
};
