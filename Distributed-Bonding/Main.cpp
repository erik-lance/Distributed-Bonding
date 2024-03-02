#include "Main.h"
#include "Client.h"
#include <iostream>

// Master Server
std::string MASTER_HOST = "192.168.1.5";
int MASTER_PORT = 5000;

int main()
{
	int machineType;

	std::cout << "Enter the machine type (0 - Client, 1 - Server): ";
	std::cin >> machineType;

	if (machineType == 1)
	{
		// Server server;
		// server.run();
	}
	else
	{
		int moleculeType;
		std::cout << "Enter the molecule type (0 - H, 1 - O): ";
		std::cin >> moleculeType;
		Client client(moleculeType);
		client.init(MASTER_HOST, MASTER_PORT);
	}

	return 0;
}