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

void Client::run()
{
	isRunning = true;
	
	std::cout << "Connecting to server..." << std::endl;

	// Connect to the server
	if (connect(m_socket, (struct sockaddr*)&m_addr, sizeof(m_addr)) == -1)
	{
		std::cerr << "Error connecting to server" << std::endl;
		exit(1);
	}

	std::cout << "Connected to server" << std::endl;
	std::cout << "Host: " << inet_ntoa(m_addr.sin_addr) << std::endl;
	std::cout << "Port: " << ntohs(m_addr.sin_port) << std::endl;

	// Start the listener thread
	m_thread = std::thread(&Client::listener, this);

	while (isRunning)
	{
		std::cout << "\nBegin requesting data from server" << std::endl;
		std::string molecule_type = isHydrogen ? "Hydrogen" : "Oxygen";


		std::cout << "Moleceule Type: " << molecule_type << std::endl;

		// Send molecules to the server
		for (int i = 0; i < molecules; i++) {
			std::string message = molecule_type[0] + (i+1)+ " Request";
			int sent = send(m_socket, message.c_str(), message.size() + 1, 0);

			if (sent < 0)
			{
				std::cerr << "Error sending message to server" << std::endl;
				exit(1);
			}
		}
			
	}

	// Join the thread
	m_thread.join();
}

/**
 * Prepares the molecules to be sent to the server
 * @param type 0 for hydrogen, 1 for oxygen
 */
void Client::prepareMolecules(int type)
{
	if (type == 0) {
		this->isHydrogen = true;
		molecules = 1000;
	}
	else {
		this->isHydrogen = false;
		molecules = 500;
	}
}

/**
 * Simply listens to requests from the server so that it can receive messages
 * as it is sending molecules concurrently.
 */
void Client::listener()
{
	while (isRunning) {
		char buffer[1024];
		memset(buffer, 0, 1024);

		int bytesReceived = recv(m_socket, buffer, 1024, 0);
		if (bytesReceived == -1)
		{
			std::cerr << "Error in recv(). Quitting" << std::endl;
			break;
		}

		if (bytesReceived == 0)
		{
			std::cout << "Server disconnected" << std::endl;
			break;
		}

		std::cout << std::string(buffer, bytesReceived) << " bonded" << std::endl;
	}
}
