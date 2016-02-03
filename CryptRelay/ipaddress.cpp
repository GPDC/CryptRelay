//ipaddress.cpp

//wrote this all 3 majorly different ways 3 different times as practice. (didn't use library functions that would make this extremely quick & easy)
//what is written below is a bit ugly, and could be improved __a_lot__, but it currently works.
//note to self: next time you know what to do just do it even if it means messing up your schedule (to a certain extent). Otherwise you will forget how it was to be written.
//note to self: don't write notes for how to write the code next time. write the code instead, or perhaps try quick and dirty sudo code?.
//
//for the next project, lay it all out first. design it with loose terms, sudo code, w/e. typing away and hoping it all falls into place was pretty dumb, but admittedly I learned a lot during the 4 re-writes.
//this is all working, but it is missing safety checks for overflows. these saftey checks being missing for the moment are not 100% scary, I believe, since it is all done client side, and nobody through the internet
//can interact with the variables that don't have safety checks. The attacker would have to have some access to this computer, and at that point, everything is compromised anyways.
//Regardless, I would still like to create safety checks for this class. However I will do this after the connect class has been created and is working.

#include "ipaddress.h"
#include "GlobalTypeHeader.h"
#include <string>
#include <iostream>

ipaddress::ipaddress()
{

}
ipaddress::~ipaddress()
{

}

std::string ipaddress::get_target(char* targetIPaddress)
{
	//get target IP address from user
	if (global_verbose == true)
		std::cout << "targetIPaddress = " << targetIPaddress << "\n";
	bool is_format_good = false;
	while (is_format_good == false){
		//check if the formatting for the IP address is correct
		is_format_good = is_ipv4_format_correct(targetIPaddress);

		//if the format is correct then go ahead and give main the target IP string so it can be used.
		if (is_format_good == true)
			return targetIPaddress;
		else{
			std::cout << "bad IP address format.\n\n";
			targetIPaddress = "bad IP address format.";
			return targetIPaddress;
		}

	}
	return "Error: This shouldn't be possible.\n";
}


int ipaddress::find_next_period(std::string targetIPaddress, int start)
{
	int size_of_ip_address = targetIPaddress.size();
	int period_location = -1;
	//if it isn't the starting subnet, then give it a +1 or else it will always find the next period where the last period was.
	if (start > 0)
		start = start + 1;

	//start iterating at the spot of the last period.
	for (; start < size_of_ip_address; start++){
		//not working yet.
		if (targetIPaddress[start + 1] == '.' && targetIPaddress[start] == '.'){
			if (global_verbose == true)
				std::cout << "2 periods in a row detected.\n";
			//return false
			return -100;
		}

		if (start == size_of_ip_address - 1)
			if (global_verbose == true)
				std::cout << "end of ip address string detected.\n";

		//if it is a period, save the location. else, increment
		if (targetIPaddress[start] == '.'){
			period_location = start;
			if (global_verbose == true)
				std::cout << "found a period at location: " << period_location << ".\n";
			if (period_location == 0)
				return -100;
			break;
		}
	}
	return period_location;
}

bool ipaddress::check_subnet_range(std::string targetIPaddress, int start, int end)
{
	int subnet_array_count = end - start;

	//add protection for arrays going out of bounds, and check for invalid IP address at the same time
	if (subnet_array_count > 3 || subnet_array_count < 1){
		std::cout << "subnet out of range\n";
		return false;
	}

	const int subnet_array_size = 3;
	int subnet_array[subnet_array_size] = {};

	//ascii -> decimal.
	int last_period = start;
	for (int i = 0; i < subnet_array_count; i++, last_period++) {
		subnet_array[i] = targetIPaddress[last_period] - 48;
	}

	//the subnet was broken apart into individual single digit numbers. now put them back together using multiplication.
	int total = 0;
	for (int i = 0; i < subnet_array_count; i++) {
		total = (total * 10) + subnet_array[i];
	}

	//check for valid subnet range.
	if (global_verbose == true)
		std::cout << "subnet addition total = " << total << "\n";
	if (total > 255 || total < 0){
		//total is being saved out of scope for some reason...........................................it has diff behavior if it failed once for my input, then on my next correct format input it will be odd output.
		std::cout << "subnet out of range.\n";
		return false;
	}
	return true;
}


bool ipaddress::is_ipv4_format_correct(std::string targetIPaddress)
{
	int period_count = 0;
	int start = 0;
	int end = 0;

	for (unsigned int c = 0; c < targetIPaddress.size(); c++){
		//checking for ascii 0-9 and '.'   ... if it isn't any of those, then it sends the user to try again with different input.
		//grouping together multiple OR statements or AND statements with parenthesis makes them group up into a single BOOL statement.
		if ((targetIPaddress[c] < '0' || targetIPaddress[c] > '9') && targetIPaddress[c] != '.'){
			std::cout << "Only numbers and periods allowed.\n";
			return false;
		}
	}

	for (int i = 0; i < 4; i++){
		end = find_next_period(targetIPaddress, start);
		if (end == -100)
			return false;
		if (end != -100 && end != -1)
			period_count++;
		if (end == -1)
			end = targetIPaddress.size();
		if (check_subnet_range(targetIPaddress, start, end) == false)
			return false;

		//back to the future
		start = end + 1;
		if (end == targetIPaddress.size())
			break;
	}

	if (period_count != 3)
		return false;
	else
		return true;
}