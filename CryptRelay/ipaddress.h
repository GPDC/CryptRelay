#pragma once
#ifndef ipaddress_h__
#define ipaddress_h__
#include <string>

class ipaddress
{
public:
	ipaddress();
	~ipaddress();
	int get_target();
	bool is_number_or_period(char character);
	bool check_valid_format(std::string targetIPaddress, int protocol);
	int check_protocol(std::string targetIPaddress);

private:
protected:
};

#endif