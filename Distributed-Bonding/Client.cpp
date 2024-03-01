#include "Client.h"
#include "Client.h"

Client::Client(int type) {
	prepareMolecules(type);

	// Setup Winsock
	#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			std::cerr << "Error initializing Winsock" << std::endl;
			exit(1);
		}
	#endif
}

Client::~Client()
{
}

/**
 * Prepares the socket of server
 */
void Client::init(std::string host, int port)
{
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
}


void Client::prepareMolecules(int type)
{
}
