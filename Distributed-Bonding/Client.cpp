#include "Client.h"

Client::Client(int type)
{
	prepareAtoms(type);

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
	if (connect(m_socket, (struct sockaddr *)&m_addr, sizeof(m_addr)) == -1)
	{
		std::cerr << "Error connecting to server" << std::endl;
		exit(1);
	}

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(m_addr.sin_addr), ip, INET_ADDRSTRLEN);

	std::cout << "Connected to server" << std::endl;
	std::cout << "Host: " << ip << std::endl;
	std::cout << "Port: " << ntohs(m_addr.sin_port) << std::endl;

	// Start the listener thread
	m_thread = std::thread(&Client::listener, this);

	std::string atom_type = isHydrogen ? "Hydrogen" : "Oxygen";
	char m_type = isHydrogen ? 'H' : 'O';

	// Wait for server to send "STARTBOND"
	std::cout << "Waiting for other client..." << std::endl;
	std::unique_lock<std::mutex> lck(mtx);
	cv.wait(lck, [this]
			{ return isReady; });

	std::cout << "Atom Type: " << atom_type << std::endl;

	// Start timer
	auto start = std::chrono::high_resolution_clock::now();

	// Send atoms to the server
	for (int i = 0; i < atoms; i++)
	{
		int m_number = i + 1;
		// (H0 Request / O0 Request)
		// m_type+m_number+" Request"+timestamp
		// Timestamp is YYYY-MM-DD HH:MM:SS
		std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
		std::time_t timestamp = std::chrono::system_clock::to_time_t(current_time);
		struct tm local_time;
		localtime_s(&local_time, &timestamp);

		char time_str[20]; // Enough space to hold "YYYY-MM-DD HH:MM:SS\0"
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_time);

		std::string message = m_type + std::to_string(m_number) + ", request, " + time_str;

		// std::cout << message << std::endl;
		int sent = send(m_socket, message.c_str(), message.size() + 1, 0);

		if (sent < 0)
		{
			std::cerr << "Error sending message to server" << std::endl;
			exit(1);
		}

		// Update atom_status to indicate that the atom has been requested
		std::get<0>(atom_status[i]) = true;

		// Print Sent message
		std::cout << message << std::endl;
	}

	// Send "Done" to the server
	std::string message = "Done";
	int sent = send(m_socket, message.c_str(), message.size() + 1, 0);

	if (sent < 0)
	{
		std::cerr << "Error sending message to server" << std::endl;
		exit(1);
	}

	// Wait until the server is done
	// Join the thread
	m_thread.join();

	// Stop timer
	auto end = std::chrono::high_resolution_clock::now();

	// Calculate the time taken
	std::chrono::duration<double> time_taken = end - start;
	std::cout << "Time taken: " << time_taken.count() << "s" << std::endl;

	// Sanity check
	std::cout << "Checking request and bond:\n";
	for (int i = 0; i < atom_status.size(); i++)
	{
		bool requested = std::get<0>(atom_status[i]);
		bool bonded = std::get<1>(atom_status[i]);

		if (requested && bonded)
		{
			std::cout << m_type << i + 1 << " passed" << std::endl;
		}
		else if (!requested && bonded)
		{
			std::cout << m_type << i + 1 << " failed (bonded before request)" << std::endl;
		}
		else if (requested && !bonded)
		{
			std::cout << m_type << i + 1 << " failed (requested but not bonded)" << std::endl;
		}
		else
		{
			std::cout << m_type << i + 1 << " failed (no request or bond)" << std::endl;
		}
	}
}

/**
 * Prepares the atoms to be sent to the server
 * @param type 0 for hydrogen, 1 for oxygen
 */
void Client::prepareAtoms(int type)
{
	// ask client for the number of atoms
	std::cout << "Enter the number of atoms: ";
	std::cin >> atoms;

	atom_status = std::vector<std::tuple<bool, bool>>(atoms, std::make_tuple(false, false));

	if (type == 0)
	{
		this->isHydrogen = true;
	}
	else
	{
		this->isHydrogen = false;
	}
}

/**
 * Simply listens to requests from the server so that it can receive messages
 * as it is sending atoms concurrently.
 */
void Client::listener()
{
	while (isRunning)
	{
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

		// If message is "Done", then stop the listener
		if (std::string(buffer, 0, bytesReceived) == "Done")
		{
			isRunning = false;
			break;
		}
		else if (std::string(buffer, 0, bytesReceived) == "STARTBOND")
		{
			std::cout << "\nBegin requesting data from server" << std::endl;
			isReady = true;
			cv.notify_one();
			continue;
		}
		// else store the message and print it
		std::string message = std::string(buffer, 0, bytesReceived);
		std::cout << message << std::endl;

		char m_type = isHydrogen ? 'H' : 'O';
		std::string::size_type start = 0;
		while ((start = message.find(m_type, start)) != std::string::npos)
		{
			// Message is in the format "M#, bonded, timestamp"
			// e.g.: "H5, bonded, 2024-03-08 12:00:00"
			std::string::size_type end = message.find(",", start);
			if (end != std::string::npos)
			{
				// Extract the molecule number
				int molecule_number = std::stoi(message.substr(start + 1, end - start - 1));
				// Mark the molecule as bonded
				std::get<1>(atom_status[molecule_number - 1]) = true;
			}
			start = end;
		}
	}
}