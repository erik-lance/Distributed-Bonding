#include "Server.h"

Server::Server(std::string hostname, int portNum)
{
	this->host = hostname;
	this->port = portNum;
	H_binded = false;
	O_binded = false;

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
	if (bind(m_socket, (struct sockaddr *)&m_addr, sizeof(m_addr)) == -1)
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

	// Start clients by sending "START"
	std::string message = "STARTBOND";
	int sent = send(connected_clients[0], message.c_str(), message.size(), 0);
	sent = send(connected_clients[1], message.c_str(), message.size(), 0);

	startTime = std::chrono::system_clock::now(); // Start Timing
	start();
}

Server::~Server()
{
}

void Server::start()
{
	isRunning = true;
	std::cout << "Server started" << std::endl;
	// Start the threads
	m_listenThread = std::thread(&Server::listener, this);
	m_processorThread = std::thread(&Server::processor, this);
	m_sendThread = std::thread(&Server::notify_clients, this);

	// Bonding done on main thread
	bonding();

	// Wait for the listener thread to finish
	m_listenThread.join();
	m_processorThread.join();
	m_sendThread.join();
}

void Server::listener()
{
	int MAX_BUFFER = 256;
	std::vector<char> buffer(MAX_BUFFER);
	std::vector<std::string> fragments = std::vector<std::string>(2, "");
	while (isRunning)
	{
		// Wait for a connection

		for (int i = 0; i < connected_clients.size(); i++)
		{
			SOCKET server_socket = connected_clients[i];

			if (socket_done[i])
			{
				continue;
			}

			int bytes_received = recv(server_socket, buffer.data(), buffer.size(), 0);

			if (bytes_received < 0)
			{
#ifdef _WIN32
				int error_code = WSAGetLastError();
				if (error_code != WSAEWOULDBLOCK)
				{
					char error[1024];
					strerror_s(error, sizeof(error), error_code);
					std::cerr << "[" << error_code << "] Error receiving message: " << error << std::endl;
					exit(1);
				}
				else
				{
					continue;
				}
#else
				if (errno != EWOULDBLOCK && errno != EAGAIN)
				{
					char error[1024];
					strerror_r(errno, error, sizeof(error));
					std::cerr << "Error receiving message: " << error << std::endl;
					exit(1);
				}
#endif
			}

			// Add message to queue

			// Check if message is fragmented or not by looking
			// if there is a null terminator in the message
			// and split the message if necessary
			std::string message = std::string(buffer.data(), bytes_received);

			// If fragment is not empty, append the message to the fragment
			if (fragments[i].size() > 0)
			{
				message = fragments[i] + message;
			}

			// Remembers the socket for the atoms
			// the first message will usually never be fragmented
			if (!H_binded && message[0] == 'H')
			{
				m_Hydrogen = server_socket;
				H_binded = true;
			}
			else if (!O_binded && message[0] == 'O')
			{
				m_Oxygen = server_socket;
				O_binded = true;
			}

			// If null terminator is found, split the message
			if (message.find('\0') != std::string::npos)
			{
				// Based on buffer size, could receive a message such as
				// H0 Request\0H2 Request\0H3 Request\0
				// So we split the message into individual messages
				std::vector<std::string> split_messages;

				// Split all the messages with null terminators to split_messages
				while (message.find('\0') != std::string::npos)
				{
					std::string split = message.substr(0, message.find('\0'));
					split_messages.push_back(split);
					message = message.substr(message.find('\0') + 1); // Remove the message from the buffer
				}

				// Push
				for (std::string msg : split_messages)
				{
					// Received "Done" message
					if (msg[0] != 'H' && msg[0] != 'O')
					{
						socket_done[i] = true;
					}
					else
					{
						std::cout << msg << std::endl;
						receivedMolecule[i]++;
					}

					mtx.lock();
					message_queue.push(msg);
					mtx.unlock();
				}

				fragments[i] = message;
			}
			else
			{
				std::cout << "Buffer without null terminator: " << message << std::endl;
			}
		}
	}
}

