#include "Main.h"
#include "Client.h"
#include "Globals.h"


int main() {
	if (MACHINE_TYPE == 0) {
		Client client(MOLECULE_TYPE);
	} else {
		//Server server;
		//server.run();
	}
	return 0;

}