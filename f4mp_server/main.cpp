#define LIBRG_IMPLEMENTATION

#include "Server.h"

#include <iostream>

int main()
{
    std::string address;

    std::cout << "address? ";
    std::cin >> address;

    f4mp::Server* server = new f4mp::Server(address);

	return 0;
}