void Server::processor()
{
	while (isRunning)
	{
		if (!message_queue.empty())
		{
			mtx.lock();
			std::string message = message_queue.front();
			message_queue.pop();
			mtx.unlock();

			// Message is in the format "M#, request, timestamp"
			// e.g.: "H5, request, 2024-03-08 12:00:00"
			message = message.substr(0, message.find(", request"));

			// Process the message
			if (message[0] == 'H')
			{
				hydrogen.push(message);
			}
			else if (message[0] == 'O')
			{
				oxygen.push(message);
			}
			else
			{
				std::cerr << "Invalid message received: " << message << std::endl;
			}
		}
	}
}

void Server::bonding()
{
	while (isRunning)
	{
		if (hydrogen.size() >= 2 && !oxygen.empty())
		{

			// Timestamp
			std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
			std::time_t timestamp = std::chrono::system_clock::to_time_t(current_time);
			struct tm local_time;
			localtime_s(&local_time, &timestamp);

			char time_str[20]; // Enough space to hold "YYYY-MM-DD HH:MM:SS\0"
			strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_time);

			std::string h1 = hydrogen.front();
			std::string h1_msg = h1 + ", bonded, " + time_str;
			hydrogen.pop();

			std::string h2 = hydrogen.front();
			std::string h2_msg = h2 + ", bonded, " + time_str;
			hydrogen.pop();

			std::string o = oxygen.front();
			std::string o_msg = o + ", bonded, " + time_str;
			oxygen.pop();

			std::string bonded_molecule = h1 + "-" + o + "-" + h2 + " bonded";
			bonded_atoms.push(bonded_molecule);

			// sends message to client
			std::cout << h1_msg << std::endl;
			std::cout << h2_msg << std::endl;
			std::cout << o_msg << std::endl;

			send_mtx.lock();
			send_queue.push(h1_msg);
			send_queue.push(h2_msg);
			send_queue.push(o_msg);
			send_mtx.unlock();
		}
	}
}

void Server::notify_clients()
{
	while (isRunning)
	{
		if (!send_queue.empty())
		{
			send_mtx.lock();
			std::string message = send_queue.front();
			send_queue.pop();
			send_mtx.unlock();

			if (message[0] == 'H')
			{
				int sent = send(m_Hydrogen, message.c_str(), message.size(), 0);
			}
			else if (message[0] == 'O')
			{
				int sent = send(m_Oxygen, message.c_str(), message.size(), 0);
				H2O_bonded++;
			}
			else
			{
				std::cerr << "Invalid message to send: " << message << std::endl;
			}
		}

		// If both clients are done, then we are done
		if (socket_done[0] && socket_done[1] && H2O_bonded == abs(receivedMolecule[0] - receivedMolecule[1]))
		{
			isRunning = false;
			std::string message = "Done";

			// Time taken
			std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
			std::chrono::duration<double> time_taken = endTime - startTime;
			std::cout << "Time taken: " << time_taken.count() << "s" << std::endl;

			int sent = send(m_Hydrogen, message.c_str(), message.size(), 0);
			sent = send(m_Oxygen, message.c_str(), message.size(), 0);

			std::cout << "Bonded atoms:\n";
			while (!bonded_atoms.empty())
			{
				std::cout << bonded_atoms.front() << std::endl;
				bonded_atoms.pop();
			}
			std::cout << "Unbonded hydrogen atoms:\n";
			while (!hydrogen.empty())
			{
				std::cout << hydrogen.front() << std::endl;
				hydrogen.pop();
			}

			std::cout << "Unbonded oxygen atoms:\n";
			while (!oxygen.empty())
			{
				std::cout << oxygen.front() << std::endl;
				oxygen.pop();
			}

			
		}
	}
}
