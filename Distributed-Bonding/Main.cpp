#include "Main.h"
#include "Client.h"
#include "Server.h"
#include <iostream>

// Master Server
std::string MASTER_HOST = "127:0.0.1";
int MASTER_PORT = 5000;

int main()
{
	int machineType;

	std::cout << "Enter the machine type (0 - Client, 1 - Server): ";
	std::cin >> machineType;

	if (machineType == 1)
	{
		Server server(MASTER_HOST, MASTER_PORT);
		server.start();
	}
	else
	{
		int moleculeType;
		std::cout << "Enter the molecule type (0 - H, 1 - O): ";
		std::cin >> moleculeType;
		Client client(moleculeType);
		client.init(MASTER_HOST, MASTER_PORT);
		client.run();
	}

	return 0;
}