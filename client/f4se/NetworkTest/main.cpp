#include "../f4mp/Librg.h"

#include <iostream>

int main()
{
	f4mp::networking::Networking* networking = new f4mp::librg::Librg(false);

	networking->onConnectionRequest = [](f4mp::networking::Event& event)
	{
		std::cout << "connection requested." << std::endl;
	};

	networking->onConnectionAccept = [](f4mp::networking::Event& event)
	{
		std::cout << "connection accepted." << std::endl;
	};

	networking->onConnectionRefuse = [](f4mp::networking::Event& event)
	{
		std::cout << "connection refused." << std::endl;
	};

	networking->Start("localhost", 7779);

	while (true)
	{
		networking->Tick();
	}

	return 0;
}