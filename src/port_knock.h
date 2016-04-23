//port_knock.h
// ummmm not sure why I've put this inside CryptRelay... ******* PENDING DELETION *******

// Overview:
// This class deals with all checks for open ports and closed ports.

// Warnings:
// This source file expects any input that is given to it has already been checked
//  for safety and validity. For example if a user supplies a port, then
//  this source file will expect the port will be >= 0, and <= 65535.
//  As with an IP address it will expect it to be valid input, however it doesn't
//  expect you to have checked to see if there is a host at that IP address
//  before giving it to this source file.

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