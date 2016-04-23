//port_knock.h
// ummmm not sure why I've put this inside CryptRelay... ******* PENDING DELETION *******

// Overview:
// This class deals with all checks for open ports and closed ports.

#ifndef port_knock_h__
#define port_knock_h__

#include "CommandLineInput.h"

class PortKnock
{
public:
	PortKnock();
	~PortKnock();

	int isLocalPortInUse(std::string port, std::string my_local_ip);

	// Very simple checking of 1 port. Not for checking many ports quickly.
	bool isPortOpen(std::string ip, std::string port);

protected:
private:
};

#endif//port_knock_h__