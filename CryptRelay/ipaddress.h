//ipaddres.h
#pragma once
#include <string>

class ipaddress
{
public:
	ipaddress();
	~ipaddress();
	bool get_target(char* targetIPaddress);
	bool ipaddress::is_ipv4_format_correct(std::string targetIPaddress);
	bool ipaddress::check_subnet_range(std::string targetIPaddress, int start, int end);
	int ipaddress::find_next_period(std::string targetIPaddress, int start);

private:
protected:
};