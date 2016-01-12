//todo: change ip checking function to a better one. iterate through the char array finding the address of all periods. take all things between that period and the last period.
//now you have those addressess. check to see if they are numbers. if numbers, multiply each number by 10, then add them together. if > 255, return false.


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