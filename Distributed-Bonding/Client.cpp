#include "Client.h"

Client::Client(int type)
{
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

	std::string molecule_type = isHydrogen ? "Hydrogen" : "Oxygen";
	char m_type = isHydrogen ? 'H' : 'O';

	// Wait for server to send "STARTBOND"
	std::cout << "Waiting for other client..." << std::endl;
	std::unique_lock<std::mutex> lck(mtx);
	cv.wait(lck, [this] { return isReady; });

	std::cout << "Moleceule Type: " << molecule_type << std::endl;

	// Start timer
	auto start = std::chrono::high_resolution_clock::now();

	// Send molecules to the server
	for (int i = 0; i < molecules; i++)
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

		//std::cout << message << std::endl;
		int sent = send(m_socket, message.c_str(), message.size() + 1, 0);

		if (sent < 0)
		{
			std::cerr << "Error sending message to server" << std::endl;
			exit(1);
		}

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
}

/**
 * Prepares the molecules to be sent to the server
 * @param type 0 for hydrogen, 1 for oxygen
 */
void Client::prepareMolecules(int type)
{
	//ask client for the number of molecules
	std::cout << "Enter the number of molecules: ";
	std::cin >> molecules;

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
 * as it is sending molecules concurrently.
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
		else if (std::string(buffer, 0, bytesReceived) == "STARTBOND") {
			std::cout << "\nBegin requesting data from server" << std::endl;
			isReady = true;
			cv.notify_one();
			continue;
		}
		std::cout << std::string(buffer, bytesReceived) << std::endl;

	}
}
