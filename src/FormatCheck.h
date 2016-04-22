//FormatCheck.h

// Overview:
// Purpose is for checking for proper format on a given thing.
//  Examples: checking for proper IPv4 format, IPv6, Port number
//  is in a valid range and has numbers, etc. This is generally
//  to make sure the user isn't providing wierd input, and to
//  let the user know if he is.

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
	bool checkSubnetRange(std::string target_ipaddress, int start, int end);
	int findNextPeriod(std::string target_ipaddress, int start);
};
#endif