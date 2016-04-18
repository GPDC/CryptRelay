//FormatCheck.cpp

//wrote this all 3 majorly different ways 3 different times as practice. (didn't use library functions that would make this extremely quick & easy)
//what is written below is a bit ugly, and could be improved __a_lot__, but it currently works.
//note to self: next time you know what to do just do it even if it means messing up your schedule (to a certain extent). Otherwise you will forget how it was to be written.
//note to self: don't write notes for how to write the code next time. write the code instead, or perhaps try quick and dirty sudo code?.
//
//for the next project, lay it all out first. design it with loose terms, sudo code, w/e. typing away and hoping it all falls into place was pretty dumb, but admittedly I learned a lot during the 4 re-writes.
//this is all working, but it is missing safety checks for overflows. these saftey checks being missing for the moment are not 100% scary, I believe, since it is all done client side, and nobody through the internet
//can interact with the variables that don't have safety checks. The attacker would have to have some access to this computer, and at that point, everything is compromised anyways.
//Regardless, I would still like to create safety checks for this class. However I will do this after the connect class has been created and is working.

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
//#include <errno.h>

#include "FormatCheck.h"
#include "GlobalTypeHeader.h"
#endif

#ifdef _WIN32
#include <string>
#include <iostream>

#include "FormatCheck.h"
#include "GlobalTypeHeader.h"
#endif

#define BAD_FORMAT -100
#define MAX_PORT_LENGTH 5
#define MAX_PORT_NUMBER 65535		// Ports are 0-65535	(a total of 65536 ports) but port 0 is generally reserved or not used.

IPAddress::IPAddress()
{
}
IPAddress::~IPAddress()
{
}

bool IPAddress::isIPV4FormatCorrect(char* ipaddr)
{
	std::string ipaddress = ipaddr;

	if (global_verbose == true)
		std::cout << "Unverified IPaddress = " << ipaddress << "\n";

	// Check if the formatting for the IP address is correct
	int period_count = 0;
	int start = 0;
	int end = 0;

	for (unsigned int c = 0; c < ipaddress.size(); c++)
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

	for (int i = 0; i < 4; i++)
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
			end = ipaddress.size();
		}
		if (checkSubnetRange(ipaddress, start, end) == false)
		{
			return false;
		}
		//back to the future
		start = end + 1;
		if (end == ipaddress.size())
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


int IPAddress::findNextPeriod(std::string ipaddress, int start)
{
	int size_of_ip_address = ipaddress.size();
	int period_location = -1;

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
			if (global_verbose == true)
				std::cout << "2 periods in a row detected.\n";
			return BAD_FORMAT;
		}

		if (start == size_of_ip_address - 1)
		{
			if (global_verbose == true)
			{
				std::cout << "End of ip address string detected.\n";
			}
		}

		// If it is a period, save the location. else, increment
		if (ipaddress[start] == '.')
		{
			period_location = start;
			if (global_verbose == true)
				std::cout << "Found a period at location: " << period_location << ".\n";
			if (period_location == 0)
				return BAD_FORMAT;
			break;
		}
	}
	return period_location;
}

bool IPAddress::checkSubnetRange(std::string ipaddress, int start, int end)
{
	int subnet_array_count = end - start;

	// Add protection for arrays going out of bounds, and
	// check for invalid IP address at the same time
	if (subnet_array_count > 3 || subnet_array_count < 1)
	{
		std::cout << "Subnet out of range\n";
		return false;
	}

	const int subnet_array_size = 3;
	int subnet_array[subnet_array_size] = {};

	//ascii -> decimal.
	int last_period = start;
	for (int i = 0; i < subnet_array_count; i++, last_period++)
	{
		subnet_array[i] = ipaddress[last_period] - 48;
	}

	// The subnet was broken apart into individual single digit numbers.
	// Now put them back together using multiplication.
	int total = 0;
	for (int i = 0; i < subnet_array_count; i++)
	{
		total = (total * 10) + subnet_array[i];
	}

	// Check for valid subnet range.
	if (global_verbose == true)
	{
		std::cout << "Subnet addition total = " << total << "\n";
	}
	if (total > 255 || total < 0)
	{
		std::cout << "Subnet out of range.\n";
		return false;
	}
	return true;
}


bool IPAddress::isPortFormatCorrect(char* port)
{
	int length_of_port = strlen(port);
	if (global_verbose == true)
	{
		std::cout << "Unverified port: " << port << "\n";
	}

	if (length_of_port > MAX_PORT_LENGTH)
	{
		std::cout << "ERROR: Port number is too high\n";
		return false;
	}
	for (int i = 0; i < length_of_port; ++i)	
	{
		// ascii 48 == 0 and ascii 57 == 9
		if (port[i] < 48 || port[i] > 57)
		{
			std::cout << "Please enter a valid port number.\n";
			return false;
		}
	}

	// Convert from ascii to decimal while adding up each individual number
	int total = 0;
	for (int i = 0; i < length_of_port; ++i)
	{
		total = (total * 10) + (port[i] - 48);
	}

	// Now that we know they are all numbers, check for valid port range
	if (total > MAX_PORT_NUMBER)
	{
		std::cout << "Port number is too large. Exiting.\n";
		return false;
	}

	return true;
}