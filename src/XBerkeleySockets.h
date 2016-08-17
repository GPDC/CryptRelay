// XBerkeleySockets.h

// Overview:
// Purpose of this class is to make cross platform versions of
// the Berkeley Sockets API. The 'X' in the class name stands
// for X-platform, or cross-platform.

// Terminology:
// Below is terminology with simple descriptions for anyone new to socket programming:
// fd stands for File Descriptor. It is linux's version of a SOCKET.
// SOCKET is an end-point that is defined by an IP-address and port.
//   A socket is just an integer with a number assigned to it.
//   That number can be thought of as a unique ID for the connection
//   that may or may not be established.


#ifndef XBerkeleySockets_h__
#define XBerkeleySockets_h__


#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>

#include "IXBerkeleySockets.h"
#endif//__linux__

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>	// <sys/socket.h>
#include <string>

#include "IXBerkeleySockets.h"
#endif//_WIN32


class XBerkeleySockets : public IXBerkeleySockets
{
	// Typedefs
public:
#ifdef __linux__
	typedef int32_t SOCKET;
#endif//__linux__

public:
	XBerkeleySockets();
	virtual ~XBerkeleySockets();

	// Shuts down the current connection that is active on the given socket.
	// The shutdown operation is one of three macros.
	// SD_RECEIVE, SD_SEND, SD_BOTH.
	int32_t shutdown(SOCKET socket, int32_t operation);
	
	// Cross-platform closing of a socket / fd.
	void closesocket(SOCKET socket);

	// All addrinfo structures that have been allocated by the getaddrinfo()
	// function must be freed once they are done being used. Since getaddrinfo()
	// gives you a pointer to the allocated addrinfo structure, you should
	// do something like this example:
	// addrinfo * ServerConnectionInfo = nullptr;
	// getaddrinfo(my_local_ip, my_local_port, &ServerHints, &ServerConnectionInfo)
	// 	if (ServerConnectionInfo != nullptr)
	//		Socket->freeaddrinfo(&ServerConnectionInfo);
	//
	// Never freeaddrinfo() on something that has already been freed.
	// In order to avoid doing that, check for a nullptr first.
	// If nullptr, it has already been freed.
	// Example:
	// if (&ClientConnectionInfo != nullptr)
	// {
	//      this->freeaddrinfo(&ClientConnectionInfo);
	// }
	void freeaddrinfo(addrinfo** ppAddrInfo);

	// Display the IP and Port of the peer you have a connection with on the SOCKET.
	void coutPeerIPAndPort(SOCKET connection_with_peer);


	// For most consistent results, errno needs to be set to 0 before
	// every function call that can return an errno.
	// This method is only intended for use with things that deal with sockets.
	// getError() will output the error code + description unless
	// output_to_console arg is given false.
	// The return value is just incase you want to do something specific with the error code.
	// On windows, this function returns WSAERROR codes.
	// On linux, this function returns errno codes.
	int32_t getError(bool output_to_console = true);
	static const bool DISABLE_CONSOLE_OUTPUT;


	// Only intended for use with Socket errors.
	// Attempts to output a description for the error code.
	// On windows it expects a WSAERROR code.
	// On linux it expects an errno code.
	void outputSocketErrorToConsole(int32_t error_code);

	// With this method you can enable or disable blocking for a given socket.
	// FYI: SOCKETs are blocking by default
	// DISABLE_BLOCKING == 1;
	// ENABLE_BLOCKING == 0;
	// returns 0, success
	// returns -1, error
	int32_t setBlockingSocketOpt(SOCKET fd_socket, const u_long* option);
	static const unsigned long DISABLE_BLOCKING;
	static const unsigned long ENABLE_BLOCKING;

	// Get error information from the socket.
	// Returns SOCKET_ERROR, error
	// Returns the socket option error code, success
	int32_t getSockOptError(SOCKET fd_socket);
	

protected:
private:

	// Prevent anyone from copying this class.
	XBerkeleySockets(XBerkeleySockets& XBerkeleySocketsInstance) = delete;			 // disable copy operator
	XBerkeleySockets& operator=(XBerkeleySockets& XBerkeleySocketsInstance) = delete; // disable assignment operator

public:
	// Accessors

	// Even though these constants are public, accessors were made so
	// that ppl can access them through the interface.
	bool getDisableConsoleOutput() { return DISABLE_CONSOLE_OUTPUT; }
	const unsigned long& getDisableBlocking() { return DISABLE_BLOCKING; }
	const unsigned long& getEnableBlocking() { return ENABLE_BLOCKING; }
};

#endif//XBerkeleySockets_h__
