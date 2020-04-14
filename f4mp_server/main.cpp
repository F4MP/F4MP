#define LIBRG_IMPLEMENTATION

#include "Server.h"

#include <iostream>
#include <fstream>

f4mp::Server* f4mp::Server::instance = nullptr;

int main()
{
	const std::string configFilePath = "server_config.txt";
	std::string address;

	std::ifstream config(configFilePath);
	if (config)
	{
		config >> address;
		config.close();
	}
	else
	{
		std::cout << "address? ";
		std::cin >> address;
		
		std::ofstream file(configFilePath);
		file << address;
	}

    f4mp::Server* server = new f4mp::Server(address);

	return 0;
}