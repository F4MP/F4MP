#define LIBRG_IMPLEMENTATION
//#define LIBRG_DISABLE_FEATURE_ENTITY_VISIBILITY

#include "Server.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

f4mp::Server* f4mp::Server::instance = nullptr;

int main(int argc, char* argv[])
{
	const std::string address = "localhost";
	i32 port = 7779;

	if (argc > 1)
	{
		try
		{
			port = std::stoi(std::string(argv[1]));
		}
		catch (const std::exception& e)
		{
			std::cerr << "ERROR: " << e.what() << std::endl;
			std::exit(1);
		}
	}


	auto server = new f4mp::Server(address, port);

	server->Start();

	while (true)
	{
		server->Tick();
	}

	return 0;
}
