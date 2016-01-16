//ipv4 format checking is complete.
//ipv6 not currently implemented.


#include <iostream>
#include <string>
#include "ipaddress.h"

int main()
{
	std::cout << "Welcome to the chat program. Enter the IPv4 address of the person you wish to chat with:\n";
	std::string targetIPstring = "";
		
	ipaddress ipaddress_o;
	//user inputs the target IP address
	std::string target = ipaddress_o.get_target();
	
	std::string pause = "";
	std::cin >> pause;
}