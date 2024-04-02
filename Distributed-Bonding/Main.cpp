#include "Main.h"
#include "Client.h"
#include "Server.h"
#include <iostream>

// Master Server
std::string MASTER_HOST = "127.0.0.1";
int MASTER_PORT = 5000;

int main()
{
	int machineType;

	std::cout << "Enter the machine type (0 - Client, 1 - Server): ";
	std::cin >> machineType;

	if (machineType == 1)
	{
		Server server(MASTER_HOST, MASTER_PORT);
	}
	else
	{
		int atomType;
		std::cout << "Enter the atom type (0 - H, 1 - O): ";
		std::cin >> atomType;
		Client client(atomType);
		client.init(MASTER_HOST, MASTER_PORT);
		client.run();
	}

	// Ask user to press any key to exit
	std::cout << "Press any key to exit" << std::endl;
	char c;
	std::cin.get(c);

	return 0;
}