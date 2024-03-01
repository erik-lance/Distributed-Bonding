#include "Server.h"

Server::Server(std::string hostname, int portNum)
{
	this->host = hostname;
	this->port = portNum;

	// Setup Winsock
	#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			std::cerr << "Error initializing Winsock" << std::endl;
			exit(1);
		}
	#endif

	// Create a socket
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == -1)
	{
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Fill in the address structure
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	InetPtonA(AF_INET, host.c_str(), &m_addr.sin_addr);

	// Bind the socket
	if (bind(m_socket, (struct sockaddr*)&m_addr, sizeof(m_addr)) == -1)
	{
		std::cerr << "Error binding socket" << std::endl;
		exit(1);
	}

	// Listen on the socket
	if (listen(m_socket, 2) == -1)
	{
		std::cerr << "Error listening on socket" << std::endl;
		exit(1);
	}

	// Accept two connections
	connected_clients.push_back(accept(m_socket, NULL, NULL));
	connected_clients.push_back(accept(m_socket, NULL, NULL));
}

Server::~Server()
{
}

void Server::start()
{
}

void Server::listener()
{
}
