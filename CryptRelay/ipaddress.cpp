#include "ipaddress.h"
#include <string>
#include <iostream>

ipaddress::ipaddress()
{

}
ipaddress::~ipaddress()
{

}

int ipaddress::get_target()
{
	//get target IP address from user
	std::string targetIPstring = "";
	std::cout << "Please input the target IP address: ";
	std::getline(std::cin, targetIPstring);

	//check for ipv4 or ipv6 formatting
	int protocol = check_protocol(targetIPstring);

	//if it isn't ipv4 or ipv6, then the bad input was received from the user.
	if (protocol != 4 && protocol != 6)
	{
		get_target();
	}
	//if the IP is in a valid format xxx.xxx.xxx.xxx, then do stuff
	if (check_valid_format(targetIPstring, protocol) == true)
	{

	}
	else
	{
		std::cout << "ERROR: Invalid IP format.\n";
		get_target();
	}
	return 0;
}

bool ipaddress::is_number_or_period(char character)
{
	const int comparison_array_size = 11;
	const char comparison_array[comparison_array_size] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.' };

	//for i char in targetIPaddress, iterate through the comparison_array checking to see if there is a match
	for (int c = 0; c <= comparison_array_size; c++)
	{
		//if it is a period or a number return true
		if (character == comparison_array[c])
		{
			return true;
		}
	}
	//if it didn't find a period or number, return false
	return false;
}

bool ipaddress::check_valid_format(std::string targetIPaddress, int protocol)
{	
	//if ipv4
	if (protocol == 4)
	{
		//if its above char size 15 it can't possibly be a valid ipv4 address.
		if (targetIPaddress.size() > 15)
		{
			std::cout << "Bad Input. This is not a valid IP address.\n";
			get_target();
		}
		int number_before_last_period_counter = 0;
		int period_counter = 0;

		//iterate through array targetIPaddress to check for numbers and periods
		for (unsigned int i = 0; i < targetIPaddress.size(); i++)
		{
			//if i isn't a number or period, send user to input IP again
			if (is_number_or_period(targetIPaddress[i]) == false)
			{
				std::cout << "ERROR: not a number or period\n";
				get_target();
			}
			//if i is a period, +1 to period counter
			if (targetIPaddress[i] == '.')
			{
				period_counter = period_counter + 1;
				number_before_last_period_counter = 0;
			}
			//if it isn't a period, it is a number! +1 number since last period counter
			else
			{
				number_before_last_period_counter = number_before_last_period_counter + 1;
			}
			//if theres more than 3 numbers in a subdomain, then it isn't an IP address. send user to input IP again.
			if (number_before_last_period_counter > 3)
			{
				std::cout << "ERROR: too many numbers in the subdomain.\n";
				get_target();
			}

		}
	}
	//if ipv6
	else if (protocol == 6)
	{

	}
	//shouldn't even go here
	else
	{
		std::cout << "Unexpected else{} reached due to poor design. It isn't ipv4 or ipv6. How did I get here?\n";
	}
	return true;
}

//check for ipv4 or ipv6
int ipaddress::check_protocol(std::string targetIPaddress)
{
	int answer = 999;
	static const char digit_indices[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9'};//put single quotes around it to make it a character literal instead of int literal (not quotes)
	static const size_t input_size = targetIPaddress.size();

	for (unsigned int i = 0; i < input_size; i++)
	{
		if (input_size >= 45)
		{
			std::cout << "Error. That is not a valid IP address.\n";
			answer = 999;
			return answer;
		}
		/*for (int c = 0; c <= 9; c++)
		{
			if (targetIPaddress[i] == digit_indices[c])
			{
				std::cout << "woohoo targetIPaddress " << targetIPaddress[i] << " has matched digit_indices " << digit_indices[c] << "\n";
				answer = 4;
				return answer;
			}
		}*/
		if (targetIPaddress[i] == '.')
		{
			std::cout << "ipv4 detected.\n";
			answer = 4;
			return answer;
		}
		if (targetIPaddress[i] == ':')
		{
			std::cout << "ipv6 detected.\n";
			answer = 6;
			return answer;
		}
		if (targetIPaddress[i] == '>' || targetIPaddress[i] == ',' || targetIPaddress[i] == ';')
		{
			std::cout << "Bad input. Not a valid IP address.\n";
			answer = 999;
			return answer;
		}
	}
	return answer;
}