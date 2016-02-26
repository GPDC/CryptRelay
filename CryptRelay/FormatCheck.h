//FormatCheck.h
#ifndef FormatCheck_h__
#define FormatCheck_h__
#include <string>


class IPAddress
{
public:
	IPAddress();
	~IPAddress();

	bool isIPV4FormatCorrect(char* target_ipaddress);
	bool isPortFormatCorrect(char* port);

protected:
private:
	bool checkFormat(std::string targetip_address);
	bool checkSubnetRange(std::string target_ipaddress, int start, int end);
	int findNextPeriod(std::string target_ipaddress, int start);
};
#endif