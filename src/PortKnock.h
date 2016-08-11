// PortKnock.h
// ummmm not sure why I've put this inside CryptRelay... ******* PENDING DELETION *******

// Overview:
// This class deals with all checks for open ports and closed ports.
// Class PortKnock does not do input validation.

#ifndef PortKnock_h__
#define PortKnock_h__

#include "CommandLineInput.h"

class PortKnock
{
public:
	PortKnock();
	virtual ~PortKnock();

	//
	int32_t isLocalPortInUse(std::string port, std::string my_local_ip);

	// Very simple checking of 1 port. Not for checking many ports quickly.
	bool isPortOpen(std::string ip, std::string port);

protected:
private:

	// Prevent anyone from copying this class
	PortKnock(PortKnock& PortKnockInstance) = delete; // Delete copy operator
	PortKnock& operator=(PortKnock& PortKnockInstance) = delete; // Delete assignment operator
};

#endif//PortKnock_h__