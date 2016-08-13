//FormatCheck.cpp

#ifdef __linux__
#include <iostream>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#include "FormatCheck.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <string>
#include <iostream>

#include "FormatCheck.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32

FormatCheck::FormatCheck()
{

}
FormatCheck::~FormatCheck()
{
}

// returns true for correct, false for incorrect
bool FormatCheck::isIPV4FormatCorrect(char* ipaddr)
{
	std::string ipaddress = ipaddr;
	int32_t ipaddress_size = ipaddress.size();

	// If the string is too big to be an IPV4 address
	if (ipaddress_size > INET_ADDR_STR_LEN)
	{
		std::cout << "IPV4 address is too big to be valid.\n";
		return false;
	}

	// Check if the formatting for the IP address is correct
	int32_t period_count = 0;
	int32_t start = 0;
	int32_t end = 0;

	for (int32_t c = 0; c < ipaddress_size; c++)
	{
		// Checking for ascii 0-9 and '.'   ... if it isn't any of those, then it
		// sends the user to try again with different input.
		// Grouping together multiple OR statements or AND statements with
		// parenthesis makes them group up into a single BOOL statement.
		if ((ipaddress[c] < '0' || ipaddress[c] > '9') && ipaddress[c] != '.')
		{
			std::cout << "Only numbers and periods allowed.\n";
			return false;
		}
	}

	for (int32_t i = 0; i < 4; i++)
	{
		end = findNextPeriod(ipaddress, start);
		if (end == BAD_FORMAT)
		{
			return false;
		}
		if (end != BAD_FORMAT && end != -1)
		{
			period_count++;
		}
		if (end == -1)
		{
			end = ipaddress_size;
		}
		if (checkSubnetRange(ipaddress, start, end) == true)
		{
			return false;
		}
		//back to the future
		start = end + 1;
		if (end == ipaddress_size)
		{
			break;
		}
	}

	if (period_count != 3)
	{
		return false;
	}
	else
	{
		return true;
	}
}


int32_t FormatCheck::findNextPeriod(std::string ipaddress, int32_t start)
{
	int32_t size_of_ip_address = ipaddress.size();
	int32_t period_location = -1;

	// If it isn't the starting subnet, then give it a +1 or else it
	// will always find the next period where the last period was.
	if (start > 0)
	{
		start = start + 1;
	}

	// Start iterating at the spot of the last period.
	for (; start < size_of_ip_address; start++)
	{
		// Check for two periods right next to eachother
		if (ipaddress[start + 1] == '.' && ipaddress[start] == '.')
		{
			return BAD_FORMAT;
		}

		// If it is a period, save the location. else, increment
		if (ipaddress[start] == '.')
		{
			period_location = start;
			if (period_location == 0)
				return BAD_FORMAT;
			break;
		}
	}
	return period_location;
}

bool FormatCheck::checkSubnetRange(std::string ipaddress, int32_t start, int32_t end)
{
	int32_t subnet_array_count = end - start;

	// Add protection for arrays going out of bounds, and
	// check for invalid IP address at the same time
	if (subnet_array_count > 3 || subnet_array_count < 1)
	{
		std::cout << "Subnet out of range\n";
		return true;
	}

	const int32_t subnet_array_size = 3;
	int32_t subnet_array[subnet_array_size] = {};

	//ascii -> decimal.
	int32_t last_period = start;
	for (int32_t i = 0; i < subnet_array_count; i++, last_period++)
	{
		subnet_array[i] = ipaddress[last_period] - 48;
	}

	// The subnet was broken apart into individual single digit numbers.
	// Now put them back together using multiplication.
	int32_t total = 0;
	for (int32_t i = 0; i < subnet_array_count; i++)
	{
		total = (total * 10) + subnet_array[i];
	}

	// Check for valid subnet range.
	if (total > 255 || total < 0)
	{
		std::cout << "Subnet out of range.\n";
		return true;
	}
	return false;
}


bool FormatCheck::isPortFormatCorrect(char* port)
{
	int32_t length_of_port = strlen(port);

	if (length_of_port > MAX_PORT_LENGTH)
	{
		std::cout << "ERROR: Port number is too high\n";
		return true;
	}
	for (int32_t i = 0; i < length_of_port; ++i)	
	{
		// ascii 48 == 0 and ascii 57 == 9
		if (port[i] < 48 || port[i] > 57)
		{
			std::cout << "Please enter a valid port number.\n";
			return true;
		}
	}

	// Convert from ascii to decimal while adding up each individual number
	int32_t total = 0;
	for (int32_t i = 0; i < length_of_port; ++i)
	{
		total = (total * 10) + (port[i] - 48);
	}

	// Now that we know they are all numbers, check for valid port range
	if (total > MAX_PORT_NUMBER)
	{
		std::cout << "Port number is too large. Exiting.\n";
		return true;
	}

	return false;
}
