//FormatCheck.h

// Overview:
// Purpose is for checking for proper format on a given thing.
//  Examples: checking for proper IPv4 format, IPv6, Port number
//  is in a valid range and has numbers, etc. This is generally
//  to make sure the user isn't providing wierd input, and to
//  let the user know if they are.

#ifndef FormatCheck_h__
#define FormatCheck_h__
#include <string>


class IPAddress
{
public:
	IPAddress();
	virtual ~IPAddress();

	bool isIPV4FormatCorrect(char* target_ipaddress);
	bool isPortFormatCorrect(char* port);

protected:
private:

	const int BAD_FORMAT = -100;
	const int MAX_PORT_LENGTH = 5;
	const int MAX_PORT_NUMBER = 65535;		// Ports are 0-65535	(a total of 65536 ports) but port 0 is generally reserved or not used.

	const int INET_ADDRSTRLEN = 15;			// max size of ipv4 address / ipv6 is 45

	bool checkSubnetRange(std::string target_ipaddress, int start, int end);
	int findNextPeriod(std::string target_ipaddress, int start);
};
#endif