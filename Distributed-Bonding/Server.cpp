#include "Server.h"

Server::Server(std::string hostname, int portNum)
{
	this->host = hostname;
	this->port = portNum;
	H_binded = false;
	O_binded = false;
	H20_bonded = 0;
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
	std::cout << "Server waiting for 2 connections" << std::endl;
	connected_clients.push_back(accept(m_socket, NULL, NULL));

	std::cout << "Server waiting for 1 more connection" << std::endl;
	connected_clients.push_back(accept(m_socket, NULL, NULL));
}

Server::~Server()
{
}

void Server::start()
{
	isRunning = true;
	std::cout << "Server started" << std::endl;
	// Start the listener thread
	m_listenThread = std::thread(&Server::listener, this);
	m_processorThread = std::thread(&Server::processor, this);

	// Wait for the listener thread to finish
	m_listenThread.join();
	m_processorThread.join();
}

void Server::listener()
{
	int MAX_BUFFER = 1024;
	std::vector<char> buffer(MAX_BUFFER);
	while (isRunning)
	{
		// Wait for a connection

		for (int i = 0; i < connected_clients.size(); i++) {
			SOCKET server_socket = connected_clients[i];

			if (socket_done[i]) { continue; }

			//clear the buffer
			buffer.clear();
			buffer.resize(MAX_BUFFER);

			int bytes_received = recv(server_socket, buffer.data(), buffer.size(), 0);

			if (bytes_received < 0) {
				#ifdef _WIN32
					int error_code = WSAGetLastError();
					if (error_code != WSAEWOULDBLOCK) {
						char error[1024];
						strerror_s(error, sizeof(error), error_code);
						std::cerr << "[" << error_code << "] Error receiving message: " << error << std::endl;
						exit(1);
					}
					else {
						continue;
					}
				#else
					if (errno != EWOULDBLOCK && errno != EAGAIN) {
						char error[1024];
						strerror_r(errno, error, sizeof(error));
						std::cerr << "Error receiving message: " << error << std::endl;
						exit(1);
					}
				#endif
			}
			
			// Hydrogen molecule: "H:#Request Number" (e.g. "H: 1")
			// Oxygen molecule: "O:#Request Number" (e.g. "O: 1")

			//Add message to queue
			std::string message = std::string(buffer.data(), bytes_received);

			//Remembers the socket for the molecules
			if (H_binded && message[0] == 'H') {
				m_Hydrogen = server_socket;
				H_binded = true;
			}
			else if (O_binded && message[0] == 'O') {
				m_Oxygen = server_socket;
				O_binded = true;
			}


			message_queue.push(message);

		}
	}
}

void Server::processor()
{
	while (isRunning) {
		if (!message_queue.empty())
		{
			std::string message = message_queue.front();
			message_queue.pop();

			// Message is in the format "Hydrogen:1 Request" or "Oxygen:1 Request" Removes " Request"
			message.erase(message.find(" Request"));

			// Process the message
			if (message[0] == 'H') {
				std::cout << message << " Requesting bond" << std::endl;
				hydrogen.push(message);
			}
			else if (message[0] == 'O') {
				std::cout << message << " Requesting bond" << std::endl;
				oxygen.push(message);
			}
			else {
				std::cerr << "Invalid message received: " << message << std::endl;
			}
		}
	}
}

void Server::bonding(){
	while (isRunning) {
		if (hydrogen.size() >= 2 && !oxygen.empty()) {

			std::string h1 = hydrogen.front();
			hydrogen.pop();
			std::string h2 = hydrogen.front();
			hydrogen.pop();

			std::string o = oxygen.front();
			oxygen.pop();

			std::cout << h1 << "bonded" << std::endl;
			std::cout << h2 << "bonded" << std::endl;
			std::cout << o << "bonded" << std::endl;
			H20_bonded++;

			//sends message to client
			send_queue.push(h1);
			send_queue.push(h2);
			send_queue.push(o);
		}
	}
}

void Server::notify_clients()
{
	while (isRunning) {
		if (!send_queue.empty()) {
			std::string message = send_queue.front();
			send_queue.pop();

			if (message[0] == 'H') {
				int sent = send(m_Hydrogen, message.c_str(), message.size() + 1, 0);
				
			}
			else if (message[0] == 'O') {
				int sent = send(m_Oxygen, message.c_str(), message.size() + 1, 0);
			}
			else {
				std::cerr << "Invalid message to send: " << message << std::endl;
			}
		}
	}
}
