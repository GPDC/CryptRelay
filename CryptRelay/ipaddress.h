//ipaddress.h
#ifndef ipaddress_h__
#define ipaddress_h__
#include <string>

class ipaddress
{
public:
	ipaddress();
	~ipaddress();
	bool get_target(char* targetIPaddress);
	bool is_ipv4_format_correct(std::string targetIPaddress);
	bool check_subnet_range(std::string targetIPaddress, int start, int end);
	int find_next_period(std::string targetIPaddress, int start);

private:
protected:
};
#endif