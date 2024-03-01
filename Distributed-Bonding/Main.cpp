#include "Main.h"
#include "Client.h"

// Type of Machine (0 - Client, 1 - Server)
constexpr int MACHINE_TYPE = 0;
constexpr int MOLECULE_TYPE = 0; // (For client, 0 - H, 1 - O)

// Master Server
std::string MASTER_HOST = "192.168.1.5";
int MASTER_PORT = 5000;


int main() {
	if (MACHINE_TYPE == 0) {
		Client client(MOLECULE_TYPE);
		client.init(MASTER_HOST, MASTER_PORT);
	} else {
		//Server server;
		//server.run();
	}
	return 0;

}