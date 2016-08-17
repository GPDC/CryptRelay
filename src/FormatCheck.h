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


class FormatCheck
{
public:
	FormatCheck();
	virtual ~FormatCheck();

	static bool isIPV4FormatCorrect(char* target_ipaddress);
	static bool isPortFormatCorrect(char* port);

protected:
private:

	// Prevent anyone from copying this class
	FormatCheck(FormatCheck& FormatCheckInstance) = delete; // Delete copy operator.
	FormatCheck& operator=(FormatCheck& FormatCheckInstance) = delete; // Delete assignment operator.

	static const int32_t BAD_FORMAT = -100;
	static const int32_t MAX_PORT_LENGTH = 5;
	static const int32_t MAX_PORT_NUMBER = 65535;		// Ports are 0-65535	(a total of 65536 ports) but port 0 is generally reserved or not used.

	static const int32_t INET_ADDR_STR_LEN = 15;		// max size of ipv4 address 15 / ipv6 is 45

	// Make sure the IPv4 address octets stay between 0-255.
	static bool checkIPv4SubnetRange(std::string target_ipaddress, int32_t start, int32_t end);

	// For finding the location of the next '.' in an IP address.
	static int32_t findNextIPv4Period(std::string target_ipaddress, int32_t start);
};
#endif
