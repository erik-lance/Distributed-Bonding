#include "Main.h"
#include "Client.h"

// Type of Machine (0 - Client, 1 - Server)
constexpr int MACHINE_TYPE = 0;
constexpr int MOLECULE_TYPE = 0; // (For client, 0 - H, 1 - O)

int main() {
	if (MACHINE_TYPE == 0) {
		Client client(MOLECULE_TYPE);
	} else {
		//Server server;
		//server.run();
	}
	return 0;

}