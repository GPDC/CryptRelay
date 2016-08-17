// PortKnock.h

// Overview:
// This class deals with all checks for open ports and closed ports.

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

	// Checks to see if the specified port is in use on the user's local computer.
	int32_t isLocalPortInUse(std::string port, std::string my_local_ip);

	// Very simple checking of 1 port. Not for checking many ports quickly.
	int32_t isPortOpen(std::string ip, std::string port);

protected:
private:

	// Prevent anyone from copying this class
	PortKnock(PortKnock& PortKnockInstance) = delete; // Delete copy operator
	PortKnock& operator=(PortKnock& PortKnockInstance) = delete; // Delete assignment operator

	IXBerkeleySockets * BerkeleySockets = nullptr;

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	int32_t WSAStartup();

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32

};

#endif//PortKnock_h__