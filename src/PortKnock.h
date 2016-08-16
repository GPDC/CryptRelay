// PortKnock.h
// ummmm not sure why I've put this inside CryptRelay... ******* PENDING DELETION *******

// Overview:
// This class deals with all checks for open ports and closed ports.
// Class PortKnock does not do input validation.

#ifndef PortKnock_h__
#define PortKnock_h__

// Forward declaration
class IXBerkeleySockets;

class PortKnock
{
public:
	PortKnock(
		IXBerkeleySockets * IXBerkeleySocketsInstance, // Simply a cross platform implementation of certain Berkeley Socket functions.
		bool turn_verbose_output_on = false
	);
	virtual ~PortKnock();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Checks to see if the specified port is in use on the user's computer.
	int32_t isLocalPortInUse(std::string port, std::string my_local_ip);

	// Very simple checking of 1 port. Not for checking many ports quickly.
	int32_t isPortOpen(std::string ip, std::string port);

protected:
private:

	// Prevent anyone from copying this class
	PortKnock(PortKnock& PortKnockInstance) = delete; // Delete copy operator
	PortKnock& operator=(PortKnock& PortKnockInstance) = delete; // Delete assignment operator

	IXBerkeleySockets * IBerkeleySockets = nullptr;
};

#endif//PortKnock_h__