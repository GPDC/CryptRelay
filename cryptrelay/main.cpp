#include <iostream>
#include <string>
#include "ipaddress.h"

int main()
{
	std::cout << "Welcome to the chat program. Enter the IP address of the person you wish to chat with:\n";
	std::string targetIPstring = "";

	//user inputs the target IP address
	ipaddress ipaddress_o;
	ipaddress_o.get_target();

	std::string pause = "";
	std::cin >> pause;
}