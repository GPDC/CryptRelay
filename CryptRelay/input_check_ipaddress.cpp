#include <string>
#include <iostream>
#include "input_check_ipaddress.h"

void check_ipv4(std::string targetIPstring)
{
	//whitelist for numbers
	char number[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };


	//check for bad input
	for (int i = 0; i < targetIPstring.size() + 1; i++)
	{
		//compare each char to a whitelist of numbers 0-9, and a period '.'
		if (targetIPstring[i] == number || targetIPstring[i] == period)
		{
			std::cout << targetIPstring[i];
		}

		//break for bad input
		else
			std::cout << "Error. bad input for IP address.";
		break;
	}
	std::cout << "\nYou entered: " << targetIPstring << "\n";
}
